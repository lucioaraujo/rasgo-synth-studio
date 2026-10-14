#pragma once
#include <cmath>

namespace rasgo {

// A non-retriggerable monostable ("one-shot"): trigger() starts it, and the
// output stays high for a fixed duration regardless of further triggers
// arriving during that time -- exactly the classic 555/556 monostable
// behavior at the heart of the Atari Punk Console. That "ignore triggers
// while already high" detail is not an implementation shortcut; it's the
// entire mechanism behind the circuit's characteristic sound (see
// AtariPunkConsole below).
class Monostable {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setPulseWidth(double seconds) {
        pulseWidthSamples_ = static_cast<long>(seconds * sampleRate_);
    }
    void trigger() {
        if (remaining_ <= 0) remaining_ = pulseWidthSamples_;
    }
    float tick() {
        float out = (remaining_ > 0) ? 1.0f : 0.0f;
        if (remaining_ > 0) --remaining_;
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    long pulseWidthSamples_ = 100;
    long remaining_ = 0;
};

// Forrest M. Mims III's "Stepped Tone Generator," popularized as the Atari
// Punk Console: a 556 dual timer with one half wired as a free-running
// astable clock, whose output triggers the other half wired as a
// monostable. The audible result is NOT simple FM or frequency division --
// it comes from a race between two independent durations. While the
// monostable's pulse is shorter than the clock's period, every clock edge
// gets through and you hear something close to the clock frequency. Once
// the monostable's pulse width approaches or exceeds the clock period, some
// clock edges arrive while the monostable is still high and get silently
// ignored (non-retriggerable, see above) -- so the effective output rate
// drops in irregular jumps rather than sliding smoothly. That's the
// "stepped" in Stepped Tone Generator: turning the timbre knob doesn't
// glide the pitch, it jumps it around.
class AtariPunkConsole {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        monostable_.setSampleRate(sr);
    }
    void setClockFrequency(double hz) { clockFreqHz_ = hz; }
    void setPulseWidth(double seconds) { monostable_.setPulseWidth(seconds); }

    // Returns a bipolar square-ish signal in [-1, 1].
    float tick() {
        double increment = clockFreqHz_ / sampleRate_;
        phase_ += increment;
        if (phase_ >= 1.0) {
            phase_ -= 1.0;
            monostable_.trigger();
        }
        float level = monostable_.tick();
        return level * 2.0f - 1.0f;
    }

private:
    double sampleRate_ = 44100.0;
    double clockFreqHz_ = 200.0;
    double phase_ = 0.0;
    Monostable monostable_;
};

} // namespace rasgo
