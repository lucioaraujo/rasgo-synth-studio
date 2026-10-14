#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

namespace rasgo {

using RegionId = std::uint32_t;
using ArchivePageId = std::uint64_t;

// Phase 119: modules are open archives, not audio-only black boxes. A domain
// names the kind of material being offered or read; it does not prescribe a
// uniform set of ports for every module.
enum class ModuleDomain {
    Audio,
    Material,
    Gesture,
    Movement,
    Pulse,
    Rhythm,
    Pitch,
    Harmony,
    Timbre,
    Voice,
    Energy,
    Resistance,
    Memory,
    Rule,
    Space,
    Silence,
    Opening,
    Scar
};

enum class ArchiveAction {
    Publish,
    Read,
    Reread,
    Reinterpret,
    Recreate,
    Split,
    Traverse,
    Open,
    Scar
};

struct ArchivePage {
    ArchivePageId id = 0;
    ArchivePageId parent = 0;
    RegionId source = 0;
    RegionId reader = 0;
    ModuleDomain inputDomain = ModuleDomain::Material;
    ModuleDomain outputDomain = ModuleDomain::Material;
    ArchiveAction action = ArchiveAction::Publish;
    double value = 0.0;
    double auxiliary = 0.0;
    int bar = 0;
};

class OpenModuleArchive {
public:
    explicit OpenModuleArchive(std::size_t capacity = 4096)
        : capacity_(std::max<std::size_t>(32, capacity)) {}

    ArchivePageId publish(RegionId source, ModuleDomain domain, double value,
                          double auxiliary, int bar,
                          ArchiveAction action = ArchiveAction::Publish,
                          ArchivePageId parent = 0,
                          RegionId reader = 0,
                          ModuleDomain inputDomain = ModuleDomain::Material) {
        ArchivePage page;
        page.id = nextId_++;
        page.parent = parent;
        page.source = source;
        page.reader = reader;
        page.inputDomain = parent == 0 ? domain : inputDomain;
        page.outputDomain = domain;
        page.action = action;
        page.value = value;
        page.auxiliary = auxiliary;
        page.bar = bar;
        pages_.push_back(page);
        trim();
        return page.id;
    }

    std::optional<ArchivePage> page(ArchivePageId id) const {
        auto it = std::find_if(pages_.begin(), pages_.end(),
                               [id](const ArchivePage &candidate) { return candidate.id == id; });
        if (it == pages_.end()) return std::nullopt;
        return *it;
    }

    std::optional<ArchivePage> latest(ModuleDomain domain,
                                      RegionId excludingSource = 0) const {
        for (auto it = pages_.rbegin(); it != pages_.rend(); ++it) {
            if (it->outputDomain == domain
                && (excludingSource == 0 || it->source != excludingSource)) {
                return *it;
            }
        }
        return std::nullopt;
    }

    // Reading is itself archived. This makes influence auditable: a source
    // publishing a page and another region actually reading it are distinct
    // events, rather than a flag that merely claims a relationship exists.
    std::optional<ArchivePage> readLatest(RegionId reader, ModuleDomain domain,
                                          int bar, bool reread = false) {
        auto found = latest(domain, reader);
        if (!found) return std::nullopt;
        ArchivePage copy = *found;
        publish(copy.source, domain, copy.value, copy.auxiliary, bar,
                reread ? ArchiveAction::Reread : ArchiveAction::Read,
                copy.id, reader, copy.outputDomain);
        return copy;
    }

    ArchivePageId reinterpret(RegionId reader, ArchivePageId sourcePage,
                              ModuleDomain outputDomain, double value,
                              double auxiliary, int bar,
                              ArchiveAction action = ArchiveAction::Reinterpret) {
        auto source = page(sourcePage);
        const RegionId provenance = source ? source->source : reader;
        const ModuleDomain input = source ? source->outputDomain : outputDomain;
        return publish(provenance, outputDomain, value, auxiliary, bar, action,
                       sourcePage, reader, input);
    }

    std::size_t count(ArchiveAction action) const {
        return static_cast<std::size_t>(std::count_if(
            pages_.begin(), pages_.end(),
            [action](const ArchivePage &page) { return page.action == action; }));
    }

    std::size_t count(ModuleDomain domain) const {
        return static_cast<std::size_t>(std::count_if(
            pages_.begin(), pages_.end(),
            [domain](const ArchivePage &page) { return page.outputDomain == domain; }));
    }

    const std::vector<ArchivePage> &pages() const { return pages_; }
    std::size_t size() const { return pages_.size(); }

private:
    void trim() {
        if (pages_.size() <= capacity_) return;
        const std::size_t excess = pages_.size() - capacity_;
        pages_.erase(pages_.begin(), pages_.begin() + static_cast<std::ptrdiff_t>(excess));
    }

    std::size_t capacity_;
    ArchivePageId nextId_ = 1;
    std::vector<ArchivePage> pages_;
};

} // namespace rasgo

