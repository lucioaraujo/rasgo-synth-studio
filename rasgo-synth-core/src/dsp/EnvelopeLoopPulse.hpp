#pragma once
#include "Envelope.hpp"
#include "Oscillator.hpp"

namespace rasgo {

// Polivoks EG "loop" + "end of attack" functions (see Envelope.hpp),
// exercised together as one real, audible mechanism: loop mode turns the
// envelope into a self-cycling pulse source that never needs an external
// note-on, and every time it completes an attack -- its own dedicated
// end-of-attack output -- a short sine "blip" is struck at the piece's
// current root pitch. A real modular idiom (patching an EG's own end-of-
// cycle output back into something else's trigger input, or its own gate)
// that nothing else in this engine does -- every other self-triggering
// mechanism here fires from explicit note/step logic, not from an
// envelope retriggering itself.
class EnvelopeLoopPulse {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        env_.setSampleRate(sr);
        env_.setLoopEnabled(true);
        osc_.setSampleRate(sr);
        osc_.setWaveform(Oscillator::Waveform::Sine);
    }

    // sustainLevel is always 0 here on purpose -- Decay must reach zero
    // for the loop's own hand-back to Attack to read as a real percussive
    // pulse rather than a held plateau.
    void configure(double attackSeconds, double decaySeconds) {
        env_.setADSR(attackSeconds, decaySeconds, 0.0, 0.05);
        env_.noteOn(); // arms the self-cycling loop once; it never stops on its own
    }

    void setFrequency(double hz) { osc_.setFrequency(hz); }

    // True for exactly the one sample the loop's attack completes -- lets
    // a caller count real disparos for empirical auditing, same
    // convention used throughout this engine's rarer mechanisms.
    bool justPulsed() const { return env_.endOfAttack(); }

    float tick() {
        float level = env_.tick();
        // level^2: a fast-attack pulse should read as a percussive strike,
        // not a linear fade -- the same curve shape KickVoice/SnareVoice's
        // own decay envelopes already lean on perceptually.
        return osc_.tick() * level * level;
    }

private:
    double sampleRate_ = 44100.0;
    Envelope env_;
    Oscillator osc_;
};

} // namespace rasgo
