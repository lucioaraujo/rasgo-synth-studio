#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <vector>

#include "OpenModuleArchive.hpp"

namespace rasgo {

enum class RegionRole {
    Core,
    Rhythm,
    Harmony,
    Voice,
    Texture,
    Space,
    Rule,
    Archive,
    Descartographer,
    Other
};

enum class RegionSide { Whole, Interior, Exterior };

enum class PassageMode {
    Preserve,
    Invert,
    Difference,
    Accumulate,
    Fold
};

struct TopologyRegion {
    RegionId id = 0;
    RegionId parent = 0;
    RegionRole role = RegionRole::Other;
    RegionSide side = RegionSide::Whole;
    bool active = true;
    int generation = 0;
    double state = 0.0;
    double energy = 0.0;
    double resistance = 0.5;
    double scar = 0.0;
};

struct TopologyPassage {
    std::uint32_t id = 0;
    RegionId from = 0;
    RegionId to = 0;
    ModuleDomain inputDomain = ModuleDomain::Material;
    ModuleDomain outputDomain = ModuleDomain::Material;
    PassageMode mode = PassageMode::Preserve;
    bool open = true;
    bool bidirectional = false;
    double strength = 1.0;
    double resistance = 0.0;
    double memory = 0.0;
    int traversalCount = 0;
};

struct CismResult {
    RegionId original = 0;
    RegionId interior = 0;
    RegionId exterior = 0;
    std::uint32_t opening = 0;
};

struct TraversalResult {
    bool crossed = false;
    RegionId from = 0;
    RegionId to = 0;
    ModuleDomain inputDomain = ModuleDomain::Material;
    ModuleDomain outputDomain = ModuleDomain::Material;
    double input = 0.0;
    double output = 0.0;
};

struct FeedbackResult {
    TraversalResult outward;
    TraversalResult returning;
};

// A small mutable topology for compositional relations. It deliberately does
// not process audio samples: it represents who can read whom, through which
// domain and with what resistance. Audio modules remain free to expose only the
// openings that belong to their own personality.
class CompositionalTopology {
public:
    explicit CompositionalTopology(OpenModuleArchive &archive)
        : archive_(archive) {}

    RegionId addRegion(RegionRole role, double state, double energy,
                       double resistance, int bar = 0,
                       RegionId parent = 0,
                       RegionSide side = RegionSide::Whole) {
        TopologyRegion region;
        region.id = nextRegionId_++;
        region.parent = parent;
        region.role = role;
        region.side = side;
        region.generation = parent == 0 ? 0 : generationOf(parent) + 1;
        region.state = state;
        region.energy = std::clamp(energy, 0.0, 1.0);
        region.resistance = std::clamp(resistance, 0.0, 1.0);
        regions_.push_back(region);
        archive_.publish(region.id, ModuleDomain::Material, state, region.energy, bar);
        archive_.publish(region.id, ModuleDomain::Resistance, region.resistance,
                         static_cast<double>(region.generation), bar);
        return region.id;
    }

    std::uint32_t connect(RegionId from, RegionId to,
                          ModuleDomain inputDomain,
                          ModuleDomain outputDomain,
                          PassageMode mode, double strength,
                          double resistance, bool bidirectional,
                          int bar = 0) {
        if (!activeRegion(from) || !activeRegion(to) || from == to) return 0;
        TopologyPassage passage;
        passage.id = nextPassageId_++;
        passage.from = from;
        passage.to = to;
        passage.inputDomain = inputDomain;
        passage.outputDomain = outputDomain;
        passage.mode = mode;
        passage.strength = std::clamp(strength, 0.0, 2.0);
        passage.resistance = std::clamp(resistance, 0.0, 1.0);
        passage.bidirectional = bidirectional;
        passages_.push_back(passage);
        archive_.publish(from, ModuleDomain::Opening, passage.strength,
                         passage.resistance, bar, ArchiveAction::Open, 0, to,
                         inputDomain);
        return passage.id;
    }

    CismResult split(RegionId original, double force,
                      ModuleDomain openingDomain, int bar) {
        TopologyRegion *source = mutableRegion(original);
        if (!source || !source->active) return {};

        const TopologyRegion snapshot = *source;
        source->active = false;
        source->scar = std::max(source->scar, std::clamp(force, 0.0, 1.0));

        const double f = std::clamp(force, 0.0, 1.0);
        const double asymmetry = 0.08 + 0.34 * f;
        const RegionId interior = addRegion(
            snapshot.role,
            snapshot.state - asymmetry * (0.5 + std::fabs(snapshot.state)),
            snapshot.energy * (0.58 + 0.22 * f),
            std::clamp(snapshot.resistance + 0.18 * (1.0 - f), 0.0, 1.0),
            bar, original, RegionSide::Interior);
        const RegionId exterior = addRegion(
            snapshot.role,
            snapshot.state + asymmetry * (0.5 + std::fabs(snapshot.state)),
            snapshot.energy * (0.42 + 0.38 * f),
            std::clamp(snapshot.resistance - 0.22 * f, 0.0, 1.0),
            bar, original, RegionSide::Exterior);

        // Existing routes do not disappear: they are redistributed between
        // the descendants. This changes adjacency rather than cloning the
        // original region and all of its connections.
        for (TopologyPassage &passage : passages_) {
            if (passage.from == original)
                passage.from = (passage.id % 2U == 0U) ? interior : exterior;
            if (passage.to == original)
                passage.to = (passage.id % 2U == 0U) ? exterior : interior;
        }

        const std::uint32_t opening = connect(
            interior, exterior, openingDomain, openingDomain,
            PassageMode::Difference, 0.75 + 0.75 * f,
            std::clamp(snapshot.resistance * (1.0 - 0.8 * f), 0.0, 1.0),
            true, bar);

        archive_.publish(original, ModuleDomain::Scar, f,
                         static_cast<double>(opening), bar,
                         ArchiveAction::Split);
        archive_.publish(interior, ModuleDomain::Opening,
                         static_cast<double>(exterior), f, bar,
                         ArchiveAction::Open, 0, exterior, openingDomain);
        ++splitCount_;
        return {original, interior, exterior, opening};
    }

    TraversalResult traverse(std::uint32_t passageId, RegionId from,
                             double value, int bar) {
        TopologyPassage *passage = mutablePassage(passageId);
        if (!passage || !passage->open) return {};
        RegionId target = 0;
        if (from == passage->from) target = passage->to;
        else if (from == passage->to && passage->bidirectional) target = passage->from;
        else return {};
        TopologyRegion *source = mutableRegion(from);
        TopologyRegion *destination = mutableRegion(target);
        if (!source || !destination || !source->active || !destination->active) return {};

        double interpreted = value;
        switch (passage->mode) {
            case PassageMode::Preserve: break;
            case PassageMode::Invert: interpreted = -value; break;
            case PassageMode::Difference: interpreted = value - passage->memory; break;
            case PassageMode::Accumulate: interpreted = value + passage->memory; break;
            case PassageMode::Fold:
                interpreted = 1.0 - std::fabs(std::fmod(std::fabs(value), 2.0) - 1.0);
                break;
        }
        const double transmission = passage->strength * (1.0 - passage->resistance);
        const double output = interpreted * transmission;

        // A crossing changes traveler, destination and passage. Resistance
        // erodes slightly; the destination incorporates the arrival; the
        // passage remembers what crossed it.
        passage->memory = 0.78 * passage->memory + 0.22 * output;
        passage->resistance = std::clamp(
            passage->resistance - std::min(0.04, std::fabs(value) * 0.008),
            0.0, 1.0);
        ++passage->traversalCount;
        destination->state = 0.82 * destination->state + 0.18 * output;
        destination->energy = std::clamp(
            destination->energy + std::fabs(output) * 0.05, 0.0, 1.0);
        source->scar = std::clamp(source->scar + std::fabs(value - output) * 0.01,
                                  0.0, 1.0);

        const ArchivePageId sourcePage = archive_.publish(
            from, passage->inputDomain, value, passage->resistance, bar,
            ArchiveAction::Traverse, 0, target, passage->inputDomain);
        archive_.reinterpret(target, sourcePage, passage->outputDomain,
                             output, passage->memory, bar,
                             ArchiveAction::Reinterpret);
        ++traversalCount_;
        return {true, from, target, passage->inputDomain,
                passage->outputDomain, value, output};
    }

    // A feedback is an outward crossing followed by a transformed return.
    // It is only possible through a bidirectional opening and therefore
    // retains the same provenance, resistance erosion and archive trail as
    // two real traversals rather than being an invisible numeric shortcut.
    FeedbackResult feedback(std::uint32_t passageId, RegionId from,
                            double value, double returnAmount, int bar) {
        FeedbackResult result;
        result.outward = traverse(passageId, from, value, bar);
        if (!result.outward.crossed) return result;
        const TopologyPassage *crossing = passage(passageId);
        if (!crossing || !crossing->bidirectional) return result;
        const double returnedValue = result.outward.output
                                   * std::clamp(returnAmount, -1.5, 1.5);
        result.returning = traverse(passageId, result.outward.to,
                                    returnedValue, bar);
        return result;
    }

    bool reroute(std::uint32_t passageId, RegionId newFrom,
                 RegionId newTo, int bar) {
        TopologyPassage *passage = mutablePassage(passageId);
        if (!passage || !activeRegion(newFrom) || !activeRegion(newTo)
            || newFrom == newTo) return false;
        const RegionId oldFrom = passage->from;
        const RegionId oldTo = passage->to;
        passage->from = newFrom;
        passage->to = newTo;
        archive_.publish(oldFrom, ModuleDomain::Scar,
                         static_cast<double>(oldTo), passage->memory, bar,
                         ArchiveAction::Scar, 0, newFrom, passage->inputDomain);
        archive_.publish(newFrom, ModuleDomain::Opening,
                         static_cast<double>(newTo), passage->strength, bar,
                         ArchiveAction::Open, 0, newTo, passage->inputDomain);
        ++rerouteCount_;
        return true;
    }

    bool setPassageOpen(std::uint32_t id, bool open, int bar) {
        TopologyPassage *passage = mutablePassage(id);
        if (!passage || passage->open == open) return false;
        passage->open = open;
        archive_.publish(passage->from,
                         open ? ModuleDomain::Opening : ModuleDomain::Silence,
                         open ? 1.0 : 0.0, passage->resistance, bar,
                         open ? ArchiveAction::Open : ArchiveAction::Scar,
                         0, passage->to, passage->inputDomain);
        return true;
    }

    const TopologyRegion *region(RegionId id) const {
        auto it = std::find_if(regions_.begin(), regions_.end(),
                               [id](const TopologyRegion &region) { return region.id == id; });
        return it == regions_.end() ? nullptr : &*it;
    }

    const TopologyPassage *passage(std::uint32_t id) const {
        auto it = std::find_if(passages_.begin(), passages_.end(),
                               [id](const TopologyPassage &passage) { return passage.id == id; });
        return it == passages_.end() ? nullptr : &*it;
    }

    const std::vector<TopologyRegion> &regions() const { return regions_; }
    const std::vector<TopologyPassage> &passages() const { return passages_; }
    int splitCount() const { return splitCount_; }
    int traversalCount() const { return traversalCount_; }
    int rerouteCount() const { return rerouteCount_; }

private:
    TopologyRegion *mutableRegion(RegionId id) {
        auto it = std::find_if(regions_.begin(), regions_.end(),
                               [id](const TopologyRegion &region) { return region.id == id; });
        return it == regions_.end() ? nullptr : &*it;
    }

    TopologyPassage *mutablePassage(std::uint32_t id) {
        auto it = std::find_if(passages_.begin(), passages_.end(),
                               [id](const TopologyPassage &passage) { return passage.id == id; });
        return it == passages_.end() ? nullptr : &*it;
    }

    bool activeRegion(RegionId id) const {
        const TopologyRegion *candidate = region(id);
        return candidate && candidate->active;
    }

    int generationOf(RegionId id) const {
        const TopologyRegion *candidate = region(id);
        return candidate ? candidate->generation : -1;
    }

    OpenModuleArchive &archive_;
    RegionId nextRegionId_ = 1;
    std::uint32_t nextPassageId_ = 1;
    std::vector<TopologyRegion> regions_;
    std::vector<TopologyPassage> passages_;
    int splitCount_ = 0;
    int traversalCount_ = 0;
    int rerouteCount_ = 0;
};

} // namespace rasgo
