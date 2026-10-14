#pragma once
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include "../io/PublishedIndex.hpp"

namespace rasgo {

// Fase 69 -- ideia do usuario: em vez de cada mecanismo seed-gated ter uma
// probabilidade fixa (`unit(rng) < 0.25`, uma constante escrita no codigo,
// igual pra sempre), cada um passa a ter um "preco" que sofre um random
// walk com reversao a media, persistido num arquivo entre renders -- uma
// bolsa de valores real, nao so uma metafora. Um mecanismo usado com
// frequencia recentemente fica mais raro (preco cai); um que nao dispara ha
// varios renders fica mais provavel (preco sobe) -- nunca extingue (piso
// 0.03) nem vira certeza absoluta (teto 0.97), sempre puxado de volta rumo
// ao baseline original do codigo por uma forca de reversao.
//
// Esta e a PRIMEIRA vez que o motor tem estado que persiste ENTRE renders
// (fora a excecao ja documentada do elemento estranho literal, que le a
// maquina real mas nao guarda nada em disco) -- quebra deliberadamente a
// garantia de "mesma seed = mesma peca pra sempre" pra os mecanismos aqui
// cobertos, exatamente como o elemento estranho ja quebra pro seu proprio
// caso: exposto com honestidade nos metadados de cada peca (o preco USADO
// nesta renderizacao especifica fica gravado no JSON), nunca escondido.
//
// Escopo desta fase: os oito mecanismos mais recentes (fases 59-68), todos
// decididos diretamente em PieceRenderer.hpp -- nao os que vivem dentro de
// StepSequencer (voiceLeading/neoRiemannian, seedados pelo proprio charRng
// interno), pra manter a mudanca contida e revisavel em vez de reescrever
// todo sorteio do motor de uma vez.
class ProbabilityMarket {
public:
    // Carries only the persisted price fields to the next render, without
    // disk/serialization rounding or transient baseline/fired bookkeeping.
    ProbabilityMarket continuation() const {
        ProbabilityMarket next;
        next.prices_ = prices_;
        return next;
    }

    // Reads whatever state exists at `path` (silently starts fresh if the
    // file doesn't exist yet, or is malformed -- never a fatal error for
    // an offline renderer, since a missing market file has an obvious,
    // safe fallback: everyone starts at their own coded baseline).
    void load(const std::string &path) {
        std::error_code error;
        const auto nativePath = std::filesystem::u8path(path);
        if (!std::filesystem::is_regular_file(std::filesystem::symlink_status(nativePath, error)) || error) return;
        const auto size = std::filesystem::file_size(nativePath, error);
        if (error || size > kMaxStateBytes) return;
        std::ifstream in(nativePath);
        if (!in.is_open()) return;
        std::string line;
        unsigned linesRead = 0;
        while (std::getline(in, line)) {
            if (++linesRead > kMaxEntries) break;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string name = line.substr(0, eq);
            if (!validName(name)) continue;
            double value = 0.0;
            try { value = std::stod(line.substr(eq + 1)); } catch (...) { continue; }
            if (!std::isfinite(value)) continue;
            prices_[name] = std::clamp(value, kFloor, kCeiling);
        }
    }

    // The probability to actually use THIS render for `name` -- its
    // current market price if one exists yet, else the coded baseline
    // (first time this mechanism is ever priced). `baseline` is also
    // remembered as the reversion target for settle() below.
    double price(const std::string &name, double baseline) {
        baselines_[name] = baseline;
        auto it = prices_.find(name);
        if (it != prices_.end()) return it->second;
        prices_[name] = baseline;
        return baseline;
    }

    // Records whether `name` actually fired this render -- read back by
    // settle() to decide which direction to nudge.
    void record(const std::string &name, bool fired) { fired_[name] = fired; }

    // One update step per mechanism: reversion toward its own coded
    // baseline, a small deterministic nudge away from wherever it just
    // fired (rarer next time) or didn't (more likely next time), plus
    // real random-walk noise -- all bounded so the price can never reach
    // true 0 or 1.
    void settle(std::mt19937 &rng) {
        std::uniform_real_distribution<double> noise(-0.035, 0.035);
        for (auto &entry : prices_) {
            const std::string &name = entry.first;
            double &p = entry.second;
            double baseline = baselines_.count(name) ? baselines_[name] : p;
            double reversion = 0.10 * (baseline - p);
            double firedNudge = 0.0;
            if (fired_.count(name)) firedNudge = fired_[name] ? -0.025 : 0.018;
            p = std::clamp(p + reversion + firedNudge + noise(rng), kFloor, kCeiling);
        }
    }

    void save(const std::string &path) const {
        const auto fail = [&](const char* reason) {
            throw std::runtime_error(std::string("Probability market publication failed (")
                + reason + "); previous state preserved at: " + path);
        };
        if (prices_.size() > kMaxEntries) fail("entry budget exceeded");
        std::ostringstream serialized;
        for (const auto &entry : prices_) {
            if (!validName(entry.first) || !std::isfinite(entry.second)) fail("invalid entry");
            serialized << entry.first << "=" << entry.second << "\n";
        }
        const auto text = serialized.str();
        if (!serialized.good() || text.size() > kMaxStateBytes) fail("state budget exceeded");
        // Same local cooperative writer contract as the index, but replace a
        // complete state rather than copying/appending old bytes. Offline only.
        const auto target = std::filesystem::u8path(path);
        index_detail::Lease lease(target);
        if (lease.status() != IndexAppendStatus::Complete) fail(indexAppendMessage(lease.status()));
        index_detail::Snapshot before;
        if (!index_detail::snapshot(target, before)) fail("invalid target");
        if (before.size > kMaxStateBytes) fail("previous state exceeds budget");
        RecordingPublication publication(target);
        if (!publication.ready()) fail("staging unavailable");
        std::ofstream out(publication.temporaryPath(), std::ios::binary | std::ios::trunc);
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        out.flush();
        const bool written = out.good();
        out.close();
        if (!written || out.fail()) fail("write/flush/close failed");
        std::error_code error;
        if (before.exists && before.permissions != std::filesystem::perms::unknown) {
            std::filesystem::permissions(publication.temporaryPath(), before.permissions, error);
            if (error) fail("permissions unavailable");
        }
        index_detail::Snapshot after;
        if (!index_detail::snapshot(target, after) || !index_detail::same(before, after)) fail("target changed");
        if (!before.exists) {
            if (!publication.publish()) fail("new state publication failed");
        } else {
            std::filesystem::rename(publication.temporaryPath(), target, error);
            if (error) fail("state replacement failed");
        }
    }

    // For per-piece metadata transparency -- the exact prices USED this
    // render (before settle() moves them for the next one).
    const std::map<std::string, double> &prices() const { return prices_; }

private:
    static constexpr std::uintmax_t kMaxStateBytes = 64 * 1024;
    static constexpr unsigned kMaxEntries = 512;
    static bool validName(const std::string& name) {
        return !name.empty() && name.size() <= 128
            && name.find_first_of("=\r\n") == std::string::npos && name.find('\0') == std::string::npos;
    }
    static constexpr double kFloor = 0.03;
    static constexpr double kCeiling = 0.97;
    std::map<std::string, double> prices_;
    std::map<std::string, double> baselines_;
    std::map<std::string, bool> fired_;
};

} // namespace rasgo
