#pragma once

#include <array>
#include <cstdint>

#include "../sequencer/Scales.hpp"

namespace rasgo {

// Phase 118: a seed first chooses a small number of perceptually meaningful
// decisions.  Optional modules are then allowed to elaborate that identity;
// they no longer have to manufacture identity by accumulating unrelated
// Bernoulli rolls on top of one invariant patch.
enum class MaterialFocus { Harmonic, Rhythmic, Spectral, Textural, Circuit, Fragmented };
enum class ScaleFamily { Pentatonic, Diatonic, Altered, Symmetric, BluesBebop, Chromatic };
enum class CoreVoiceModel { Subtractive, Rounded, Pulse, Inharmonic, Minimal };
enum class SequenceRhythmModel { Euclidean, Stochastic, Clustered, Irregular };
enum class LeadVoiceModel { Core, FmFlute, Theremin, Tuba, Ensemble };
enum class HarmonicLanguage { Modal, Functional, Jazz, Chromatic, Drone };
enum class EnsembleModel { Solo, CallResponse, Counterpoint, Polyphonic };

struct SpectralPlan {
    bool generatedScale = false;
    bool microtonal = false;
    bool spectralObject = false;
    bool spectralGhost = false;
    bool sievePolyphony = false;

    int count() const {
        return static_cast<int>(generatedScale) + static_cast<int>(microtonal)
             + static_cast<int>(spectralObject) + static_cast<int>(spectralGhost)
             + static_cast<int>(sievePolyphony);
    }
};

struct CompositionIdentity {
    MaterialFocus focus = MaterialFocus::Harmonic;
    ScaleFamily scaleFamily = ScaleFamily::Diatonic;
    CoreVoiceModel coreVoice = CoreVoiceModel::Subtractive;
    SequenceRhythmModel rhythmModel = SequenceRhythmModel::Euclidean;
    LeadVoiceModel leadVoice = LeadVoiceModel::Core;
    HarmonicLanguage harmonicLanguage = HarmonicLanguage::Modal;
    EnsembleModel ensembleModel = EnsembleModel::Solo;
    Scale initialScale = Scale::Dorian;
    int rootMidiNote = 57;
    double scaleBlendProbability = 0.12;
    SpectralPlan spectral;
};

// SplitMix64 finalizer: cheap stable stream separation.  A new decision can
// receive a new stream id without shifting every decision which follows it,
// unlike the single sequential mt19937 stream previously used for the whole
// renderer.
inline std::uint64_t identityBits(unsigned seed, std::uint64_t stream) {
    std::uint64_t z = static_cast<std::uint64_t>(seed)
                    + 0x9e3779b97f4a7c15ULL * (stream + 1ULL);
    z = (z ^ (z >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27U)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31U);
}

inline double identityUnit(unsigned seed, std::uint64_t stream) {
    return static_cast<double>(identityBits(seed, stream) >> 11U)
         * (1.0 / 9007199254740992.0);
}

template <typename T, std::size_t N>
inline T identityChoice(const std::array<T, N> &values, unsigned seed, std::uint64_t stream) {
    return values[static_cast<std::size_t>(identityBits(seed, stream) % N)];
}

inline const char *materialFocusName(MaterialFocus value) {
    static const char *names[] = {"harmonic", "rhythmic", "spectral", "textural", "circuit", "fragmented"};
    return names[static_cast<int>(value)];
}

inline const char *scaleFamilyName(ScaleFamily value) {
    static const char *names[] = {"pentatonic", "diatonic", "altered", "symmetric", "blues-bebop", "chromatic"};
    return names[static_cast<int>(value)];
}

inline const char *coreVoiceModelName(CoreVoiceModel value) {
    static const char *names[] = {"subtractive", "rounded", "pulse", "inharmonic", "minimal"};
    return names[static_cast<int>(value)];
}

inline const char *sequenceRhythmModelName(SequenceRhythmModel value) {
    static const char *names[] = {"euclidean", "stochastic", "clustered", "irregular"};
    return names[static_cast<int>(value)];
}

inline const char *leadVoiceModelName(LeadVoiceModel value) {
    static const char *names[] = {"core", "fm-flute", "theremin", "tuba", "ensemble"};
    return names[static_cast<int>(value)];
}

inline const char *harmonicLanguageName(HarmonicLanguage value) {
    static const char *names[] = {"modal", "functional", "jazz", "chromatic", "drone"};
    return names[static_cast<int>(value)];
}

inline const char *ensembleModelName(EnsembleModel value) {
    static const char *names[] = {"solo", "call-response", "counterpoint", "polyphonic"};
    return names[static_cast<int>(value)];
}

inline Scale chooseScaleInFamily(ScaleFamily family, unsigned seed) {
    static constexpr std::array<Scale, 8> pentatonic = {
        Scale::MinorPentatonic, Scale::MajorPentatonic, Scale::Suspended, Scale::ManGong,
        Scale::Ritusen, Scale::Hirajoshi, Scale::InScale, Scale::Kumoi
    };
    static constexpr std::array<Scale, 7> diatonic = {
        Scale::Major, Scale::Dorian, Scale::Phrygian, Scale::Lydian,
        Scale::Mixolydian, Scale::Aeolian, Scale::Locrian
    };
    static constexpr std::array<Scale, 19> altered = {
        Scale::MelodicMinor, Scale::HarmonicMinor, Scale::DorianB2, Scale::LydianAugmented,
        Scale::LydianDominant, Scale::MixolydianB6, Scale::HalfDiminished, Scale::Altered,
        Scale::LocrianNat6, Scale::IonianAugmented, Scale::DorianSharp4, Scale::PhrygianDominant,
        Scale::LydianSharp2, Scale::Ultralocrian, Scale::Byzantine, Scale::HungarianMinor,
        Scale::HarmonicMajor, Scale::NeapolitanMinor, Scale::MixolydianB9
    };
    static constexpr std::array<Scale, 5> symmetric = {
        Scale::WholeTone, Scale::DiminishedWholeHalf, Scale::DiminishedHalfWhole,
        Scale::AugmentedScale, Scale::Prometheus
    };
    static constexpr std::array<Scale, 5> bluesBebop = {
        Scale::Blues, Scale::Bebop, Scale::HungarianMajor,
        Scale::BebopMajor, Scale::BebopMinor
    };

    switch (family) {
        case ScaleFamily::Pentatonic: return identityChoice(pentatonic, seed, 21);
        case ScaleFamily::Diatonic: return identityChoice(diatonic, seed, 22);
        case ScaleFamily::Altered: return identityChoice(altered, seed, 23);
        case ScaleFamily::Symmetric: return identityChoice(symmetric, seed, 24);
        case ScaleFamily::BluesBebop: return identityChoice(bluesBebop, seed, 25);
        case ScaleFamily::Chromatic: return Scale::Chromatic;
    }
    return Scale::Dorian;
}

inline CompositionIdentity makeCompositionIdentity(unsigned seed) {
    CompositionIdentity out;

    // Unequal family weights are deliberate. The draw is not uniform over the
    // whole fixed catalog: large modal families must not receive more tickets
    // than compact but
    // perceptually distinct families such as symmetric or pentatonic sets.
    double focusRoll = identityUnit(seed, 1);
    if (focusRoll < 0.22) out.focus = MaterialFocus::Harmonic;
    else if (focusRoll < 0.44) out.focus = MaterialFocus::Rhythmic;
    else if (focusRoll < 0.58) out.focus = MaterialFocus::Spectral;
    else if (focusRoll < 0.75) out.focus = MaterialFocus::Textural;
    else if (focusRoll < 0.89) out.focus = MaterialFocus::Circuit;
    else out.focus = MaterialFocus::Fragmented;

    double familyRoll = identityUnit(seed, 2);
    if (familyRoll < 0.21) out.scaleFamily = ScaleFamily::Pentatonic;
    else if (familyRoll < 0.43) out.scaleFamily = ScaleFamily::Diatonic;
    else if (familyRoll < 0.65) out.scaleFamily = ScaleFamily::Altered;
    else if (familyRoll < 0.82) out.scaleFamily = ScaleFamily::Symmetric;
    else if (familyRoll < 0.95) out.scaleFamily = ScaleFamily::BluesBebop;
    else out.scaleFamily = ScaleFamily::Chromatic;
    out.initialScale = chooseScaleInFamily(out.scaleFamily, seed);

    out.coreVoice = static_cast<CoreVoiceModel>(identityBits(seed, 3) % 5ULL);
    out.rhythmModel = static_cast<SequenceRhythmModel>(identityBits(seed, 4) % 4ULL);
    out.leadVoice = static_cast<LeadVoiceModel>(identityBits(seed, 9) % 5ULL);
    out.harmonicLanguage = static_cast<HarmonicLanguage>(identityBits(seed, 10) % 5ULL);
    out.ensembleModel = static_cast<EnsembleModel>(identityBits(seed, 11) % 4ULL);

    // Align high-level choices just enough to create a readable identity,
    // without reducing them to fixed presets. Rhythmic pieces may still
    // have a melodic lead, but an ensemble lead is more common; harmonic
    // pieces have a real extra chance to foreground the flute.
    if (out.focus == MaterialFocus::Rhythmic && identityUnit(seed, 12) < 0.55)
        out.leadVoice = LeadVoiceModel::Ensemble;
    if (out.focus == MaterialFocus::Harmonic && identityUnit(seed, 13) < 0.45)
        out.leadVoice = LeadVoiceModel::FmFlute;
    if (out.focus == MaterialFocus::Circuit && identityUnit(seed, 14) < 0.6)
        out.leadVoice = LeadVoiceModel::Core;
    if (out.leadVoice == LeadVoiceModel::Ensemble && out.ensembleModel == EnsembleModel::Solo)
        out.ensembleModel = identityUnit(seed, 15) < 0.5
            ? EnsembleModel::Counterpoint : EnsembleModel::Polyphonic;

    // Cover all twelve pitch classes and sometimes move the entire piece by
    // an octave.  The conservative 43..67 clamp keeps existing voices in a
    // safe register even after their own +/- octave decisions.
    int root = 48 + static_cast<int>(identityBits(seed, 5) % 12ULL);
    double registerRoll = identityUnit(seed, 6);
    if (registerRoll < 0.12) root -= 12;
    else if (registerRoll > 0.86) root += 12;
    if (root < 43) root = 43;
    if (root > 67) root = 67;
    out.rootMidiNote = root;

    out.scaleBlendProbability = out.focus == MaterialFocus::Textural ? 0.28
                              : out.focus == MaterialFocus::Fragmented ? 0.22
                              : 0.10;

    // A family budget replaces five independent spectral-family rolls.
    // Spectral-focused pieces receive two coordinated techniques; other
    // identities usually receive none and occasionally one.  Consequently
    // spectralism can be a clear identity without becoming the default
    // dialect of nearly every seed.
    int slots = 0;
    if (out.focus == MaterialFocus::Spectral) slots = 2;
    else if (identityUnit(seed, 7) < 0.28) slots = 1;
    if (out.focus == MaterialFocus::Fragmented && identityUnit(seed, 8) < 0.25) ++slots;

    std::array<int, 5> candidates = {0, 1, 2, 3, 4};
    for (int i = 4; i > 0; --i) {
        int j = static_cast<int>(identityBits(seed, 30 + static_cast<std::uint64_t>(i))
                               % static_cast<std::uint64_t>(i + 1));
        int tmp = candidates[static_cast<std::size_t>(i)];
        candidates[static_cast<std::size_t>(i)] = candidates[static_cast<std::size_t>(j)];
        candidates[static_cast<std::size_t>(j)] = tmp;
    }
    for (int i = 0; i < slots && i < 5; ++i) {
        switch (candidates[static_cast<std::size_t>(i)]) {
            case 0: out.spectral.generatedScale = true; break;
            case 1: out.spectral.microtonal = true; break;
            case 2: out.spectral.spectralObject = true; break;
            case 3: out.spectral.spectralGhost = true; break;
            case 4: out.spectral.sievePolyphony = true; break;
        }
    }

    return out;
}

inline double fmFluteProbability(const CompositionIdentity &identity) {
    switch (identity.focus) {
        case MaterialFocus::Harmonic: return 0.58;
        case MaterialFocus::Textural: return 0.22;
        case MaterialFocus::Fragmented: return 0.18;
        case MaterialFocus::Rhythmic: return 0.10;
        case MaterialFocus::Spectral: return 0.08;
        case MaterialFocus::Circuit: return 0.06;
    }
    return 0.12;
}

} // namespace rasgo
