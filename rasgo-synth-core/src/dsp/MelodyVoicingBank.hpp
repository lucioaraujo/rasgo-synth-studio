#pragma once
#include <array>
#include <cmath>
#include <random>
#include "Oscillator.hpp"
#include "Envelope.hpp"
#include "../sequencer/Scales.hpp"
#include "../sequencer/HarmonicWanderer.hpp"

namespace rasgo {

// The melody/flute object's "Eixo B" (task #31) -- how the harmonic
// support UNDER the flute's phrases is built vertically, independent of
// "Eixo A" (which key centre/scale is current) and "Eixo C" (dissonance
// treatment, not yet implemented). Five real, documented voicing
// principles, one seed-picked per piece:
//   Quartal      -- stacked perfect 4ths (McCoy Tyner's real signature
//                   voicing), deliberately ambiguous major/minor quality.
//   Planing      -- a fixed augmented-triad SHAPE moved in rigid parallel
//                   motion with the key centre (Debussy's real "planing"
//                   device, e.g. "Voiles" -- the whole-tone/augmented
//                   shape divides the octave symmetrically, so it carries
//                   no functional identity to begin with, exactly the
//                   point of moving it in parallel rather than
//                   reharmonizing it).
//   Bitonal      -- two simultaneous triads a tritone apart (a real
//                   Stravinsky/Milhaud device -- Milhaud's own "Saudades
//                   do Brasil" already sits adjacent to this project's own
//                   bossa nova reference points).
//   ClassicalTriad -- root/3rd/5th read directly off the CURRENT scale's
//                   own degrees (not a hardcoded major/minor), the control
//                   condition among the five.
//   ArmenianModal -- a real technique brought directly from the user's own
//                   research this session (corrected mid-session: Armenian,
//                   not Albanian): a static drone held at the PIECE'S
//                   ORIGINAL root -- the "dam," the continuous drone a
//                   second duduk plays under a solo melodic line in real
//                   Armenian folk practice, the same drone-under-melody
//                   principle Komitas Vardapet's own systematic collection
//                   of Armenian folk/liturgical (Sharakan) melody is built
//                   on -- topped with a real open 5th of the CURRENT key
//                   centre (never a 3rd, the same incomplete-interval
//                   harmonic signature) plus one quartal colour tone (a
//                   4th/11th above) instead of a plain octave echo -- the
//                   real jazz-modal fusion of that same drone/modal
//                   language with McCoy-Tyner-style quartal voicing,
//                   exactly what Tigran Hamasyan's own real, contemporary
//                   practice sounds like (Armenian modal material read
//                   through modal-jazz harmonic vocabulary, the user's own
//                   named reference point: "armenian jazz modal").
enum class VoicingStyle {
    Quartal,
    Planing,
    Bitonal,
    ClassicalTriad,
    ArmenianModal,
    Count
};

class MelodyVoicingBank {
public:
    static constexpr int kNumVoices = 3;

    void setSampleRate(double sr) {
        sampleRate_ = sr;
        for (auto &osc : oscs_) osc.setSampleRate(sr);
        for (auto &env : envs_) env.setSampleRate(sr);
    }

    void configure(VoicingStyle style, int pieceRootMidiNote, unsigned seed) {
        style_ = style;
        pieceRootMidiNote_ = pieceRootMidiNote;
        rng_.seed(seed);
        for (auto &osc : oscs_) osc.setWaveform(Oscillator::Waveform::Sine);
        for (auto &env : envs_) env.setADSR(0.8, 0.5, 0.85, 1.2);
    }

    void setGain(float g) { gain_ = g; }

    // Called whenever the flute's HarmonicWanderer reaches a new key
    // centre -- re-voices the accompaniment for that key centre and
    // (re)triggers the voices' own slow envelopes, a real ensemble
    // "breath" rather than a hard-cut retune.
    void retune(const KeyCenter &key) {
        std::array<int, kNumVoices> semitoneOffsets = intervalsFor(style_, key.scale);
        int droneRoot = (style_ == VoicingStyle::ArmenianModal) ? pieceRootMidiNote_ : key.rootMidiNote;
        for (int v = 0; v < kNumVoices; ++v) {
            int root = (style_ == VoicingStyle::ArmenianModal && v == 0) ? droneRoot : key.rootMidiNote;
            double hz = midiToFreqLocal(root + semitoneOffsets[static_cast<size_t>(v)]);
            oscs_[static_cast<size_t>(v)].setFrequency(hz);
            envs_[static_cast<size_t>(v)].noteOn();
            lastFrequencies_[static_cast<size_t>(v)] = hz;
        }
    }

    float tick() {
        float sum = 0.0f;
        for (int v = 0; v < kNumVoices; ++v) {
            float e = envs_[static_cast<size_t>(v)].tick();
            sum += oscs_[static_cast<size_t>(v)].tick() * e;
        }
        return sum * gain_ / static_cast<float>(kNumVoices);
    }

    VoicingStyle style() const { return style_; }

    // Exposed mainly for isolated testing (verifying the drone voice
    // genuinely never moves) -- harmless to read in production too.
    double lastVoiceFrequency(int voiceIndex) const { return lastFrequencies_[static_cast<size_t>(voiceIndex)]; }

    static std::string styleName(VoicingStyle s) {
        switch (s) {
            case VoicingStyle::Quartal:        return "Quartal (McCoy Tyner)";
            case VoicingStyle::Planing:        return "Planing paralelo (Debussy)";
            case VoicingStyle::Bitonal:        return "Politonal (Stravinsky/Milhaud)";
            case VoicingStyle::ClassicalTriad: return "Triade classica";
            case VoicingStyle::ArmenianModal:  return "Modal armenio (bordao real + 5a aberta + tom quartal, jazz modal)";
            default: return "";
        }
    }

private:
    static double midiToFreqLocal(int midiNote) {
        return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
    }

    // Real scale-degree lookups for ClassicalTriad -- 3rd and 5th read off
    // whatever scale is actually active (works for 5-note pentatonics as
    // well as 7-note modes), not a hardcoded +4/+7.
    static int scaleDegreeSemitones(Scale scale, int degreeIndex) {
        const ScaleTable &t = scaleTable(scale);
        return t.degrees[static_cast<size_t>(degreeIndex % t.length)];
    }

    std::array<int, kNumVoices> intervalsFor(VoicingStyle style, Scale scale) {
        switch (style) {
            case VoicingStyle::Quartal: {
                // Real McCoy Tyner/Tigran Hamasyan technique: a stack of
                // 4ths has NO 3rd at all, so the same shape reads as
                // ambiguously major or minor depending on what's emphasized
                // above it -- the top note alternates between a real minor
                // 7th (10) and a real major 7th (11) seed-varied per
                // re-voicing, deliberately keeping that maj7/min7 ambiguity
                // alive rather than settling on one quality permanently.
                int seventh = (unit() < 0.5) ? 10 : 11;
                return { 0, 5, seventh };
            }
            case VoicingStyle::Planing:
                return { 0, 4, 8 }; // augmented triad shape, moved rigidly
            case VoicingStyle::Bitonal:
                // Two triads a tritone apart -- only 3 voices available, so
                // one voice from each root: the two roots themselves plus
                // one third above the second root (enough to read as two
                // simultaneous tonal centres, not one big added-note chord).
                return { 0, 6, 10 };
            case VoicingStyle::ClassicalTriad: {
                int third = scaleDegreeSemitones(scale, 2);
                int fifth = scaleDegreeSemitones(scale, 4);
                return { 0, third, fifth };
            }
            case VoicingStyle::ArmenianModal: {
                // Voice 0 is the drone itself (handled separately in
                // retune() via droneRoot -- semitone offset 0 relative to
                // the PIECE's own root, never the wanderer's). Voice 1: a
                // real open 5th of the current key centre (the duduk dam's
                // own interval, never a 3rd). Voice 2: one quartal colour
                // tone above that -- the real jazz-modal fusion move
                // (Tigran Hamasyan's own quartal practice: a stack with no
                // 3rd reads as ambiguously major or minor, seventh
                // seed-varied between real minor 7th and major 7th rather
                // than settling on one quality piece to piece).
                int seventh = (unit() < 0.5) ? 10 : 11;
                return { 0, 7, seventh };
            }
            default:
                return { 0, 0, 0 };
        }
    }

    double unit() { return std::uniform_real_distribution<double>(0.0, 1.0)(rng_); }

    double sampleRate_ = 44100.0;
    VoicingStyle style_ = VoicingStyle::ClassicalTriad;
    int pieceRootMidiNote_ = 57;
    std::array<Oscillator, kNumVoices> oscs_;
    std::array<Envelope, kNumVoices> envs_;
    std::array<double, kNumVoices> lastFrequencies_{};
    std::mt19937 rng_;
    float gain_ = 0.0f;
};

} // namespace rasgo
