#pragma once
#include "Oscillator.hpp"

namespace rasgo {

// Hard sync: a genuinely distinct synthesis principle from every other
// oscillator/texture already in this engine (Walsh's orthogonal rectangles,
// VOSIM's decaying pips, Gendyn's breakpoints, morph-crossfade...) -- here
// the harmonic content comes from forcibly restarting a "slave" oscillator
// mid-cycle. The slave runs at its own free frequency, but every time the
// "master" oscillator completes a cycle, the slave's phase is torn back to
// zero regardless of where it had gotten to. The audible result: pitch
// stays locked to the MASTER's frequency (every output cycle is exactly one
// master period long), while the slave/master frequency ratio controls a
// harsh, sweepable timbre on top of that fixed pitch -- the classic
// hard-sync sweep sound. Real technique from countless analog VCOs with a
// "sync" input (Polivoks VCO3 among them); reimplemented here from that
// documented behaviour, not any specific circuit's schematic or code.
class HardSyncVoice {
public:
    void setSampleRate(double sr) {
        master_.setSampleRate(sr);
        slave_.setSampleRate(sr);
    }
    void setMasterFrequency(double hz) { master_.setFrequency(hz); }
    void setSlaveFrequency(double hz) { slave_.setFrequency(hz); }
    void setSlaveWaveform(Oscillator::Waveform w) { slave_.setWaveform(w); }

    float tick() {
        // Tick the master first (silent -- only its wrap matters here, its
        // own waveform is never part of the output) so the slave's reset
        // lands on the exact sample the new master cycle begins.
        master_.tick();
        if (master_.didWrap()) slave_.resetPhase();
        return slave_.tick();
    }

private:
    Oscillator master_;
    Oscillator slave_;
};

} // namespace rasgo
