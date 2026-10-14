#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../engine/OrganismStemAuditor.hpp"

namespace {
int failures = 0;
void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}
}

int main() {
    using namespace rasgo;

    OrganismStemAuditor disabled;
    disabled.prepare(48000.0, false, false, 0.6, 0.6);
    check(!disabled.enabled(), "audit can stay completely disabled");
    disabled.beginSection();
    for (int frame = 0; frame < 32; ++frame) {
        disabled.beginFrame();
        disabled.addPreDelay(OrganismStem::Core, 0.2);
        disabled.addPreReverb(OrganismStem::Drums, 0.1, 0.1);
        disabled.closePreReverb(0.3, 0.3);
        disabled.processPostSpace(0.3, 0.3, 0.0, 0.4, 0.4,
                                  1.0, 1.0, 1.0, 1.0,
                                  false, 1.0, 0.0, 0.0);
    }
    check(disabled.sectionMetrics(OrganismStem::Core).rms > 0.0,
          "disabled shadow audit still exposes cheap section observation");
    check(disabled.sectionMetrics(OrganismStem::Drums).audibleRatio > 0.0,
          "cheap section observation preserves active direct additions");

    OrganismStemAuditor audit;
    audit.prepare(48000.0, true, false, 0.6, 0.6);
    for (int frame = 0; frame < 4096; ++frame) {
        const double core = std::sin(frame * 0.031) * 0.18;
        const double drums = (frame % 400) < 20 ? 0.12 : 0.0;
        audit.beginFrame();
        audit.addPreDelay(OrganismStem::Core, core);
        audit.closePreDelay(core, 1.0, 0.0, 0.2, 0.23, 0.0, 0.0);
        audit.addPreReverb(OrganismStem::Drums, drums, drums);
        const double mixed = core + drums;
        audit.closePreReverb(mixed, mixed);
        audit.processPostSpace(mixed, mixed, 0.0, 0.4, 0.4,
                               1.0 / 0.7, 1.0, 1.0, 1.0,
                               false, 1.0, 0.0, 0.0);
    }

    const auto coreMetrics = audit.metrics(OrganismStem::Core);
    const auto drumMetrics = audit.metrics(OrganismStem::Drums);
    const auto otherMetrics = audit.metrics(OrganismStem::Other);
    if (audit.maxPostReconstructionError() >= 1.0e-5
        || otherMetrics.preSpaceRms >= 1.0e-8
        || std::fabs(coreMetrics.postToPreDb) >= 0.01) {
        std::cerr << "diagnostic: error=" << audit.maxPostReconstructionError()
                  << " otherRms=" << otherMetrics.preSpaceRms
                  << " coreDeltaDb=" << coreMetrics.postToPreDb << "\n";
    }
    check(audit.frames() == 4096, "auditor counts completed frames");
    check(audit.maxPostReconstructionError() < 1.0e-5,
          "linear shadow stems reconstruct the declared post-space point");
    check(coreMetrics.preSpaceRms > drumMetrics.preSpaceRms,
          "independent desks retain different measured energy");
    check(drumMetrics.preAudibleRatio > 0.0 && drumMetrics.preAudibleRatio < 0.2,
          "event-shaped organism keeps its own occupancy");
    check(drumMetrics.lowBandRms > 0.0 || drumMetrics.midBandRms > 0.0
              || drumMetrics.highBandRms > 0.0,
          "direct stereo organism additions enter spectral telemetry");
    check(otherMetrics.preSpaceRms < 1.0e-8,
          "residual desk stays silent when every contribution is classified");
    check(std::fabs(coreMetrics.postToPreDb) < 0.01,
          "transparent space/master reports neutral survival gain");

    OrganismStemAuditor widened;
    widened.prepare(48000.0, true, false, 0.6, 0.6);
    for (int frame = 0; frame < 1024; ++frame) {
        const double left = std::sin(frame * 0.017) * 0.21;
        const double right = std::cos(frame * 0.023) * 0.13;
        widened.beginFrame();
        widened.addPreReverb(OrganismStem::Texture, left, right);
        widened.closePreReverb(left, right);
        const auto spaced = SpatialExpressionField::applyWidth({left, right}, 0.68);
        widened.processPostSpace(spaced.left, spaced.right, 0.0, 0.4, 0.4,
                                 1.0 / 0.7, 1.0, 1.0, 1.0,
                                 false, 1.0, 0.0, 0.0, 0.68);
    }
    check(widened.maxPostReconstructionError() < 1.0e-5,
          "post-space stems reconstruct after expressive width");

    const auto completedSection = audit.sectionSnapshot();
    const auto coreIndex = static_cast<std::size_t>(OrganismStem::Core);
    const auto drumsIndex = static_cast<std::size_t>(OrganismStem::Drums);
    check(completedSection[coreIndex].frames == 4096,
          "section snapshot captures the completed section frame count");
    check(completedSection[coreIndex].rms > completedSection[drumsIndex].rms,
          "section snapshot preserves independent organism evidence");
    audit.beginSection();
    check(audit.sectionMetrics(OrganismStem::Core).frames == 0,
          "beginSection clears only the live section accumulators");
    check(completedSection[coreIndex].frames == 4096
              && completedSection[coreIndex].rms > 0.0,
          "captured section remains immutable after the live reset");

    double energyShareSum = 0.0;
    for (int stem = 0; stem < static_cast<int>(OrganismStem::Count); ++stem) {
        const auto metrics = audit.metrics(static_cast<OrganismStem>(stem));
        check(metrics.preSpaceRms >= 0.0 && metrics.postSpaceRms >= 0.0,
              "stem RMS values never become negative");
        check(metrics.preSpacePeak >= 0.0 && metrics.postSpacePeak >= 0.0,
              "stem peak values never become negative");
        check(metrics.preAudibleRatio >= 0.0 && metrics.preAudibleRatio <= 1.0
                  && metrics.postAudibleRatio >= 0.0 && metrics.postAudibleRatio <= 1.0,
              "stem audible ratios stay within probability bounds");
        check(metrics.levelBuriedRatio >= 0.0 && metrics.levelBuriedRatio <= 1.0,
              "stem buried ratio stays within probability bounds");
        check(metrics.energyShare >= 0.0 && metrics.energyShare <= 1.0,
              "stem energy share stays within probability bounds");
        check(std::isfinite(metrics.lowBandRms)
                  && std::isfinite(metrics.midBandRms)
                  && std::isfinite(metrics.highBandRms)
                  && metrics.lowBandRms >= 0.0
                  && metrics.midBandRms >= 0.0
                  && metrics.highBandRms >= 0.0,
              "auditor spectral-band RMS values stay finite and nonnegative");
        energyShareSum += metrics.energyShare;
    }
    check(std::fabs(energyShareSum - 1.0) < 1.0e-9,
          "classified stem energy shares reconstruct the whole pre-space energy");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "OrganismStemAuditor tests passed\n";
    return EXIT_SUCCESS;
}
