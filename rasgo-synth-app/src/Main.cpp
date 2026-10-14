// Rasgo Synth App -- desktop entry point.
//
// Standalone JUCE application. Primary interaction is "configure a patch,
// press compor, get a WAV with its own seed embedded as metadata" (see
// PieceRenderer.hpp / the plan document for why); a TOCAR button added
// later plays the rendered buffer back through juce_audio_devices.

#include <cstdio>
#include <cstdlib>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ConsoleRedirect.h"
#include "RasgoPaths.h"
#include "MainComponent.h"
#include "ViewportLayout.h"

// Redirects the process's own stdout/stderr to a file before anything else
// runs, so native library chatter that never goes through JUCE's own
// logging -- ALSA/PipeWire warnings printed straight to stderr, a C++
// exception's default terminate message, an assert -- lands somewhere that
// can be read back afterward instead of only a terminal window that may
// already be closed by the time anyone looks. The helper shares the underlying
// descriptor; on Windows it first initializes CRT streams for a console-less
// GUI app. Deliberately separate from MainComponent's own structured
// rasgo_synth.log: this one is raw and low-level, that one is curated.
static void redirectConsoleToFile() {
    auto path = rasgo::paths::consoleLogFile();
    path.getParentDirectory().createDirectory();
    rasgo::diagnostics::redirectConsole(
        std::filesystem::u8path(path.getFullPathName().toRawUTF8()));
}

// Below a comfortable minimum size, MainComponent's own controls would
// start clipping or overlapping (see the resize-limit comment below) --
// rather than let that happen on a small monitor, this keeps
// MainComponent itself pinned at that comfortable minimum and lets a real
// scrollbar take over instead. Above the minimum, MainComponent fills the
// viewport exactly (its own resized() already scales up responsively) and
// no scrollbar appears at all.
class RasgoViewport : public juce::Viewport, private juce::Timer {
public:
    RasgoViewport() {
        setViewedComponent(&mainComponent_, false);
        if (std::getenv("RASGO_LAYOUT_AUDIT") != nullptr) startTimer(250);
    }
    rasgo::ui::MainComponent &mainComponent() { return mainComponent_; }

    void resized() override {
        const auto layout = rasgo::ui::ViewportLayout::fitContent(getWidth(), getHeight(), getScrollBarThickness(),
            [this](int width) { return mainComponent_.minimumLayoutHeight(width); });
        mainComponent_.setSize(layout.width, layout.height);
        juce::Viewport::resized();
    }

private:
    void timerCallback() override {
        if (++auditTicks_ > 120) { stopTimer(); return; } // at most30 s per opt-in run
        if (std::getenv("RASGO_LAYOUT_AUDIT_SCROLL_BOTTOM") != nullptr)
            setViewPosition(0, mainComponent_.getHeight());
        auto snapshot = mainComponent_.layoutAudit();
        auto* object = snapshot.getDynamicObject();
        object->setProperty("viewportWidth", getWidth());
        object->setProperty("viewportHeight", getHeight());
        object->setProperty("visibleWidth", getMaximumVisibleWidth());
        object->setProperty("visibleHeight", getMaximumVisibleHeight());
        object->setProperty("viewX", getViewPositionX());
        object->setProperty("viewY", getViewPositionY());
        object->setProperty("horizontalBar", getHorizontalScrollBar().isVisible());
        object->setProperty("verticalBar", getVerticalScrollBar().isVisible());
        const auto serialized = juce::JSON::toString(snapshot);
        if (serialized != lastAudit_) {
            rasgo::paths::appDataDirectory().getChildFile("layout-audit.json").replaceWithText(serialized);
            lastAudit_ = serialized;
        }
    }
    int auditTicks_ = 0;
    juce::String lastAudit_;
    rasgo::ui::MainComponent mainComponent_;
};

class RasgoSynthWindow : public juce::DocumentWindow {
public:
    RasgoSynthWindow(const juce::String &name)
        : juce::DocumentWindow(name,
                                juce::Colour(0xff174a2a), // matches Palette::bg (RasgoLookAndFeel.h)
                                juce::DocumentWindow::allButtons) {
        setUsingNativeTitleBar(true);
        setContentOwned(new RasgoViewport(), true);
        centreWithSize(getWidth(), getHeight());
        setResizable(true, false);
        // MainComponent::resized() scales its layout responsively (see the
        // comment there); this just keeps the window itself from being
        // dragged smaller than the point where controls would start
        // overlapping, while leaving no upper limit for a big 1920x1080+
        // desktop.
        setResizeLimits(900, 600, 10000, 10000);
        setVisible(true);
        // Opens maximized on the primary monitor. A first attempt set the
        // bounds to the primary display's user area BEFORE setVisible(true)
        // and then called setFullScreen(true) -- didn't work on this
        // machine's window manager (Mutter/Cinnamon): a brand-new window's
        // creation-time position hint isn't reliably honoured, it gets
        // placed on whichever monitor the WM's own "smart placement" policy
        // picks (empirically, always the secondary monitor here regardless
        // of mouse position), so by the time setFullScreen() ran and asked
        // "which display do I mostly overlap," it already had the wrong
        // answer. Fix: let the window get created/mapped wherever the WM
        // wants first, THEN explicitly force its bounds to the primary
        // display's user area -- an explicit move+resize of an
        // already-mapped window is honoured far more reliably by this WM
        // than a creation-time placement hint.
        setFullScreen(true);
        if (auto *primary = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
            setBounds(primary->userBounds.toNearestInt());
    }

    void closeButtonPressed() override {
        if (auto* viewport = dynamic_cast<RasgoViewport*>(getContentComponent()))
            viewport->mainComponent().requestClose([] { juce::JUCEApplication::getInstance()->quit(); });
    }
};

class RasgoSynthApplication : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "Rasgo Synth"; }
    const juce::String getApplicationVersion() override { return rasgo::ui::appVersion; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String &) override {
        redirectConsoleToFile();
        mainWindow_ = std::make_unique<RasgoSynthWindow>(getApplicationName());
    }

    void shutdown() override { mainWindow_ = nullptr; }

    void systemRequestedQuit() override {
        if (mainWindow_) mainWindow_->closeButtonPressed();
        else quit();
    }

private:
    std::unique_ptr<RasgoSynthWindow> mainWindow_;
};

START_JUCE_APPLICATION(RasgoSynthApplication)
