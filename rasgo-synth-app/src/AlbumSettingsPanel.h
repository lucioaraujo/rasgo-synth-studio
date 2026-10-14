#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "UiLanguage.h"
#include "RasgoLookAndFeel.h"
#include "../../rasgo-synth-core/src/engine/AlbumPlan.hpp"

namespace rasgo::ui {

// All controls are created once. MainComponent changes visibility only;
// switching between composition and listening never recreates audio state.
class AlbumSettingsPanel final : public juce::Component {
public:
    AlbumSettingsPanel() {
        for (auto* label : {&title_, &tempoTitle_, &durationTitle_, &spreadLabel_,
                            &contourLabel_, &targetLabel_, &distributionLabel_, &help_, &directionHelp_}) {
            label->setFont(Palette::mono(14.0f));
            label->setMinimumHorizontalScale(1.0f);
            label->setColour(juce::Label::textColourId, Palette::textBright);
            addAndMakeVisible(*label);
        }
        for (auto* button : {&varyBpm_, &planDuration_, &direction_}) {
            button->setClickingTogglesState(true);
            button->setColour(juce::TextButton::buttonOnColourId, Palette::copper);
            button->onClick = [this] { refreshPreview(); };
            addAndMakeVisible(*button);
        }
        direction_.onClick = [this] {
            refreshPreview();
            if (onDirectionChanged) onDirectionChanged(direction_.getToggleState());
        };
        for (auto* slider : {&spread_, &targetMinutes_}) {
            slider->setSliderStyle(juce::Slider::LinearHorizontal);
            slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 90, 30);
            slider->onValueChange = [this] { refreshPreview(); };
            addAndMakeVisible(*slider);
        }
        spread_.setRange(0.0, 35.0, 1.0);
        spread_.setValue(15.0, juce::dontSendNotification);
        spread_.setTextValueSuffix(" %");
        targetMinutes_.setRange(1.0, 45.0, 1.0);
        targetMinutes_.setValue(20.0, juce::dontSendNotification);
        targetMinutes_.setTextValueSuffix(" min");
        for (auto* box : {&contour_, &distribution_}) {
            box->onChange = [this] { refreshPreview(); };
            addAndMakeVisible(*box);
        }
        preview_.setReadOnly(true);
        preview_.setMultiLine(true, true);
        preview_.setScrollbarsShown(true);
        preview_.setFont(Palette::mono(14.0f));
        preview_.setColour(juce::TextEditor::backgroundColourId, Palette::bgRecessed);
        addAndMakeVisible(preview_);
        setLanguage(Language::english);
    }

    AlbumOptions options() const {
        AlbumOptions value;
        value.musicalDirectionEnabled = direction_.getToggleState();
        value.varyBpm = varyBpm_.getToggleState();
        value.bpmSpread = spread_.getValue() / 100.0;
        value.tempoContour = static_cast<AlbumTempoContour>(juce::jmax(0, contour_.getSelectedId() - 1));
        value.targetDurationSeconds = planDuration_.getToggleState() ? targetMinutes_.getValue() * 60.0 : 0.0;
        value.durationDistribution = static_cast<AlbumDurationDistribution>(juce::jmax(0, distribution_.getSelectedId() - 1));
        return value;
    }

    std::function<void(bool)> onDirectionChanged;
    void setDirectionEnabled(bool enabled) {
        direction_.setToggleState(enabled, juce::dontSendNotification);
        refreshPreview();
    }

    void setContext(unsigned seed, int count, double bpm) {
        seed_ = seed; count_ = juce::jlimit(1, 40, count); baseBpm_ = bpm;
        refreshPreview();
    }

    void setLanguage(Language language) {
        language_ = language;
        title_.setText(text(strings::albumPlanTitle, language), juce::dontSendNotification);
        direction_.setButtonText(text(strings::musicalDirection, language));
        directionHelp_.setText(text(strings::musicalDirectionHelp, language), juce::dontSendNotification);
        tempoTitle_.setText(text(strings::albumTempoTitle, language), juce::dontSendNotification);
        durationTitle_.setText(text(strings::albumDurationTitle, language), juce::dontSendNotification);
        varyBpm_.setButtonText(text(strings::albumVaryBpm, language));
        planDuration_.setButtonText(text(strings::albumDurationEnabled, language));
        spreadLabel_.setText(text(strings::albumBpmSpread, language), juce::dontSendNotification);
        contourLabel_.setText(text(strings::albumTempoContour, language), juce::dontSendNotification);
        targetLabel_.setText(text(strings::albumDurationTarget, language), juce::dontSendNotification);
        distributionLabel_.setText(text(strings::albumDurationDistribution, language), juce::dontSendNotification);
        help_.setText(text(strings::albumPlanHelp, language), juce::dontSendNotification);
        const int contourId = juce::jmax(1, contour_.getSelectedId());
        const int distributionId = juce::jmax(1, distribution_.getSelectedId());
        contour_.clear(juce::dontSendNotification);
        contour_.addItem(text(strings::albumBalanced, language), 1);
        contour_.addItem(text(strings::albumContrasting, language), 2);
        contour_.addItem(text(strings::albumRising, language), 3);
        contour_.addItem(text(strings::albumArch, language), 4);
        contour_.setSelectedId(contourId, juce::dontSendNotification);
        distribution_.clear(juce::dontSendNotification);
        distribution_.addItem(text(strings::albumBalanced, language), 1);
        distribution_.addItem(text(strings::albumContrasting, language), 2);
        distribution_.addItem(text(strings::albumFree, language), 3);
        distribution_.setSelectedId(distributionId, juce::dontSendNotification);
        refreshPreview();
    }

    static int preferredHeight(int width) { return width >= 700 ? 650 : 910; }
    void resized() override {
        auto area = getLocalBounds().reduced(16);
        title_.setBounds(area.removeFromTop(36));
        direction_.setBounds(area.removeFromTop(36));
        directionHelp_.setBounds(area.removeFromTop(getWidth() >= 700 ? 66 : 86));
        area.removeFromTop(8);
        auto controls = area.removeFromTop(getWidth() >= 700 ? 224 : 448);
        auto tempo = controls, duration = controls;
        if (getWidth() >= 700) {
            tempo = controls.removeFromLeft((controls.getWidth() - 24) / 2);
            controls.removeFromLeft(24); duration = controls;
        } else {
            tempo = controls.removeFromTop(224); duration = controls;
        }
        auto layout = [](juce::Rectangle<int> col, juce::Label& heading, juce::TextButton& enabled,
                         juce::Label& first, juce::Slider& slider, juce::Label& second, juce::ComboBox& box) {
            heading.setBounds(col.removeFromTop(24));
            enabled.setBounds(col.removeFromTop(36)); col.removeFromTop(10);
            first.setBounds(col.removeFromTop(26)); slider.setBounds(col.removeFromTop(36));
            col.removeFromTop(10); second.setBounds(col.removeFromTop(26)); box.setBounds(col.removeFromTop(36));
        };
        layout(tempo, tempoTitle_, varyBpm_, spreadLabel_, spread_, contourLabel_, contour_);
        layout(duration, durationTitle_, planDuration_, targetLabel_, targetMinutes_, distributionLabel_, distribution_);
        help_.setBounds(area.removeFromTop(92)); area.removeFromTop(8);
        preview_.setBounds(area);
    }
    void paint(juce::Graphics& graphics) override {
        graphics.setColour(Palette::bgPanel);
        graphics.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
    }

private:
    static juce::String durationText(double seconds) {
        const int total = static_cast<int>(std::lround(seconds));
        return juce::String(total / 60) + ":" + juce::String(total % 60).paddedLeft('0', 2);
    }
    void refreshPreview() {
        spread_.setEnabled(varyBpm_.getToggleState());
        contour_.setEnabled(varyBpm_.getToggleState());
        targetMinutes_.setEnabled(planDuration_.getToggleState());
        distribution_.setEnabled(planDuration_.getToggleState());
        const auto plan = makeAlbumPlan(seed_, count_, baseBpm_, options());
        juce::String value = plan.effectiveDurationSeconds > 0.0
            ? text(strings::albumRequestedTotal, language_) + " " + durationText(plan.requestedDurationSeconds)
                + " | " + text(strings::albumPlannedTotal, language_) + " " + durationText(plan.effectiveDurationSeconds)
            : text(strings::albumEmergentDuration, language_);
        value += "\n\n";
        for (std::size_t index = 0; index < plan.tracks.size(); ++index) {
            value += juce::String(static_cast<int>(index + 1)).paddedLeft('0', 2)
                + "    " + juce::String(plan.tracks[index].bpm, 1) + " BPM    "
                + (plan.tracks[index].targetDurationSeconds > 0.0
                    ? durationText(plan.tracks[index].targetDurationSeconds)
                    : text(strings::albumEmergentDuration, language_)) + "\n";
        }
        preview_.setText(value, false);
    }
    Language language_ = Language::english;
    unsigned seed_ = 2026u;
    int count_ = 8;
    double baseBpm_ = 100.0;
    juce::Label title_, tempoTitle_, durationTitle_, spreadLabel_, contourLabel_, targetLabel_, distributionLabel_, help_, directionHelp_;
    juce::TextButton varyBpm_, planDuration_, direction_;
    juce::Slider spread_, targetMinutes_;
    juce::ComboBox contour_, distribution_;
    juce::TextEditor preview_;
};

} // namespace rasgo::ui
