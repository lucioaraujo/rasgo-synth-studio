// Opt-in bounded integration: fresh root, serial runs, <=24s piece/30s album.
// Caller must provide a timeout/memory limit. Never added to routine CTest.
#include "../engine/AlbumRenderer.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <numeric>

#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif
namespace fs = std::filesystem;
using namespace rasgo;
static PieceConfig config(unsigned seed, const fs::path& root) {
    PieceConfig c;
    c.seed = seed; c.bpm = 120; c.targetDurationSeconds = 8; c.maxDurationSeconds = 12;
    c.recordOutput = false; c.outDir = root.string(); c.probabilityMarketOverride = true;
    c.expressiveActuationEnabled = true; c.musicalDirectionEnabled = true;
    c.moduleSelection.set(StudioModule::Drums, ModuleChoice::Include);
    c.moduleSelection.set(StudioModule::Kick, ModuleChoice::Include);
    c.moduleSelection.set(StudioModule::Snare, ModuleChoice::Include);
    c.moduleSelection.set(StudioModule::HiHat, ModuleChoice::Include);
    c.moduleSelection.set(StudioModule::Acid, ModuleChoice::Exclude);
    c.moduleSelection.set(StudioModule::Tuba, ModuleChoice::Exclude);
    return c;
}
static void finite(const PieceResult& p) {
    assert(!p.samples.empty());
    for (auto s : p.samples) assert(std::isfinite(s) && std::abs(s) <= 1);
}
static void piece(const fs::path& root) {
    for (unsigned seed : {2026u, 2758870939u, 17u}) {
        auto cfg = config(seed, root / std::to_string(seed));
        auto active = renderIntegratedPiece(cfg); finite(active);
        assert(active.info.grooveDialogue && active.info.drumsEnabled);
        assert(active.info.grooveSubdivisions);
        assert(std::accumulate(active.info.grooveDialogueBars.begin(),
            active.info.grooveDialogueBars.end(), std::uint64_t{0}) > 0);
        assert(std::accumulate(active.info.grooveDialogueHitPlans.begin(),
            active.info.grooveDialogueHitPlans.end(), std::uint64_t{0}) > 0);
        assert(active.info.grooveDialogueBassImprints > 0);
        assert(active.info.drumsPeak > 1e-5);
        auto again = renderIntegratedPiece(cfg);
        assert(again.samples == active.samples && !fs::exists(cfg.outDir));
        again.samples.clear(); again.samples.shrink_to_fit();
        cfg.grooveDialogueEnabled = false;
        auto off = renderIntegratedPiece(cfg); finite(off);
        assert(!off.info.grooveDialogue);
        assert(active.samples != off.samples);
        double delta = 0;
        const auto commonSamples = std::min(off.samples.size(), active.samples.size());
        for (std::size_t i = 0; i < commonSamples; ++i) {
            const double d = active.samples[i] - off.samples[i]; delta += d*d;
        }
        const double rms = std::sqrt(delta / commonSamples); assert(rms > 1e-5);
        std::cout << "seed=" << seed << " hits=" << active.info.grooveDialogueHitPlans[0]
                  << '/' << active.info.grooveDialogueHitPlans[1] << '/'
                  << active.info.grooveDialogueHitPlans[2] << " bassImprints="
                  << active.info.grooveDialogueBassImprints << " differenceRms=" << rms
                  << " framesActive/Off=" << active.samples.size()/2 << '/'
                  << off.samples.size()/2 << '\n';
    }
}
static void bypass(const fs::path& root) {
    for (unsigned mode = 0; mode < 2; ++mode) {
        auto c = config(2026, root / std::to_string(mode));
        c.targetDurationSeconds = 3; c.maxDurationSeconds = 4;
        if (mode == 0) c.musicalDirectionEnabled = false;
        else c.expressiveActuationEnabled = false;
        auto a = renderIntegratedPiece(c);
        c.grooveDialogueEnabled = false;
        auto b = renderIntegratedPiece(c);
        assert(!a.info.grooveDialogue && a.samples == b.samples && !fs::exists(c.outDir));
    }
    std::cout << "Disabled Direction/Expressive: exact PCM bypass\n";
}
static void album(const fs::path& root) {
    AlbumConfig c;
    c.albumSeed = 2026; c.trackCount = 2; c.maxDurationSeconds = 16;
    c.outDir = (root / "album").string(); c.expressiveActuationEnabled = true;
    c.options.musicalDirectionEnabled = true; c.options.targetDurationSeconds = 30;
    c.options.varyBpm = true; c.moduleSelection = config(2026, root).moduleSelection;
    unsigned calibrated = 0;
    std::array<std::size_t, 2> frames{};
    auto tracks = renderAlbum(c, [&](int t, int, const PieceProgress& p, bool calibration) {
        if (calibration && p.phase == PieceRenderPhase::Complete) {
            assert(!fs::exists(c.outDir)); ++calibrated;
            frames.at(static_cast<unsigned>(t)) = p.completedFrames;
        }
    });
    assert(tracks.size() == 2 && calibrated == 2);
    for (unsigned i = 0; i < tracks.size(); ++i) {
        const auto& p = tracks[i].piece; finite(p);
        assert(p.info.grooveDialogue && !p.path.empty());
        assert(frames[i] == p.samples.size() / 2);
    }
    const auto pc = detail::makeTrackConfig(c, c.outDir, 2026, 0, {}, .5);
    assert(pc.grooveDialogueEnabled && pc.grooveSubdivisionsEnabled);
    c.grooveSubdivisionsEnabled = false;
    assert(!detail::makeTrackConfig(c, c.outDir, 2026, 0, {}, .5).grooveSubdivisionsEnabled);
    c.grooveDialogueEnabled = false;
    assert(!detail::makeTrackConfig(c, c.outDir, 2026, 0, {}, .5).grooveDialogueEnabled);
    std::cout << "Album: 2 tracks, calibration/final frames equal\n";
}
static void subdivisions(const fs::path& root) {
    // A/B isolates the new response layer, not all groove, using frozen inputs.
    // Per-seed zero is valid; the complete cohort MUST exercise extra attacks.
    // Otherwise it fails coverage rather than certifying an inert integration.
    std::uint64_t exercised = 0;
    for (unsigned seed : {1u, 3u, 4u}) {
        auto c = config(seed, root / std::to_string(seed));
        // The initial 12s cohort never reached an eligible answer. A bounded
        // 24s window exercises seed 4 naturally; do not force engine onsets.
        c.targetDurationSeconds = 24; c.maxDurationSeconds = 24;
        const auto active = renderIntegratedPiece(c); finite(active);
        const auto again = renderIntegratedPiece(c);
        assert(active.samples == again.samples);
        assert(active.info.grooveSubdivisionExtraPlans == again.info.grooveSubdivisionExtraPlans);
        assert(active.info.grooveSubdivisionExtraTriggers == again.info.grooveSubdivisionExtraTriggers);
        c.grooveSubdivisionsEnabled = false;
        const auto off = renderIntegratedPiece(c); finite(off);
        assert(active.info.grooveSubdivisions && !off.info.grooveSubdivisions);
        assert(std::accumulate(off.info.grooveSubdivisionExtraTriggers.begin(),
            off.info.grooveSubdivisionExtraTriggers.end(), std::uint64_t{0}) == 0);
        const auto actual = std::accumulate(active.info.grooveSubdivisionExtraTriggers.begin(),
            active.info.grooveSubdivisionExtraTriggers.end(), std::uint64_t{0});
        const auto plans = std::accumulate(active.info.grooveSubdivisionExtraPlans.begin(),
            active.info.grooveSubdivisionExtraPlans.end(), std::uint64_t{0});
        assert(actual <= plans && active.info.grooveSubdivisionExtraTriggers[0] == 0);
        for (unsigned voice = 0; voice < 3; ++voice)
            assert(active.info.grooveSubdivisionExtraTriggers[voice]
                <= active.info.grooveSubdivisionExtraPlans[voice]);
        double difference = 0;
        const auto common = std::min(active.samples.size(), off.samples.size());
        assert(common > 0);
        for (std::size_t i = 0; i < common; ++i) {
            const double delta = active.samples[i] - off.samples[i];
            difference += delta * delta;
        }
        const double differenceRms = std::sqrt(difference / common);
        if (actual > 0) assert(active.samples != off.samples && differenceRms > 1e-7);
        else assert(active.samples == off.samples);
        exercised += actual;
        assert(!fs::exists(c.outDir));
        std::cout << "Subdivision seed=" << seed << ": extra triggers=" << actual
                  << "; extra plans=" << plans << "; differenceRms=" << differenceRms
                  << "; PCM changed=" << (active.samples != off.samples) << '\n';
    }
    if (!exercised) std::cerr << "Coverage missing: no subdivision triggers in this cohort\n";
    assert(exercised > 0);
    auto c = config(1, root / "excluded");
    c.targetDurationSeconds = 8; c.maxDurationSeconds = 8;
    c.moduleSelection.set(StudioModule::Snare, ModuleChoice::Exclude);
    c.moduleSelection.set(StudioModule::HiHat, ModuleChoice::Exclude);
    const auto excluded = renderIntegratedPiece(c);
    c.grooveSubdivisionsEnabled = false;
    const auto excludedOff = renderIntegratedPiece(c);
    assert(excluded.samples == excludedOff.samples);
    assert((excluded.info.grooveSubdivisionExtraTriggers == std::array<std::uint64_t, 3>{}));
    assert(!fs::exists(c.outDir));
}
static void probe(const fs::path& root) {
    // Read-only diagnostic cohort: no recordings, no forced onsets/density.
    // Zero is information here, not an acceptance result. The subdivisions
    // test must separately exercise actual triggers and an isolated PCM A/B.
    for (unsigned seed : {2026u, 2758870939u, 17u, 1u, 3u, 4u}) {
        auto c = config(seed, root / std::to_string(seed));
        c.targetDurationSeconds = 24; c.maxDurationSeconds = 24;
        const auto p = renderIntegratedPiece(c); finite(p);
        const auto total = [](const auto& counts) {
            return std::accumulate(counts.begin(), counts.end(), std::uint64_t{0});
        };
        std::cout << "Probe seed=" << seed << ": groove bars=" << total(p.info.grooveDialogueBars)
                  << "; extra plans=" << total(p.info.grooveSubdivisionExtraPlans)
                  << "; extra triggers=" << total(p.info.grooveSubdivisionExtraTriggers)
                  << "; frames=" << p.samples.size()/2 << '\n';
        assert(!fs::exists(c.outDir));
    }
}
int main(int argc, char** argv) {
    assert(argc == 3);
    const fs::path root = argv[1]; assert(!fs::exists(root));
    const std::string mode = argv[2];
    if (mode == "piece") piece(root);
    else if (mode == "bypass") bypass(root);
    else if (mode == "subdivisions") subdivisions(root);
    else if (mode == "probe") probe(root);
    else { assert(mode == "album"); album(root); }
    std::cout << "Groove integration passed: " << mode << '\n';
}
