#pragma once
#include <cmath>
#include <algorithm>

// Independent C++ implementations of publicly documented chaotic recurrence
// equations. Studying Kosmische (Nicolas Steven Miller) and SuperCollider's
// ChaosUGens (created by Lance Putnam in 2004, GPLv2-or-later) helped identify
// this useful group of maps, but no source code from either project is copied
// or translated here. The state layout, rate handling, interpolation, safety
// guards and class interfaces were written for Rasgo Synth.
//
// All three update at a controllable "rate" (in Hz) that is typically far
// below the audio sample rate -- the discrete map iterates a few dozen to a
// few hundred times a second, and the output is linearly interpolated
// between iterations. This is what gives
// the sound its "stepped but sliding" quality: never a clean repeating
// cycle, always drifting -- the electronic equivalent of waves breaking
// and never quite reproducing the last backwash.

namespace rasgo {

class HenonMap {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setRate(double hz) { rateHz_ = hz; }
    // Alias: at slow rateHz values this behaves as a modulation source; set
    // it to an audio-range frequency (say 80-2000Hz) and it becomes a raw
    // oscillator in its own right, with a waveform that's neither square nor
    // sine nor anything with a name -- see DiodeShaper.hpp and the phase-3
    // waveshaping demo for how these get used as first-class audio sources.
    void setFrequency(double hz) { setRate(hz); }
    void setParams(double a, double b) { a_ = a; b_ = b; }
    void setInitial(double x0, double x1) {
        xnm2_ = x0; xnm1_ = x1; xn_ = x1; stable_ = true;
    }

    // Returns a value roughly in [-1.5, 1.5].
    float tick() {
        double samplesPerCycle = sampleRate_ / std::max(rateHz_, 0.001);
        double slope = 1.0 / samplesPerCycle;

        if (counter_ >= samplesPerCycle) {
            counter_ -= samplesPerCycle;
            if (stable_) {
                double next = 1.0 - (a_ * xnm1_ * xnm1_) + (b_ * xnm2_);
                if (next > 1.5 || next < -1.5) {
                    // Henon map diverged for this parameter combination;
                    // reset rather than blow up the audio output.
                    stable_ = false;
                    diff_ = 0.0;
                    xn_ = 1.0; xnm1_ = 0.0; xnm2_ = 0.0;
                } else {
                    xnm2_ = xnm1_;
                    xnm1_ = next;
                    xn_ = next;
                    diff_ = xnm1_ - xnm2_;
                }
            } else {
                // give it another chance next cycle with fresh initial state
                stable_ = true;
            }
            frac_ = 0.0;
        }
        counter_ += 1.0;
        float out = static_cast<float>(xnm2_ + diff_ * frac_);
        frac_ += slope;
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double rateHz_ = 40.0;
    double a_ = 1.4, b_ = 0.3;
    double xn_ = 0.1, xnm1_ = 0.1, xnm2_ = 0.05;
    double diff_ = 0.0, frac_ = 0.0, counter_ = 0.0;
    bool stable_ = true;
};

class LorenzMap {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setRate(double hz) { rateHz_ = hz; }
    void setFrequency(double hz) { setRate(hz); } // see note in HenonMap
    // Classic Lorenz parameters: s (sigma) ~10, r (rho) ~28, b (beta) ~8/3.
    void setParams(double s, double r, double b, double h) {
        s_ = s; r_ = r; b_ = b; h_ = h;
    }
    void setInitial(double x0, double y0, double z0) {
        xn_ = xnm1_ = x0; yn_ = ynm1_ = y0; zn_ = znm1_ = z0;
    }

    // Returns a value roughly in [-1, 1] (scaled down from the natural
    // Lorenz attractor range, same 0.04 scale factor as the original UGen).
    float tick() {
        double samplesPerCycle = sampleRate_ / std::max(rateHz_, 0.001);
        double slope = 1.0 / samplesPerCycle;

        if (counter_ >= samplesPerCycle) {
            counter_ -= samplesPerCycle;
            xnm1_ = xn_; ynm1_ = yn_; znm1_ = zn_;

            const double ONESIXTH = 0.1666666666666667;
            double hTimesS = h_ * s_;

            auto deriv = [&](double x, double y, double z,
                              double &dx, double &dy, double &dz) {
                dx = hTimesS * (y - x);
                dy = h_ * (x * (r_ - z) - y);
                dz = h_ * (x * y - b_ * z);
            };

            double k1x, k1y, k1z, k2x, k2y, k2z, k3x, k3y, k3z, k4x, k4y, k4z;
            deriv(xnm1_, ynm1_, znm1_, k1x, k1y, k1z);
            deriv(xnm1_ + k1x * 0.5, ynm1_ + k1y * 0.5, znm1_ + k1z * 0.5, k2x, k2y, k2z);
            deriv(xnm1_ + k2x * 0.5, ynm1_ + k2y * 0.5, znm1_ + k2z * 0.5, k3x, k3y, k3z);
            deriv(xnm1_ + k3x, ynm1_ + k3y, znm1_ + k3z, k4x, k4y, k4z);

            xn_ = xnm1_ + (k1x + 2.0 * (k2x + k3x) + k4x) * ONESIXTH;
            yn_ = ynm1_ + (k1y + 2.0 * (k2y + k3y) + k4y) * ONESIXTH;
            zn_ = znm1_ + (k1z + 2.0 * (k2z + k3z) + k4z) * ONESIXTH;

            diff_ = xn_ - xnm1_;
            frac_ = 0.0;
        }
        counter_ += 1.0;
        float out = static_cast<float>((xnm1_ + diff_ * frac_) * 0.04);
        frac_ += slope;
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double rateHz_ = 200.0;
    double s_ = 10.0, r_ = 28.0, b_ = 8.0 / 3.0, h_ = 0.05;
    double xn_ = 0.1, yn_ = 0.0, zn_ = 0.0;
    double xnm1_ = 0.1, ynm1_ = 0.0, znm1_ = 0.0;
    double diff_ = 0.0, frac_ = 0.0, counter_ = 0.0;
};

class LatoocarfianMap {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setRate(double hz) { rateHz_ = hz; }
    void setFrequency(double hz) { setRate(hz); } // see note in HenonMap
    // a, b, c, d in roughly [-3, 3]; chaotic for most combinations in that range.
    void setParams(double a, double b, double c, double d) {
        a_ = a; b_ = b; c_ = c; d_ = d;
    }
    void setInitial(double x0, double y0) { xn_ = xnm1_ = x0; yn_ = y0; }

    // Returns a value roughly in [-1, 1].
    float tick() {
        double samplesPerCycle = sampleRate_ / std::max(rateHz_, 0.001);
        double slope = 1.0 / samplesPerCycle;

        if (counter_ >= samplesPerCycle) {
            counter_ -= samplesPerCycle;
            xnm1_ = xn_;
            xn_ = std::sin(yn_ * b_) + c_ * std::sin(xnm1_ * b_);
            yn_ = std::sin(xnm1_ * a_) + d_ * std::sin(yn_ * a_);
            diff_ = xn_ - xnm1_;
            frac_ = 0.0;
        }
        counter_ += 1.0;
        float out = static_cast<float>(xnm1_ + diff_ * frac_);
        frac_ += slope;
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double rateHz_ = 80.0;
    double a_ = 1.0, b_ = 3.0, c_ = 0.5, d_ = 0.5;
    double xn_ = 0.1, xnm1_ = 0.1, yn_ = 0.1;
    double diff_ = 0.0, frac_ = 0.0, counter_ = 0.0;
};

// Six additional public chaotic maps examined during the same research phase.
// Henon/Lorenz/Latoocarfian covered the better-known attractor side; these
// broaden the available behaviours. They use Rasgo Synth's own rate-controlled,
// linearly interpolated class design and are not ports of Kosmische or
// SuperCollider source code.

class CuspMap {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setRate(double hz) { rateHz_ = hz; }
    void setFrequency(double hz) { setRate(hz); }
    // a in [1,2], b in [0,3] roughly; classic cusp bifurcation values a=1, b=1.
    void setParams(double a, double b) { a_ = a; b_ = b; }
    void setInitial(double x0) { xn_ = xnm1_ = x0; }

    // Returns a value in [-b, a] roughly (bounded by construction: each
    // iterate is a minus a non-negative term).
    float tick() {
        double samplesPerCycle = sampleRate_ / std::max(rateHz_, 0.001);
        double slope = 1.0 / samplesPerCycle;
        if (counter_ >= samplesPerCycle) {
            counter_ -= samplesPerCycle;
            xnm1_ = xn_;
            xn_ = a_ - (b_ * std::sqrt(std::abs(xn_)));
            diff_ = xn_ - xnm1_;
            frac_ = 0.0;
        }
        counter_ += 1.0;
        float out = static_cast<float>(xnm1_ + diff_ * frac_);
        frac_ += slope;
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double rateHz_ = 60.0;
    double a_ = 1.0, b_ = 1.0;
    double xn_ = 0.1, xnm1_ = 0.1;
    double diff_ = 0.0, frac_ = 0.0, counter_ = 0.0;
};

class FBSineMap {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setRate(double hz) { rateHz_ = hz; }
    void setFrequency(double hz) { setRate(hz); }
    // xn+1 = sin(a*yn + b*xn); yn+1 = mod2pi(c*yn + d). a/b shape the sine
    // feedback, c/d drift the second variable forward.
    void setParams(double a, double b, double c, double d) { a_ = a; b_ = b; c_ = c; d_ = d; }
    void setInitial(double x0, double y0) { xn_ = xnm1_ = x0; yn_ = y0; }

    // Returns a value in [-1, 1] (it's a sine of real arguments).
    float tick() {
        double samplesPerCycle = sampleRate_ / std::max(rateHz_, 0.001);
        double slope = 1.0 / samplesPerCycle;
        if (counter_ >= samplesPerCycle) {
            counter_ -= samplesPerCycle;
            xnm1_ = xn_;
            xn_ = std::sin(a_ * yn_ + b_ * xn_);
            yn_ = mod2pi(c_ * yn_ + d_);
            diff_ = xn_ - xnm1_;
            frac_ = 0.0;
        }
        counter_ += 1.0;
        float out = static_cast<float>(xnm1_ + diff_ * frac_);
        frac_ += slope;
        return out;
    }

private:
    static double mod2pi(double in) {
        const double twoPi = 6.283185307179586;
        in = std::fmod(in, twoPi);
        if (in < 0.0) in += twoPi;
        return in;
    }
    double sampleRate_ = 44100.0;
    double rateHz_ = 90.0;
    double a_ = 1.0, b_ = 1.0, c_ = 1.0, d_ = 0.5;
    double xn_ = 0.1, xnm1_ = 0.1, yn_ = 0.1;
    double diff_ = 0.0, frac_ = 0.0, counter_ = 0.0;
};

class GbmanMap {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setRate(double hz) { rateHz_ = hz; }
    void setFrequency(double hz) { setRate(hz); }
    void setInitial(double x0, double y0) { xn_ = x0; yn_ = y0; }

    // "Gingerbreadman" map -- returns a value that drifts within roughly
    // [-3, 3] depending on initial conditions, generally self-bounding.
    float tick() {
        double samplesPerCycle = sampleRate_ / std::max(rateHz_, 0.001);
        double slope = 1.0 / samplesPerCycle;
        if (counter_ >= samplesPerCycle) {
            counter_ -= samplesPerCycle;
            frac_ = 0.0;
            double xPrev = xn_;
            xn_ = (xPrev < 0.0) ? (1.0 - yn_ - xPrev) : (1.0 - yn_ + xPrev);
            yn_ = xPrev;
            diff_ = xn_ - xPrev;
        }
        counter_ += 1.0;
        // Interpolates from the previous output (yn_, which now holds the
        // prior xn_) toward the new xn_, same "glide toward the next
        // iterate" shape as the other maps here.
        float out = static_cast<float>(yn_ + diff_ * frac_);
        frac_ += slope;
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double rateHz_ = 100.0;
    double xn_ = 0.1, yn_ = 0.1;
    double diff_ = 0.0, frac_ = 0.0, counter_ = 0.0;
};

class LinCongMap {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setRate(double hz) { rateHz_ = hz; }
    void setFrequency(double hz) { setRate(hz); }
    // Linear congruential generator run as an audio-rate map: xn+1 = (a*xn +
    // c) mod m, rescaled into [-1,1]. a/c/m control the period and texture --
    // small integer-ish ratios sound noticeably more "digital"/glitchy than
    // the other maps here, closer to an old PRNG than a smooth attractor.
    void setParams(double a, double c, double m) { a_ = a; c_ = c; m_ = std::max(m, 0.001); }
    void setInitial(double x0) { xn_ = x0; xnm1_ = rescale(x0); }

    float tick() {
        double samplesPerCycle = sampleRate_ / std::max(rateHz_, 0.001);
        double slope = 1.0 / samplesPerCycle;
        if (counter_ >= samplesPerCycle) {
            counter_ -= samplesPerCycle;
            frac_ = 0.0;
            xnm1_ = rescale(xn_);
            xn_ = positiveMod(xn_ * a_ + c_, m_);
            diff_ = rescale(xn_) - xnm1_;
        }
        counter_ += 1.0;
        float out = static_cast<float>(xnm1_ + diff_ * frac_);
        frac_ += slope;
        return out;
    }

private:
    double rescale(double v) const { return (v * (2.0 / m_)) - 1.0; }
    static double positiveMod(double in, double hi) {
        double r = std::fmod(in, hi);
        if (r < 0.0) r += hi;
        return r;
    }
    double sampleRate_ = 44100.0;
    double rateHz_ = 500.0;
    double a_ = 1.1, c_ = 0.13, m_ = 1.0;
    double xn_ = 0.1, xnm1_ = -0.8;
    double diff_ = 0.0, frac_ = 0.0, counter_ = 0.0;
};

class QuadMap {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setRate(double hz) { rateHz_ = hz; }
    void setFrequency(double hz) { setRate(hz); }
    // xn+1 = a*xn^2 + b*xn + c. The classic logistic-family quadratic map;
    // a around -1 to 1, with c biasing the whole thing, gives period-doubling
    // routes to chaos as you sweep -- audibly "buzzier" as a gets extreme.
    void setParams(double a, double b, double c) { a_ = a; b_ = b; c_ = c; }
    void setInitial(double x0) { xn_ = xnm1_ = x0; }

    // Can diverge for some parameter ranges; reset defensively like HenonMap.
    float tick() {
        double samplesPerCycle = sampleRate_ / std::max(rateHz_, 0.001);
        double slope = 1.0 / samplesPerCycle;
        if (counter_ >= samplesPerCycle) {
            counter_ -= samplesPerCycle;
            frac_ = 0.0;
            xnm1_ = xn_;
            double next = a_ * xn_ * xn_ + b_ * xn_ + c_;
            if (next > 4.0 || next < -4.0 || std::isnan(next)) {
                xn_ = 0.1; xnm1_ = 0.1;
            } else {
                xn_ = next;
            }
            diff_ = xn_ - xnm1_;
        }
        counter_ += 1.0;
        float out = static_cast<float>(std::clamp(xnm1_ + diff_ * frac_, -4.0, 4.0));
        frac_ += slope;
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double rateHz_ = 150.0;
    double a_ = -1.0, b_ = 0.0, c_ = 0.75;
    double xn_ = 0.1, xnm1_ = 0.1;
    double diff_ = 0.0, frac_ = 0.0, counter_ = 0.0;
};

class StandardMap {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setRate(double hz) { rateHz_ = hz; }
    void setFrequency(double hz) { setRate(hz); }
    // The Chirikov standard map: yn+1 = mod2pi(yn + k*sin(xn)), xn+1 =
    // mod2pi(xn + yn+1). k is the "kick strength" -- near 0 it's a smooth,
    // near-quasi-periodic drift; above ~1 it turns fully chaotic.
    void setParams(double k) { k_ = k; }
    void setInitial(double x0, double y0) { xn_ = xnm1_ = x0; yn_ = y0; }

    // Output pre-scaled to [-1, 1] (matches the original's (x - pi)/pi).
    float tick() {
        double samplesPerCycle = sampleRate_ / std::max(rateHz_, 0.001);
        double slope = 1.0 / samplesPerCycle;
        if (counter_ >= samplesPerCycle) {
            counter_ -= samplesPerCycle;
            frac_ = 0.0;
            xnm1_ = xn_;
            yn_ = mod2pi(yn_ + k_ * std::sin(xn_));
            xn_ = mod2pi(xn_ + yn_);
            diff_ = xn_ - xnm1_;
        }
        counter_ += 1.0;
        const double pi = 3.141592653589793;
        float out = static_cast<float>((xnm1_ + diff_ * frac_ - pi) / pi);
        frac_ += slope;
        return out;
    }

private:
    static double mod2pi(double in) {
        const double twoPi = 6.283185307179586;
        in = std::fmod(in, twoPi);
        if (in < 0.0) in += twoPi;
        return in;
    }
    double sampleRate_ = 44100.0;
    double rateHz_ = 120.0;
    double k_ = 1.0;
    double xn_ = 0.5, xnm1_ = 0.5, yn_ = 0.5;
    double diff_ = 0.0, frac_ = 0.0, counter_ = 0.0;
};

} // namespace rasgo
