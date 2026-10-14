#pragma once

#include <cstdint>

namespace rasgo {

// The eight documentary states distinguish code availability from audible
// and causal musical life. A record is monotonic within one render: later
// evidence cannot erase an earlier state, and higher states require their
// prerequisites.
enum class ModuleLifeState : std::uint8_t {
    Available = 0,
    Selected,
    Manifested,
    Audible,
    Salient,
    Influential,
    Affected,
    Transformative,
    Count
};

class ModuleLifeRecord {
public:
    static ModuleLifeRecord fromMask(std::uint8_t mask) noexcept {
        ModuleLifeRecord record;
        const auto validMask = static_cast<std::uint8_t>(
            (std::uint8_t{1u} << static_cast<std::uint8_t>(ModuleLifeState::Count)) - 1u);
        record.flags_ = static_cast<std::uint8_t>(mask & validMask);
        return record;
    }

    void mark(ModuleLifeState state) noexcept {
        const auto index = static_cast<std::uint8_t>(state);
        if (index >= static_cast<std::uint8_t>(ModuleLifeState::Count)) return;
        flags_ = static_cast<std::uint8_t>(flags_ | (std::uint8_t{1u} << index));
    }

    bool has(ModuleLifeState state) const noexcept {
        const auto index = static_cast<std::uint8_t>(state);
        return index < static_cast<std::uint8_t>(ModuleLifeState::Count)
            && (flags_ & (std::uint8_t{1u} << index)) != 0;
    }

    // A record is valid only when evidence forms a continuous path. This
    // prevents telemetry from claiming influence without manifestation or
    // transformation without a preceding causal observation.
    bool valid() const noexcept {
        bool seenGap = false;
        for (std::uint8_t i = 0;
             i < static_cast<std::uint8_t>(ModuleLifeState::Count); ++i) {
            const bool present = (flags_ & (std::uint8_t{1u} << i)) != 0;
            if (!present) seenGap = true;
            else if (seenGap) return false;
        }
        return true;
    }

    ModuleLifeState highest() const noexcept {
        ModuleLifeState result = ModuleLifeState::Available;
        for (std::uint8_t i = 0;
             i < static_cast<std::uint8_t>(ModuleLifeState::Count); ++i) {
            if ((flags_ & (std::uint8_t{1u} << i)) == 0) break;
            result = static_cast<ModuleLifeState>(i);
        }
        return result;
    }

    std::uint8_t mask() const noexcept { return flags_; }

private:
    std::uint8_t flags_ = 0;
};

inline const char *moduleLifeStateName(ModuleLifeState state) noexcept {
    static constexpr const char *names[] = {
        "available", "selected", "manifested", "audible",
        "salient", "influential", "affected", "transformative", "count"
    };
    const auto index = static_cast<std::uint8_t>(state);
    return index < static_cast<std::uint8_t>(ModuleLifeState::Count)
        ? names[index] : "unknown";
}

} // namespace rasgo
