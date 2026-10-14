// Rasgo Synth App -- the narrative arc's own real Markov walk (fase 13),
// drawn as a graph instead of the linear row of blocks ArcTimelineView
// already shows. Same underlying data (each newly-entered section name,
// streamed live during a render, same call site as ArcTimelineView's own
// addSection()) -- read differently: every DISTINCT section name becomes
// one node, positioned once; every real transition the walk actually took
// becomes a directed edge. When the walk genuinely returns to a role it
// already visited, that edge is drawn in a distinct colour -- the same
// "bifurcacoes/sobreposicoes/retornos" reading already used for the
// project's own genealogy (see docs/pt/Genealogia_do_Instrumento.md,
// fase 101) applied here to a single piece's own generative history
// instead of the project's development history.
//
// v1 scope, confirmed with the user: only the arc's own section graph.
// Overlaying Atropelo's phrase-memory events (interruption/resumption)
// on the same graph is a real next step, not this pass -- it would need
// data this engine doesn't yet stream to the app per-event.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <cmath>
#include <functional>
#include <string>
#include "RasgoLookAndFeel.h"
#include "UiLanguage.h"

namespace rasgo::ui {

class GenealogyView : public juce::Component {
public:
    void setLanguage(Language language) { language_ = language; repaint(); }
    void addSection(const juce::String &name) {
        int nodeIndex = -1;
        for (size_t i = 0; i < nodes_.size(); ++i) {
            if (nodes_[i].name == name) { nodeIndex = static_cast<int>(i); break; }
        }
        bool isReturn = (nodeIndex >= 0);
        if (nodeIndex < 0) {
            nodes_.push_back({name});
            nodeIndex = static_cast<int>(nodes_.size()) - 1;
        }

        if (lastNodeIndex_ >= 0 && lastNodeIndex_ != nodeIndex) {
            // Merge with an existing identical edge (same source/target)
            // rather than drawing the same real transition twice on top
            // of itself -- a thicker line reads as "this walk took this
            // path more than once," a real fact, not visual clutter.
            bool merged = false;
            for (auto &e : edges_) {
                if (e.from == lastNodeIndex_ && e.to == nodeIndex) { ++e.count; merged = true; break; }
            }
            if (!merged) edges_.push_back({lastNodeIndex_, nodeIndex, 1, isReturn});
        }
        lastNodeIndex_ = nodeIndex;
        repaint();
    }

    void reset() {
        nodes_.clear();
        edges_.clear();
        lastNodeIndex_ = -1;
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat().reduced(6.0f);
        if (nodes_.empty()) {
            g.setColour(Palette::textDim.withAlpha(0.4f));
            g.setFont(Palette::mono(11.0f));
            g.drawText(text(strings::emptyGenealogy, language_), getLocalBounds().toFloat(), juce::Justification::centred);
            return;
        }

        // Deterministic ELLIPTICAL layout -- no overlap regardless of node
        // count, stable frame to frame as new nodes arrive (a force-
        // directed layout would jump around every time a new node appears
        // mid-render, which reads as noise, not structure), and separate
        // x/y radii matching this panel's own wide-short aspect (a single
        // uniform radius sized to the shorter dimension left almost no
        // usable room here). Evenly-spaced angle SLOTS avoid overlap, but
        // each node also gets a per-name deterministic jitter on both
        // angle (within its slot) and radius -- placed exactly on the
        // ellipse ring, every node read as too neatly "aligned," not the
        // organic/spatial scatter a real network graph should have; the
        // jitter is a hash of the node's own name, so it's stable across
        // repaints/sessions (same name always lands in the same spot),
        // not randomized noise.
        int n = static_cast<int>(nodes_.size());
        float nodeR = 15.0f;
        float cx = bounds.getCentreX(), cy = bounds.getCentreY();
        float radiusX = juce::jmax(1.0f, bounds.getWidth() * 0.5f - nodeR);
        float radiusY = juce::jmax(1.0f, bounds.getHeight() * 0.5f - nodeR);
        for (int i = 0; i < n; ++i) {
            double slotAngle = (n == 1) ? 0.0 : (2.0 * juce::MathConstants<double>::pi * i / n) - juce::MathConstants<double>::halfPi;
            size_t h = std::hash<std::string>{}(nodes_[static_cast<size_t>(i)].name.toStdString());
            double angleJitterUnit = static_cast<double>(h % 1000) / 1000.0 - 0.5;      // -0.5..0.5
            double radiusJitterUnit = static_cast<double>((h / 1000) % 1000) / 1000.0;  // 0..1
            double slotWidth = (n <= 1) ? 0.0 : (2.0 * juce::MathConstants<double>::pi / n);
            double angle = slotAngle + angleJitterUnit * slotWidth * 0.7;
            float radiusScale = 0.55f + 0.45f * static_cast<float>(radiusJitterUnit);
            nodes_[static_cast<size_t>(i)].x = cx + radiusX * radiusScale * static_cast<float>(std::cos(angle));
            nodes_[static_cast<size_t>(i)].y = cy + radiusY * radiusScale * static_cast<float>(std::sin(angle));
        }

        for (const auto &e : edges_) {
            juce::Point<float> a(nodes_[static_cast<size_t>(e.from)].x, nodes_[static_cast<size_t>(e.from)].y);
            juce::Point<float> b(nodes_[static_cast<size_t>(e.to)].x, nodes_[static_cast<size_t>(e.to)].y);
            g.setColour((e.isReturn ? Palette::copper : Palette::brass).withAlpha(0.75f));
            float thickness = juce::jlimit(1.0f, 4.0f, 1.0f + 0.6f * static_cast<float>(e.count - 1));
            if (e.from == e.to) {
                g.drawEllipse(a.x - nodeR * 0.5f, a.y - nodeR * 1.6f, nodeR, nodeR, thickness);
            } else {
                juce::Line<float> line(a, b);
                // Pull the endpoint back off the node circle so the
                // arrowhead lands next to the node, not inside it.
                auto shortened = line.withShortenedEnd(nodeR + 3.0f);
                g.drawArrow(shortened, thickness, 7.0f, 7.0f);
            }
        }

        for (const auto &node : nodes_) {
            juce::Rectangle<float> circle(node.x - nodeR, node.y - nodeR, nodeR * 2.0f, nodeR * 2.0f);
            g.setColour(Palette::bgPanel);
            g.fillEllipse(circle);
            g.setColour(Palette::brass.withAlpha(0.6f));
            g.drawEllipse(circle, 1.2f);
            // The genealogy shares a deliberately short row with the album
            // controls. Full section names forced a 7 px font inside these
            // circles, which made the graph decorative rather than legible.
            // Use a stable three-letter mnemonic and a readable font; the
            // full name remains available in the live log/metadata.
            const auto compactName = strings::sectionName(node.name, language_).substring(0, 3).toUpperCase();
            g.setColour(Palette::textBright);
            g.setFont(Palette::mono(9.5f));
            g.drawFittedText(compactName, circle.toNearestInt().reduced(2),
                             juce::Justification::centred, 1);
        }
    }

private:
    struct Node { juce::String name; float x = 0.0f; float y = 0.0f; };
    struct Edge { int from; int to; int count; bool isReturn; };

    std::vector<Node> nodes_;
    std::vector<Edge> edges_;
    int lastNodeIndex_ = -1;
    Language language_ = Language::english;
};

} // namespace rasgo::ui
