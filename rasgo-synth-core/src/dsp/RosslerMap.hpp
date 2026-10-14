#pragma once
#include <cmath>
#include <algorithm>

namespace rasgo {

// The Rossler attractor (Otto E. Rossler, 1976) -- not part of the nine-map
// group examined through the earlier Kosmische/ChaosUGens research, but a
// separate addition surfaced from DIY/Eurorack research on rupture-
// focused circuits. Same RK4 integration and rate-based ticking as
// LorenzMap alongside it, but a qualitatively different shape: instead of
// orbiting two stable "wings," a Rossler trajectory spirals outward slowly
// on the x/y plane and then, once it swings past a threshold on x, the z
// axis kicks sharply upward and yanks the whole trajectory back toward
// the centre before the next slow spiral begins. tick() behaves like every
// other chaos map here (a continuous, roughly [-1,1] modulation source);
// justErupted() additionally exposes the exact sample where one of those
// kicks crosses its threshold, for anything that wants to key off the
// rupture moment itself rather than just ride the continuous value.
class RosslerMap {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setRate(double hz) { rateHz_ = hz; }
    // Classic parameters: a=0.2, b=0.2, c=5.7.
    void setParams(double a, double b, double c, double h) {
        a_ = a; b_ = b; c_ = c; h_ = h;
    }
    void setInitial(double x0, double y0, double z0) {
        xn_ = xnm1_ = x0; yn_ = ynm1_ = y0; zn_ = znm1_ = z0;
    }

    float tick() {
        double samplesPerCycle = sampleRate_ / std::max(rateHz_, 0.001);
        double slope = 1.0 / samplesPerCycle;

        if (counter_ >= samplesPerCycle) {
            counter_ -= samplesPerCycle;
            xnm1_ = xn_; ynm1_ = yn_; znm1_ = zn_;

            const double ONESIXTH = 0.1666666666666667;
            auto deriv = [&](double x, double y, double z,
                              double &dx, double &dy, double &dz) {
                dx = h_ * (-y - z);
                dy = h_ * (x + a_ * y);
                dz = h_ * (b_ + z * (x - c_));
            };

            double k1x, k1y, k1z, k2x, k2y, k2z, k3x, k3y, k3z, k4x, k4y, k4z;
            deriv(xnm1_, ynm1_, znm1_, k1x, k1y, k1z);
            deriv(xnm1_ + k1x * 0.5, ynm1_ + k1y * 0.5, znm1_ + k1z * 0.5, k2x, k2y, k2z);
            deriv(xnm1_ + k2x * 0.5, ynm1_ + k2y * 0.5, znm1_ + k2z * 0.5, k3x, k3y, k3z);
            deriv(xnm1_ + k3x, ynm1_ + k3y, znm1_ + k3z, k4x, k4y, k4z);

            xn_ = xnm1_ + (k1x + 2.0 * (k2x + k3x) + k4x) * ONESIXTH;
            yn_ = ynm1_ + (k1y + 2.0 * (k2y + k3y) + k4y) * ONESIXTH;
            zn_ = znm1_ + (k1z + 2.0 * (k2z + k3z) + k4z) * ONESIXTH;
            // Guard against the same runaway-integration risk the rest of
            // this codebase already defends against elsewhere (e.g.
            // ModalResonator) -- a pathological seed/rate combination
            // shouldn't be able to send this to infinity.
            if (!std::isfinite(xn_) || !std::isfinite(yn_) || !std::isfinite(zn_) ||
                std::fabs(xn_) > 60.0 || std::fabs(yn_) > 60.0 || std::fabs(zn_) > 80.0) {
                xn_ = xnm1_ = 1.0; yn_ = ynm1_ = 1.0; zn_ = znm1_ = 1.0;
            }

            bool wasHigh = zPrevRaw_ > kEruptThreshold;
            bool isHigh = zn_ > kEruptThreshold;
            justErupted_ = isHigh && !wasHigh;
            zPrevRaw_ = zn_;

            diffX_ = xn_ - xnm1_;
            frac_ = 0.0;
        } else {
            justErupted_ = false;
        }
        counter_ += 1.0;
        float out = static_cast<float>((xnm1_ + diffX_ * frac_) * 0.09);
        frac_ += slope;
        return std::clamp(out, -1.5f, 1.5f);
    }

    // True for one tick right as z crosses up past the eruption
    // threshold -- the "spiral suddenly breaks" moment.
    bool justErupted() const { return justErupted_; }

private:
    static constexpr double kEruptThreshold = 8.0;
    double sampleRate_ = 44100.0;
    double rateHz_ = 60.0;
    double a_ = 0.2, b_ = 0.2, c_ = 5.7, h_ = 0.1;
    double xn_ = 1.0, yn_ = 1.0, zn_ = 1.0;
    double xnm1_ = 1.0, ynm1_ = 1.0, znm1_ = 1.0;
    double diffX_ = 0.0, frac_ = 0.0, counter_ = 0.0;
    double zPrevRaw_ = 0.0;
    bool justErupted_ = false;
};

} // namespace rasgo
