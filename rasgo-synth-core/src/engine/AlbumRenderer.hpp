#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <functional>
#include <fstream>
#include <random>
#include <string>
#include <stdexcept>
#include <vector>

#include "PieceRenderer.hpp"
#include "AlbumPlan.hpp"
#include "../io/Utf8PathComponent.hpp"
#include "../io/PublishedText.hpp"

namespace rasgo {

// Phase 18: "one call generates a coherent set of tracks" rather than N
// separate calls to renderIntegratedPiece with random seeds that happen to
// land in the same folder.
//
// Fase 83: album cohesion no longer comes from a shared scale -- each
// track picks its own scale as freely as a standalone piece would (see the
// removed album-track exception in PieceRenderer.hpp). Sharing one forced
// tonal center was the original mechanism (phase 18), but with the
// Caillois game weights (fase 80) giving every piece its own bespoke
// Alea/Agon/Ilinx/Mimicry blend already, a fixed key read as redundant
// cohesion layered on top of a richer one. An album now instead walks a
// straight-line trajectory across that same 4-simplex: track 0 starts at
// one seed-derived point, the last track ends at another, and every track
// in between sits at its own interpolated point along that line (still
// summing to 1, since a linear interpolation of two simplex points is
// itself on the simplex) -- see rollGameWeightTrajectory()/
// gameWeightAtFraction() below. That trajectory, not a shared key, is what
// makes a set of tracks read as one body of work now.
//
// A named AlbumTheme is a curated starting point for bpm and an energyBias
// (see GenerativeArc.hpp) that leans the whole set toward a mood -- not a
// semantic understanding of the theme's concept (the seed doesn't "know"
// what mourning means), but a deliberate narrowing of the same tempo/
// intensity choices a human composer would make before even starting to
// write. More themes are just more entries in the array below.
struct AlbumTheme {
    const char *name;
    int rootMidiNote;
    double bpm;
    double energyBias;
};

inline const std::vector<AlbumTheme> &albumThemes() {
    static const std::vector<AlbumTheme> themes = {
        // The seed decides everything, same intensity range a standalone
        // piece would get -- this is the default, not a "mood."
        {"Livre", 57, 100.0, 1.0},
        // Lower register (root note down near the bottom of the sequencer's
        // comfortable range), slower tempo, and a energy ceiling turned
        // down so climaxes stay restrained across the whole set rather
        // than any one track breaking into something bright or forceful.
        // Named "Contido" (restrained) -- purely descriptive of the numeric
        // shape above, not a narrative label. The original phase-18 example
        // used a specific theme name illustratively; the user later
        // clarified that was just an example, not a permanent decision, so
        // this entry was renamed to something that describes the parameter
        // choice itself rather than implying subject matter the seed has no
        // actual understanding of.
        {"Contido", 45, 66.0, 0.6},
        // Brighter register, faster tempo, and an energy ceiling nudged
        // above 1.0 (GenerativeArc still clamps the final result -- this
        // just biases the whole set to lean into its climaxes a little
        // more readily than "Livre" does by default). Used to also force a
        // major scale before fase 83; each track now picks its own.
        {"Luminoso", 60, 118.0, 1.15},
        // Mid-low register, moderate tempo, and the highest energy ceiling
        // of the four -- a set that leans toward intensity rather than
        // restraint. Used to also force Byzantine (double-harmonic) scale
        // before fase 83; each track now picks its own.
        {"Denso", 52, 92.0, 1.4},
    };
    return themes;
}

inline const AlbumTheme *findAlbumTheme(const std::string &name) {
    for (auto &t : albumThemes())
        if (name == t.name) return &t;
    return nullptr;
}

struct AlbumConfig {
    unsigned albumSeed = 2026u;
    int trackCount = 8;
    std::string outDir = "recordings";
    double bpm = 100.0;
    int rootMidiNote = 57;
    double maxDurationSeconds = 240.0;
    double energyBias = 1.0;
    std::string themeName = "Livre";
    AlbumOptions options;
    ModuleSelection moduleSelection;
    bool musicalRelationsEnabled = true;
    bool bassPhraseTimbreEnabled = true;
    bool bassExecutionFeedbackEnabled = true;
    bool bassKickInteractionEnabled = true;
    bool grooveDialogueEnabled = true;
    bool grooveSubdivisionsEnabled = true;
    bool expressiveActuationEnabled = false;
    bool organismStemAuditEnabled = false;

    // What the user actually wants to see on each track's own title, as
    // opposed to themeName above (which selects a musical preset, not
    // necessarily what someone wants their files called). Empty falls
    // back to themeName itself, so this works sensibly with nothing
    // typed in. Track titles are always "{albumTitle} {trackIndex+1}" --
    // the numbering isn't optional the way a standalone piece's title is,
    // an album's tracks need to stay ordered.
    std::string albumTitleOverride;
    std::string artistOverride;
};

inline AlbumConfig albumConfigFromTheme(const AlbumTheme &theme, unsigned albumSeed, int trackCount, std::string outDir) {
    AlbumConfig cfg;
    cfg.albumSeed = albumSeed;
    cfg.trackCount = trackCount;
    cfg.outDir = std::move(outDir);
    cfg.rootMidiNote = theme.rootMidiNote;
    cfg.bpm = theme.bpm;
    cfg.energyBias = theme.energyBias;
    cfg.themeName = theme.name;
    return cfg;
}

struct AlbumTrackResult {
    unsigned trackSeed;
    PieceResult piece;
};

// The album makes two real passes: calibration measures each track's RMS,
// then the final pass writes the trimmed result. Expose the pass explicitly so
// a GUI never appears frozen during calibration.
using AlbumTrackProgressCallback = std::function<void(int trackIndex, int trackCount,
                                                       const PieceProgress &, bool calibrationPass)>;

namespace detail {

// Sanitizes a display name into something safe to use as a single
// directory-name component -- anything that isn't alphanumeric/space/
// hyphen collapses to an underscore, since album titles are free text the
// user typed, not something already guaranteed filesystem-safe.
inline std::string sanitizeForPath(const std::string &s) {
    return utf8PathComponent(s);
}

inline std::string timestampForDir() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::ostringstream os;
    os << std::put_time(std::localtime(&tt), "%Y%m%d_%H%M%S");
    const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() % 1000000;
    os << '_' << std::setw(6) << std::setfill('0') << micros;
    return os.str();
}

// Fase 83: a point on the Caillois 4-simplex, the same stick-breaking
// construction PieceRenderer.hpp uses per-piece (see gameCuts there) --
// duplicated rather than shared since it's three lines and pulling in a
// header just for this would be more coupling than the reuse is worth.
struct GameWeightPoint { double alea, agon, ilinx, mimicry; };

inline GameWeightPoint rollSimplexPoint(std::mt19937 &rng) {
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    double cuts[3] = { unit(rng), unit(rng), unit(rng) };
    std::sort(cuts, cuts + 3);
    return { cuts[0], cuts[1] - cuts[0], cuts[2] - cuts[1], 1.0 - cuts[2] };
}

// Linear interpolation between two simplex points stays on the simplex
// (each component is a convex combination of two values that themselves
// sum to 1, so the result still sums to 1) -- no renormalization needed.
inline GameWeightPoint lerpSimplex(const GameWeightPoint &a, const GameWeightPoint &b, double t) {
    return {
        a.alea + (b.alea - a.alea) * t,
        a.agon + (b.agon - a.agon) * t,
        a.ilinx + (b.ilinx - a.ilinx) * t,
        a.mimicry + (b.mimicry - a.mimicry) * t,
    };
}

inline PieceConfig makeTrackConfig(const AlbumConfig &cfg, const std::string &resolvedOutDir, unsigned trackSeed,
                                    int trackIndex, const GameWeightPoint &gameWeights, double bluffTruthfulness,
                                    const AlbumTrackPlan* planned = nullptr) {
    PieceConfig pc;
    pc.expressiveActuationEnabled = cfg.expressiveActuationEnabled;
    pc.organismStemAuditEnabled = cfg.organismStemAuditEnabled;
    pc.musicalDirectionEnabled = cfg.options.musicalDirectionEnabled;
    pc.musicalRelationsEnabled = cfg.musicalRelationsEnabled;
    pc.bassPhraseTimbreEnabled = cfg.bassPhraseTimbreEnabled;
    pc.bassExecutionFeedbackEnabled = cfg.bassExecutionFeedbackEnabled;
    pc.bassKickInteractionEnabled = cfg.bassKickInteractionEnabled;
    pc.grooveDialogueEnabled = cfg.grooveDialogueEnabled;
    pc.grooveSubdivisionsEnabled = cfg.grooveSubdivisionsEnabled;
    pc.moduleSelection = cfg.moduleSelection;
    pc.seed = trackSeed;
    pc.outDir = resolvedOutDir;
    pc.bpm = cfg.bpm;
    if (planned) {
        pc.bpm = planned->bpm;
        pc.targetDurationSeconds = planned->targetDurationSeconds;
    }
    pc.rootMidiNote = cfg.rootMidiNote;
    pc.maxDurationSeconds = cfg.maxDurationSeconds;
    pc.energyBias = cfg.energyBias;
    pc.albumId = static_cast<long long>(cfg.albumSeed);
    pc.albumTrackIndex = trackIndex;
    pc.albumTrackCount = cfg.trackCount;
    pc.albumTheme = cfg.themeName;
    pc.gameWeightOverride = true;
    pc.gameWeightAleaOverride = gameWeights.alea;
    pc.gameWeightAgonOverride = gameWeights.agon;
    pc.gameWeightIlinxOverride = gameWeights.ilinx;
    pc.gameWeightMimicryOverride = gameWeights.mimicry;
    // Fase 115 -- jogo aplicado alem do motor: o mesmo idioma da
    // trajetoria de pesos de jogo acima, agora pra taxa de veracidade do
    // sinal de blefe (BluffSignal.hpp) -- um album pode ter seu proprio
    // arco real de confiabilidade cruzando as faixas, nao so um blend de
    // jogo compartilhado.
    pc.bluffTruthfulnessOverride = true;
    pc.bluffTruthfulnessOverrideValue = bluffTruthfulness;
    std::string albumDisplayName = !cfg.albumTitleOverride.empty() ? cfg.albumTitleOverride : cfg.themeName;
    pc.titleOverride = albumDisplayName + " " + std::to_string(trackIndex + 1);
    pc.artistOverride = cfg.artistOverride;
    return pc;
}

inline double rms(const std::vector<float> &samples) {
    if (samples.empty()) return 0.0;
    double sum = 0.0;
    for (float s : samples) sum += static_cast<double>(s) * static_cast<double>(s);
    return std::sqrt(sum / static_cast<double>(samples.size()));
}
} // namespace detail

// Renders cfg.trackCount tracks. Each track's seed is drawn from a single
// std::mt19937 stream seeded with albumSeed, so the same album seed always
// regenerates the same set of track seeds (and, engine version held
// constant, the same actual tracks) -- an album is reproducible the same
// way a single piece already was.
//
// Phase 19 mastering pass: a gentle *relative* loudness alignment between
// tracks, not normalizing every track to identical loudness (which would
// flatten the very swing between a hushed piece and an intense one that's
// the point of this engine). Each track is rendered twice: a first "probe"
// pass in memory to measure its RMS (no disposable recording -- same seed, so the
// second pass is musically identical, just with the right gain trim
// applied), then a second pass with PieceConfig::outputGainTrim set to
// pull the track only *partway* (a sqrt-scaled correction, i.e. roughly
// half the full dB correction in gain-ratio terms) toward the album's own
// median loudness, clamped to a modest +-3dB-ish range either way.
inline std::vector<AlbumTrackResult> renderAlbum(const AlbumConfig &cfg,
                                                  const AlbumTrackProgressCallback &onTrackProgress = nullptr) {
    std::mt19937 albumRng(cfg.albumSeed);
    const auto plan = makeAlbumPlan(cfg.albumSeed, cfg.trackCount, cfg.bpm, cfg.options);
    std::vector<unsigned> trackSeeds(static_cast<size_t>(cfg.trackCount));
    for (auto &s : trackSeeds) s = albumRng();

    // Fase 83: the album's own cohesion mechanism -- a straight-line walk
    // across the Caillois 4-simplex from one seed-derived point to
    // another, one interpolated point per track (see the header comment
    // above for why this replaces the old shared-scale mechanism). A
    // single-track album has nowhere to walk to, so it just sits at the
    // trajectory's start point.
    detail::GameWeightPoint trajectoryStart = detail::rollSimplexPoint(albumRng);
    detail::GameWeightPoint trajectoryEnd = detail::rollSimplexPoint(albumRng);
    std::vector<detail::GameWeightPoint> trackGameWeights(static_cast<size_t>(cfg.trackCount));
    for (int t = 0; t < cfg.trackCount; ++t) {
        double frac = cfg.trackCount > 1 ? (double)t / (double)(cfg.trackCount - 1) : 0.0;
        trackGameWeights[static_cast<size_t>(t)] = detail::lerpSimplex(trajectoryStart, trajectoryEnd, frac);
    }

    // Fase 115 -- trajetoria real de veracidade do sinal de blefe cruzando
    // o album, mesmo idioma da trajetoria de pesos de jogo acima -- um
    // ponto inicial e um final sorteados independentemente (mesmo range
    // 0.25-0.95 que uma peca avulsa rolaria sozinha), cada faixa pousando
    // no ponto interpolado correspondente. O album pode assim comecar
    // confiavel e terminar blefando descaradamente, ou o inverso, ou
    // qualquer trajetoria intermediaria -- um arco real de confiabilidade,
    // nao um numero fixo repetido em toda faixa.
    std::uniform_real_distribution<double> truthUnit(0.25, 0.95);
    double truthTrajectoryStart = truthUnit(albumRng);
    double truthTrajectoryEnd = truthUnit(albumRng);
    std::vector<double> trackTruthfulness(static_cast<size_t>(cfg.trackCount));
    for (int t = 0; t < cfg.trackCount; ++t) {
        double frac = cfg.trackCount > 1 ? (double)t / (double)(cfg.trackCount - 1) : 0.0;
        trackTruthfulness[static_cast<size_t>(t)] = truthTrajectoryStart + (truthTrajectoryEnd - truthTrajectoryStart) * frac;
    }

    // Every new album render gets its own subdirectory instead of every
    // track from every album ever rendered landing flat in the same
    // folder -- named after whatever the user would actually recognise
    // this album as (their own title if they typed one, the theme preset
    // name otherwise) plus a timestamp, so re-rendering the same seed/
    // theme combination later doesn't merge into an earlier session's
    // folder by accident.
    std::string albumDisplayName = !cfg.albumTitleOverride.empty() ? cfg.albumTitleOverride : cfg.themeName;
    std::string resolvedOutDir = cfg.outDir + "/" + detail::sanitizeForPath(albumDisplayName)
                                + "_" + detail::timestampForDir();

    std::vector<double> trackRms(static_cast<size_t>(cfg.trackCount));
    std::vector<ProbabilityMarket> trackMarkets(static_cast<size_t>(cfg.trackCount));
    ProbabilityMarket nextMarket;
    // An album uses a new output directory. Freeze each track's input while
    // advancing the existing market in memory, once per track, not twice.
    nextMarket.load(resolvedOutDir + "/.rasgo_market.txt");
    for (int t = 0; t < cfg.trackCount; ++t) {
        PieceConfig pc = detail::makeTrackConfig(cfg, resolvedOutDir, trackSeeds[static_cast<size_t>(t)], t,
                                                  trackGameWeights[static_cast<size_t>(t)], trackTruthfulness[static_cast<size_t>(t)],
                                                  &plan.tracks[static_cast<size_t>(t)]);
        pc.recordOutput = false;
        trackMarkets[static_cast<size_t>(t)] = nextMarket.continuation();
        pc.probabilityMarketOverride = true;
        pc.probabilityMarketInput = trackMarkets[static_cast<size_t>(t)];
        ProgressCallback probeProgress = nullptr;
        if (onTrackProgress) {
            probeProgress = [&onTrackProgress, t, &cfg](const PieceProgress &p) {
                onTrackProgress(t, cfg.trackCount, p, true);
            };
        }
        PieceResult probe = renderIntegratedPiece(pc, probeProgress);
        trackRms[static_cast<size_t>(t)] = detail::rms(probe.samples);
        nextMarket = std::move(probe.nextProbabilityMarket);
    }

    std::vector<double> sortedRms = trackRms;
    std::sort(sortedRms.begin(), sortedRms.end());
    double medianRms = sortedRms[sortedRms.size() / 2];

    std::vector<AlbumTrackResult> results;
    results.reserve(static_cast<size_t>(cfg.trackCount));
    for (int t = 0; t < cfg.trackCount; ++t) {
        double trim = 1.0;
        double thisRms = trackRms[static_cast<size_t>(t)];
        if (thisRms > 1e-6 && medianRms > 1e-6) {
            trim = std::clamp(std::sqrt(medianRms / thisRms), 0.7, 1.4);
        }

        PieceConfig pc = detail::makeTrackConfig(cfg, resolvedOutDir, trackSeeds[static_cast<size_t>(t)], t,
                                                  trackGameWeights[static_cast<size_t>(t)], trackTruthfulness[static_cast<size_t>(t)],
                                                  &plan.tracks[static_cast<size_t>(t)]);
        pc.outputGainTrim = trim;
        pc.probabilityMarketOverride = true;
        pc.probabilityMarketInput = trackMarkets[static_cast<size_t>(t)];

        ProgressCallback trackProgress = nullptr;
        if (onTrackProgress) {
            trackProgress = [&onTrackProgress, t, &cfg](const PieceProgress &p) {
                onTrackProgress(t, cfg.trackCount, p, false);
            };
        }

        PieceResult piece = renderIntegratedPiece(pc, trackProgress);
        results.push_back({trackSeeds[static_cast<size_t>(t)], std::move(piece)});
    }
    // The output directory has been created by the existing recorder. The
    // manifest records planned vs actual duration; it is not an exact-time promise.
    if (!results.empty() && std::all_of(results.begin(), results.end(),
            [](const auto& track) { return !track.piece.path.empty(); })) {
        std::ostringstream manifest;
        if (manifest) {
            manifest << "{";
            cfg.moduleSelection.writeJson(manifest);
            manifest << std::setprecision(10) << "\"schemaVersion\":1,\"albumSeed\":" << cfg.albumSeed
                     << ",\"varyBpm\":" << (cfg.options.varyBpm ? "true" : "false")
                     << ",\"musicalDirection\":" << (cfg.options.musicalDirectionEnabled ? "true" : "false")
                     << ",\"musicalRelations\":" << (cfg.options.musicalDirectionEnabled && cfg.musicalRelationsEnabled ? "true" : "false")
                     << ",\"expressiveActuation\":" << (cfg.expressiveActuationEnabled ? "true" : "false")
                     << ",\"grooveDialogue\":" << (cfg.expressiveActuationEnabled
                         && cfg.options.musicalDirectionEnabled && cfg.grooveDialogueEnabled ? "true" : "false")
                     << ",\"grooveSubdivisions\":" << (cfg.expressiveActuationEnabled
                         && cfg.options.musicalDirectionEnabled && cfg.grooveDialogueEnabled
                         && cfg.grooveSubdivisionsEnabled ? "true" : "false")
                     << ",\"bassPhraseTimbre\":" << (cfg.expressiveActuationEnabled
                         && cfg.options.musicalDirectionEnabled && cfg.musicalRelationsEnabled
                         && cfg.bassPhraseTimbreEnabled ? "true" : "false")
                     << ",\"organismStemAudit\":" << (cfg.organismStemAuditEnabled ? "true" : "false")
                     << ",\"bassExecutionFeedback\":" << (cfg.expressiveActuationEnabled
                         && cfg.options.musicalDirectionEnabled
                         && cfg.bassExecutionFeedbackEnabled ? "true" : "false")
                     << ",\"bassKickInteraction\":" << (cfg.expressiveActuationEnabled
                         && cfg.options.musicalDirectionEnabled
                         && cfg.bassKickInteractionEnabled ? "true" : "false")
                     << ",\"baseBpm\":" << cfg.bpm
                     << ",\"bpmSpread\":" << cfg.options.bpmSpread
                     << ",\"tempoContour\":" << static_cast<int>(cfg.options.tempoContour)
                     << ",\"durationDistribution\":" << static_cast<int>(cfg.options.durationDistribution)
                     << ",\"requestedSeconds\":" << plan.requestedDurationSeconds
                     << ",\"plannedSeconds\":" << plan.effectiveDurationSeconds << ",\"tracks\":[";
            for (std::size_t index = 0; index < results.size(); ++index) {
                if (index) manifest << ',';
                const auto& piece = results[index].piece;
                manifest << "{\"index\":" << index + 1 << ",\"seed\":" << results[index].trackSeed
                         << ",\"bpm\":" << plan.tracks[index].bpm
                         << ",\"targetSeconds\":" << plan.tracks[index].targetDurationSeconds
                         << ",\"actualSeconds\":" << (piece.samples.size() / 2.0 / piece.sampleRate)
                         << ",\"file\":\"" << piece.path.substr(piece.path.find_last_of("/\\") + 1) << "\"}";
            }
            manifest << "]}\n";
            if (!manifest.good() || !publishTextNoReplace(
                    std::filesystem::u8path(resolvedOutDir + "/album_plan.json"), manifest.str())) {
                // Completed WAVs/sidecars remain recoverable. The app's existing
                // render-session failure path must not announce album success.
                throw std::runtime_error("Album manifest publication failed; completed tracks preserved in: "
                    + resolvedOutDir);
            }
        }
    }
    return results;
}

} // namespace rasgo
