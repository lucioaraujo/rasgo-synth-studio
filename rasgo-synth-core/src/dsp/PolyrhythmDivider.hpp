#pragma once
#include <cstdint>

namespace rasgo {

// Ken Stone's CGS22 "Master Divider" -- a fixture of the Lunetta CMOS-logic
// synth scene this project's own hardware precursors (Antitotem, the
// original Rasgo Synth) grew out of. The real module is a CD4024 binary
// ripple counter (four divide-by-2/4/8/16 outputs, Q1..Q4) feeding a bank of
// XOR gates: since each divided output is a square wave with its own phase,
// XORing any two of them does not just multiply their periods together --
// it produces a third pulse train whose rising edges fall at points neither
// original division visits alone. A handful of XOR pairs chosen up front
// turns four plain clock divisions into a family of interlocking,
// genuinely polyrhythmic trigger patterns from one single master clock.
//
// Reimplemented here as the underlying counter/XOR logic, not a literal
// port of Ken Stone's PCB -- this is public, well-documented DIY-synth
// circuit knowledge (see sdiy.info's CGS master divider page), not someone
// else's source code.
class PolyrhythmDivider {
public:
    static constexpr int kNumStages = 4; // Q1..Q4, i.e. /2 /4 /8 /16

    // Advance the ripple counter by one master-clock pulse. In this engine
    // that pulse is one sequencer step, same granularity the melodic voice
    // is clocked at -- so the percussion layer below stays locked to the
    // same underlying grid without literally repeating its pattern.
    void clock() { ++counter_; }

    // Starts the counter at a given phase instead of zero. The real CD4024
    // always powers up wherever it powers up; here it lets the seed decide
    // where in its 16-step cycle each piece's percussion pattern begins,
    // instead of every piece opening on the exact same divider phase.
    void reset(uint32_t start) { counter_ = start; }

    bool q(int stage) const { return ((counter_ >> stage) & 1u) != 0; }

    // Rising edge of Q(a) XOR Q(b) -- one of the phase-shifted pulse trains
    // the real module derives from the four plain divisions. Each (a, b)
    // pair gets its own independent edge detector even though they all
    // read the same shared counter, so several callers can each watch a
    // different combination without interfering with one another.
    bool risingEdge(int a, int b) {
        bool now = q(a) != q(b);
        bool &prev = prevXor_[a][b];
        bool edge = now && !prev;
        prev = now;
        return edge;
    }

private:
    uint32_t counter_ = 0;
    bool prevXor_[kNumStages][kNumStages] = {};
};

} // namespace rasgo
