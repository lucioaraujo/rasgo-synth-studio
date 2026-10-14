// Rasgo Synth -- "elemento estranho", the literal version: a small, honest
// contamination of the deterministic seed-derived system by something that
// isn't derived from the seed at all -- the real physical machine this
// specific render happened to run on, at the moment it happened to run.
//
// Deliberately NOT keyboard or mouse input (that's just a keylogger with
// better branding, declined regardless of artistic framing) and NOT read
// continuously (this engine renders a whole piece in a fraction of a
// second of wall-clock time -- there is no "live performance" for an
// external signal to meaningfully vary across). One reading, taken once,
// right at the start of the render, honestly exposed in the piece's own
// metadata rather than silently folded in -- a piece that used this is a
// piece that is no longer purely reproducible from its seed alone, and
// that fact is worth stating plainly, not hiding.
//
// Linux-specific (/sys, /proc) -- this is a desktop offline-render tool,
// not embedded/portable code, so that's an acceptable boundary. Every read
// degrades gracefully: a missing file, an unreadable value, or a garbage
// number all just leave `available` false rather than propagating a NaN
// or a crash into the render.

#pragma once
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

namespace rasgo {

struct ForeignReading {
    bool available = false;
    std::string description; // human-readable, honest about the source
    double normalized = 0.5;  // always in [0,1] regardless of source, safe to use unchecked
};

inline ForeignReading readForeignSignal() {
    // Primary: real CPU temperature. Most Linux desktops expose at least
    // one thermal zone; try a handful before giving up, since not every
    // zone is a meaningful CPU reading (some are Wi-Fi/battery/etc) but the
    // first one that parses is good enough -- this isn't trying to be a
    // rigorous sensor survey, just a genuine trace of the physical machine.
    for (int zone = 0; zone < 6; ++zone) {
        std::ifstream tempFile("/sys/class/thermal/thermal_zone" + std::to_string(zone) + "/temp");
        if (!tempFile.is_open()) continue;
        double milliC = 0.0;
        if (tempFile >> milliC && milliC > 0.0) {
            double celsius = milliC / 1000.0;
            // 20C..90C covers idle-to-loaded for essentially any real CPU;
            // clamp rather than wrap so a sensor reporting something odd
            // (a battery zone in the 0-100 range, say) still lands
            // somewhere plausible instead of aliasing back into range.
            double norm = std::clamp((celsius - 20.0) / 70.0, 0.0, 1.0);
            ForeignReading r;
            r.available = true;
            r.normalized = norm;
            std::ostringstream desc;
            desc << "cpu ~" << celsius << "C (thermal_zone" << zone << ")";
            r.description = desc.str();
            return r;
        }
    }

    // Fallback: 1-minute load average plus the sub-second fractional part
    // of system uptime -- both genuinely reflect this machine's real
    // condition at this exact moment, neither derivable from the piece's
    // own seed.
    std::ifstream loadFile("/proc/loadavg");
    std::ifstream upFile("/proc/uptime");
    if (loadFile.is_open() && upFile.is_open()) {
        double load1 = 0.0, uptime = 0.0;
        if ((loadFile >> load1) && (upFile >> uptime) && uptime > 0.0) {
            double frac = uptime - std::floor(uptime);
            double loadNorm = std::clamp(load1 / 4.0, 0.0, 1.0);
            double norm = std::clamp(0.5 * loadNorm + 0.5 * frac, 0.0, 1.0);
            ForeignReading r;
            r.available = true;
            r.normalized = norm;
            std::ostringstream desc;
            desc << "loadavg " << load1 << ", uptime frac " << frac << "s";
            r.description = desc.str();
            return r;
        }
    }

    return {}; // unavailable on this machine -- available stays false
}

} // namespace rasgo
