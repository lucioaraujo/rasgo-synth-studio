#pragma once
#include <cmath>
#include <cstdint>

namespace rasgo {

// Real fixed-point (Q1.15) arithmetic overflow -- the way 1980s samplers
// and sound chips (E-mu, Ensoniq, Yamaha YM2612) actually misbehaved when
// driven too hard. Not a soft clip: a silent, instantaneous wraparound
// from the most-positive representable value straight to the
// most-negative one (and back), because nothing in that era's fixed-point
// math had any concept of a limiter. The output magnitude stays exactly
// as bounded as every other waveshaper here (it's masked to 16 bits every
// time, so it can never leave roughly -1..1) -- what's different is
// continuity: a small change in input can flip the output to the
// opposite sign with no ramp in between, instead of the smooth curve
// every other waveshaper in this codebase guarantees. That discontinuity
// is the entire point, so it belongs only in a rare, seed-gated event,
// never sitting permanently in a main signal path.
//
// Never relies on signed integer overflow -- that's undefined behaviour
// in C++, full stop, regardless of how reliably real compilers happen to
// implement it today. The wraparound is reproduced deliberately: convert
// to a wide signed integer (nowhere near overflowing), move to an
// unsigned type (converting signed-to-unsigned is always well-defined,
// exactly the two's-complement bit pattern reinterpreted), mask to 16
// bits (unsigned bitwise ops are fully defined modular arithmetic), and
// only then narrow back to signed 16-bit -- that final narrowing is
// implementation-defined pre-C++20, but produces the expected
// two's-complement reinterpretation on every real-world compiler, the
// same assumption essentially all low-level audio code already makes.
class FixedPointOverflow {
public:
    // >1.0 drives the signal past the 16-bit range on purpose, causing
    // wraparound; 1.0 or below is a clean identity pass-through.
    void setDrive(double drive) { drive_ = drive; }

    // x expected roughly -1..1; output stays in that same range (see the
    // class comment on what's actually different about this stage).
    double process(double x) const {
        // A non-finite x would make the cast below genuine undefined
        // behaviour (casting NaN/Inf to an integer type), regardless of
        // how unlikely that is to actually reach this stage -- cheap to
        // rule out outright.
        if (!std::isfinite(x)) x = 0.0;
        double scaled = x * drive_ * 32768.0;
        int64_t asSigned = static_cast<int64_t>(scaled);
        uint64_t asUnsigned = static_cast<uint64_t>(asSigned); // well-defined regardless of sign
        uint16_t wrapped = static_cast<uint16_t>(asUnsigned & 0xFFFFu); // unsigned mask, fully defined
        int16_t reinterpreted = static_cast<int16_t>(wrapped); // impl-defined, universal in practice
        return static_cast<double>(reinterpreted) / 32768.0;
    }

private:
    double drive_ = 1.0;
};

} // namespace rasgo
