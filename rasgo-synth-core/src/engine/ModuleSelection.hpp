#pragma once
#include <array>
#include <cstddef>
#include <ostream>
#include <stdexcept>

namespace rasgo {
enum class ModuleChoice : unsigned char { Automatic, Include, Exclude };
enum class StudioModule : unsigned char {
    Core, Acid, Tuba, Theremin, FmFlute, Hocket, Galo, Scat, Choir, DejaVu,
    Harmonaig, Sieve, Bloom, Walsh, Vosim, HardSync, Binaural, Vector,
    Spectral, Footnote, AtariPunk, EnvelopePulse, Granular, Gendyn, NoiseBed,
    Drums, Kick, Snare, HiHat, Cowbell, Clap, Toms, Cymbal, Maracas, Count
};
inline constexpr std::size_t studioModuleCount = static_cast<std::size_t>(StudioModule::Count);
inline constexpr std::array<const char*, studioModuleCount> studioModuleIds {
    "core", "acid", "tuba", "theremin", "fm-flute", "hocket", "galo", "scat", "choir", "deja-vu",
    "harmonaig", "sieve", "bloom", "walsh", "vosim", "hard-sync", "binaural", "vector",
    "spectral", "footnote", "atari-punk", "envelope-pulse", "granular", "gendyn", "noise-bed",
    "drums", "kick", "snare", "hi-hat", "cowbell", "clap", "toms", "cymbal", "maracas"
};
inline const char* moduleChoiceName(ModuleChoice choice) {
    return choice == ModuleChoice::Include ? "include" : choice == ModuleChoice::Exclude ? "exclude" : "automatic";
}

// The musician chooses eligibility before rendering, not a mute on a final WAV.
// Include permits an entrance; it does not assert continuous/audible activity.
struct ModuleSelection {
    std::array<ModuleChoice, studioModuleCount> choices{};
    ModuleChoice get(StudioModule module) const { return choices.at(static_cast<std::size_t>(module)); }
    void set(StudioModule module, ModuleChoice choice) {
        if (choice > ModuleChoice::Exclude) throw std::invalid_argument("Invalid module choice");
        choices.at(static_cast<std::size_t>(module)) = choice;
    }
    bool choose(StudioModule module, bool automatic, bool dependency = true) const {
        const auto choice = get(module);
        return dependency && choice != ModuleChoice::Exclude
            && (choice == ModuleChoice::Include || automatic);
    }
    bool hasIncludedDrum() const {
        for (unsigned i = static_cast<unsigned>(StudioModule::Kick); i < studioModuleCount; ++i)
            if (choices[i] == ModuleChoice::Include) return true;
        return false;
    }
    void writeJson(std::ostream& out) const {
        out << "\"moduleSelection\":{";
        for (std::size_t i = 0; i < choices.size(); ++i) {
            if (i) out << ',';
            out << '"' << studioModuleIds[i] << "\":\"" << moduleChoiceName(choices[i]) << '"';
        }
        out << "},";
    }
};

class ModuleSelectionTrace {
public:
    explicit ModuleSelectionTrace(const ModuleSelection& selection) : selection_(selection) { reasons_.fill(""); }
    bool resolve(StudioModule module, bool automatic, bool dependency = true, const char* reason = "") {
        const auto i = static_cast<std::size_t>(module);
        enabled_.at(i) = selection_.choose(module, automatic, dependency);
        reasons_.at(i) = !dependency && selection_.get(module) == ModuleChoice::Include ? reason : "";
        return enabled_[i];
    }
    void writeJson(std::ostream& out) const {
        selection_.writeJson(out);
        out << "\"moduleSelectionResolved\":{";
        for (std::size_t i = 0; i < enabled_.size(); ++i) {
            if (i) out << ',';
            out << '"' << studioModuleIds[i] << "\":{\"enabled\":" << (enabled_[i] ? "true" : "false")
                << ",\"blockedBy\":\"" << reasons_[i] << "\"}";
        }
        out << "},";
    }
    bool enabled(StudioModule module) const { return enabled_.at(static_cast<std::size_t>(module)); }
    const char* blockedBy(StudioModule module) const { return reasons_.at(static_cast<std::size_t>(module)); }
    void writeScore(std::ostream& out) const {
        bool heading = false;
        for (std::size_t i = 0; i < studioModuleCount; ++i) {
            if (selection_.choices[i] == ModuleChoice::Automatic) continue;
            if (!heading) { out << "\nESCOLHA DE MODULOS -- habilitacao, nao garantia de audibilidade\n"; heading = true; }
            out << "  " << studioModuleIds[i] << " / " << moduleChoiceName(selection_.choices[i])
                << " / " << (enabled_[i] ? "enabled" : "disabled");
            if (*reasons_[i]) out << " / dependency: " << reasons_[i];
            out << '\n';
        }
    }
private:
    ModuleSelection selection_;
    std::array<bool, studioModuleCount> enabled_{};
    std::array<const char*, studioModuleCount> reasons_{};
};
} // namespace rasgo
