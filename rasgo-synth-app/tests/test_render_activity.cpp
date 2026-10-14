#include "../src/RenderActivity.h"
#include "../src/AppVersion.h"
#include <cassert>
#include <limits>
#include <string_view>

int main() {
    using rasgo::ui::RenderActivity;
    assert(std::string_view(rasgo::ui::appVersion) == "0.1.0");
    assert(!std::string_view(rasgo::ui::appBuild).empty());
    RenderActivity activity;
    activity.begin(1000.0);
    assert(RenderActivity::indeterminate < 0.0);
    assert(activity.audioSeconds == 0.0 && !activity.album);
    assert(activity.compositionFraction() == RenderActivity::indeterminate);
    activity.observeWork(999, 1000, rasgo::PieceRenderPhase::Composition);
    assert(activity.compositionPercent() == 99);
    activity.observeWork(1000, 1000, rasgo::PieceRenderPhase::Finalizing);
    assert(activity.compositionPercent() == 100);
    assert(activity.phase == rasgo::PieceRenderPhase::Finalizing);
    activity.observe(90.0);
    assert(activity.elapsedSeconds(6000.0) == 5.0);
    assert(activity.audioSeconds == 90.0); // audio time is not wall time
    assert(activity.elapsedSeconds(500.0) == 0.0);
    // Clock keeps advancing without callbacks (mastering / writing included).
    assert(activity.elapsedSeconds(11000.0) == 10.0);
    assert(activity.audioSeconds == 90.0);
    activity.begin(20000.0, true, 4);
    assert(activity.compositionFraction() == RenderActivity::indeterminate);
    assert(activity.album && activity.calibration && activity.audioSeconds == 0.0);
    activity.observe(240.0, 3, 4, true);
    assert(activity.trackIndex == 3 && activity.calibration);
    activity.observe(0.0, 0, 4, false); // all calibration tracks precede finals
    assert(activity.trackIndex == 0 && !activity.calibration);
    assert(activity.audioSeconds == 0.0);
    assert(activity.elapsedSeconds(32000.0) == 12.0); // no reset between phases
    activity.observe(-1.0, 99, 0);
    assert(activity.audioSeconds == 0.0 && activity.trackCount == 1 && activity.trackIndex == 0);
    activity.observe(std::numeric_limits<double>::quiet_NaN());
    assert(activity.audioSeconds == 0.0);
    activity.begin(40000.0);
    assert(!activity.album && activity.elapsedSeconds(40000.0) == 0.0);
}
