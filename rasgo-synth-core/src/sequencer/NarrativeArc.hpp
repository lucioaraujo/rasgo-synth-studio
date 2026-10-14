#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

namespace rasgo {

// The layer the original Kosmische never had: instead of a single static
// patch looping forever, a piece is described as a sequence of sections
// (an "arc"), each with its own target density, filter openness, resonance
// and melodic register. Values are smoothly interpolated between section
// targets across the whole piece, using a smoothstep easing so transitions
// feel like a gradual shift rather than a hard cut -- the closest thing so
// far to "a composition that develops," per the goal of using this as an
// album-writing instrument rather than a performance loop.
struct Section {
    std::string name;
    double relativeDuration; // proportion of total piece length (weights, not seconds)
    double density;          // 0..1, how full the rhythmic gate pattern is
    double filterOpenness;   // 0..1, maps to a cutoff frequency range
    double resonance;        // filter resonance amount
    int registerOffset;      // semitones added to the sequencer's notes
};

struct ArcState {
    double density;
    double filterOpenness;
    double resonance;
    int registerOffset; // nearest-section value (not interpolated -- register jumps make sense; cutoff/density glide)
    std::string currentSectionName;
};

class NarrativeArc {
public:
    void setSections(std::vector<Section> sections) { sections_ = std::move(sections); }

    // t: 0..1, fraction of the piece elapsed.
    ArcState evaluate(double t) const {
        t = std::clamp(t, 0.0, 1.0);
        double totalWeight = 0.0;
        for (auto &s : sections_) totalWeight += s.relativeDuration;

        double cursor = 0.0;
        for (size_t i = 0; i < sections_.size(); ++i) {
            double segment = sections_[i].relativeDuration / totalWeight;
            double segEnd = cursor + segment;
            if (t <= segEnd || i == sections_.size() - 1) {
                double localT = segment > 0.0 ? (t - cursor) / segment : 0.0;
                localT = std::clamp(localT, 0.0, 1.0);

                const Section &cur = sections_[i];
                const Section &next = (i + 1 < sections_.size()) ? sections_[i + 1] : sections_[i];

                double ease = smoothstep(localT);
                ArcState state;
                state.density = lerp(cur.density, next.density, ease);
                state.filterOpenness = lerp(cur.filterOpenness, next.filterOpenness, ease);
                state.resonance = lerp(cur.resonance, next.resonance, ease);
                state.registerOffset = (localT < 0.5) ? cur.registerOffset : next.registerOffset;
                state.currentSectionName = cur.name;
                return state;
            }
            cursor = segEnd;
        }
        // Fallback (empty arc): a neutral, static state.
        return {0.5, 0.5, 1.0, 0, "(sem secoes)"};
    }

private:
    static double lerp(double a, double b, double t) { return a + (b - a) * t; }
    static double smoothstep(double t) { return t * t * (3.0 - 2.0 * t); }

    std::vector<Section> sections_;
};

} // namespace rasgo
