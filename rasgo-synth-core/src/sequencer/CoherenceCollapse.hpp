#pragma once
#include <array>
#include <cmath>
#include <random>

namespace rasgo {

// Fase 117 -- ruptura por excesso de coerencia (task #7, segundo eixo de
// ruptura/rasgo, distinto do rasgo espectral da fase 116 -- o usuario
// pediu explicitamente que ruptura e rasgo fiquem separados). Da secao
// 34 do manifesto do usuario: "um sistema pode romper porque ficou
// coerente demais... a estabilidade absoluta torna-se uma especie de
// colapso. Isso inverte a ideia habitual de que ruptura = caos. As
// vezes: ordem extrema e ruptura." Nenhum mecanismo deste motor ate
// aqui media a propria previsibilidade -- todo mecanismo de
// "instabilidade" ja existente (chaos maps, sequencer wild timing,
// free intonation) empurra rumo a MAIS variedade; este e o primeiro que
// reage quando ha variedade DEMAIS POUCA.
//
// Medida real, nao uma heuristica arbitraria: entropia de Shannon (
// Shannon 1948, "A Mathematical Theory of Communication", Bell System
// Technical Journal -- a definicao matematica real de informacao/
// imprevisibilidade de uma fonte discreta) sobre a distribuicao de
// classes de altura (pitch class, 0-11) das ultimas N notas realmente
// tocadas pela voz principal. H = -sum(p_i * log2(p_i)) sobre as 12
// classes -- 0 bits quando toda nota recente e identica (coerencia
// maxima), ate log2(12) ~ 3,58 bits quando as 12 classes aparecem em
// proporcoes iguais (imprevisibilidade maxima). Quando a entropia fica
// baixa demais por BARRAS CONSECUTIVAS reais (nao um instante isolado --
// a secao 34 do usuario fala de convergencia sustentada, nao uma
// coincidencia de um compasso), a peca sofre um colapso real: um
// "estalo" audivel e uma reorganizacao forcada e temporaria do
// sequencer (mais denso, registro extremo) por algumas barras, depois
// volta ao comportamento normal -- pressao->resistencia->limiar->
// ruptura->reorganizacao, a formula do proprio usuario (secao 37).
class CoherenceCollapse {
public:
    static constexpr int kNumPitchClasses = 12;
    static constexpr int kWindowSize = 24;

    void configure(double entropyThreshold, int sustainedBarsRequired, int cooldownBars, unsigned seed) {
        entropyThreshold_ = entropyThreshold;
        sustainedBarsRequired_ = sustainedBarsRequired;
        cooldownBars_ = cooldownBars;
        rng_.seed(seed);
        window_.fill(0);
        writeIndex_ = 0;
        filled_ = 0;
        consecutiveLowBars_ = 0;
        cooldownRemaining_ = 0;
        collapseCount_ = 0;
    }

    // Chamado toda vez que a voz principal realmente soa uma nota nova.
    void pushNote(int midiNote) {
        int pc = ((midiNote % 12) + 12) % 12;
        window_[static_cast<size_t>(writeIndex_)] = pc;
        writeIndex_ = (writeIndex_ + 1) % kWindowSize;
        if (filled_ < kWindowSize) ++filled_;
    }

    // Entropia real de Shannon, em bits, sobre a janela atual (0 se
    // ainda nao ha notas suficientes pra calcular algo significativo).
    double currentEntropy() const {
        if (filled_ < 4) return std::log2(static_cast<double>(kNumPitchClasses)); // janela vazia = "nao converge ainda", nunca dispara
        std::array<int, kNumPitchClasses> counts{};
        for (int i = 0; i < filled_; ++i) ++counts[static_cast<size_t>(window_[static_cast<size_t>(i)])];
        double h = 0.0;
        for (int c : counts) {
            if (c == 0) continue;
            double p = static_cast<double>(c) / static_cast<double>(filled_);
            h -= p * std::log2(p);
        }
        return h;
    }

    // Chamado uma vez por fronteira real de compasso. Retorna true
    // exatamente no compasso em que um colapso real dispara (nunca dois
    // compassos seguidos -- ha um cooldown real depois).
    bool checkBarBoundary() {
        if (cooldownRemaining_ > 0) { --cooldownRemaining_; return false; }
        double h = currentEntropy();
        if (h < entropyThreshold_) {
            ++consecutiveLowBars_;
        } else {
            consecutiveLowBars_ = 0;
        }
        if (consecutiveLowBars_ >= sustainedBarsRequired_) {
            consecutiveLowBars_ = 0;
            cooldownRemaining_ = cooldownBars_;
            ++collapseCount_;
            return true;
        }
        return false;
    }

    int collapseCount() const { return collapseCount_; }
    int consecutiveLowBars() const { return consecutiveLowBars_; }

private:
    double entropyThreshold_ = 1.0;
    int sustainedBarsRequired_ = 4;
    int cooldownBars_ = 8;
    std::array<int, kWindowSize> window_{};
    int writeIndex_ = 0;
    int filled_ = 0;
    int consecutiveLowBars_ = 0;
    int cooldownRemaining_ = 0;
    int collapseCount_ = 0;
    std::mt19937 rng_;
};

} // namespace rasgo
