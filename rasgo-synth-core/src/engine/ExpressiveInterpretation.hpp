#pragma once

#include "MaterialExpressionField.hpp"
#include "MelodicInterpreter.hpp"
#include "SpatialExpressionField.hpp"

namespace rasgo {

// Common hand-off between an instrument-specific interpreter and a later
// renderer. The fields remain inspectable and bounded; no consumer is forced
// to implement every domain at once.
struct ExpressiveInterpretation {
    bool enabled = false;
    MelodicInterpretation melodic;
    MaterialExpressionPlan material;
    SpatialExpressionPlan spatial;

    static ExpressiveInterpretation neutral() noexcept {
        ExpressiveInterpretation result;
        result.enabled = false;
        return result;
    }

    bool valid() const noexcept {
        return melodic.livingMidi >= 0.0 && melodic.livingMidi <= 127.0
            && melodic.connection >= 0.0 && melodic.connection <= 1.0
            && melodic.attack >= 0.0 && melodic.attack <= 1.0
            && melodic.sustain >= 0.0 && melodic.sustain <= 1.0
            && melodic.release >= 0.0 && melodic.release <= 1.0
            && melodic.gestureMemory >= 0.0 && melodic.gestureMemory <= 1.0
            && material.valid() && spatial.valid();
    }
};

} // namespace rasgo
