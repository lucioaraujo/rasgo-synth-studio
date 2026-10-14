#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "AccentComposition.h"
#include "LinuxKeyboardSymbols.h"

namespace rasgo::ui {
class AccentedTextEditor : public juce::TextEditor {
public:
    bool keyStateChanged(bool down) override {
#if JUCE_LINUX
        std::uint32_t held = 0;
        if (!isReadOnly() && !juce::ModifierKeys::getCurrentModifiers().isCtrlDown()
            && !juce::ModifierKeys::getCurrentModifiers().isCommandDown()) {
            for (auto symbol : AccentComposition::symbols) {
                if (juce::KeyPress::isKeyCurrentlyDown(static_cast<int>(symbol))
                    && keyboard_.resolveHeldSymbol(symbol) == symbol) { held = symbol; break; }
            }
        }
        if (held && held != heldAccent_) {
            heldAccent_ = held;
            insert(composition_.begin(held));
            return true;
        }
        heldAccent_ = held;
#endif
        return juce::TextEditor::keyStateChanged(down);
    }

    bool keyPressed(const juce::KeyPress& key) override {
#if JUCE_LINUX
        if (isReadOnly()) { composition_.reset(); return juce::TextEditor::keyPressed(key); }
        if (key.getModifiers().isCtrlDown() || key.getModifiers().isCommandDown()
            || key.getModifiers().isAltDown()) {
            composition_.reset();
            return juce::TextEditor::keyPressed(key);
        }
        auto character = static_cast<char32_t>(key.getTextCharacter());
        // XLookupString returns Latin-1 bytes, not UTF-8. Recover directly
        // typed Latin-1 glyphs from the active XKB layout, not from bad bytes.
        if (character >= 128) {
            for (std::uint32_t symbol = 0xa0; symbol <= 0xff; ++symbol) {
                if (juce::KeyPress::isKeyCurrentlyDown(static_cast<int>(symbol))
                    && keyboard_.resolveHeldSymbol(symbol) == symbol) {
                    character = static_cast<char32_t>(symbol);
                    insert(composition_.finish(character));
                    return true;
                }
            }
        }
        if (composition_.pending()) {
            if (key.getKeyCode() == juce::KeyPress::backspaceKey
                || key.getKeyCode() == juce::KeyPress::escapeKey) {
                composition_.reset(); return true;
            }
            if (character >= 32 && character != 127) {
                insert(composition_.finish(character)); return true;
            }
            composition_.reset(); // navigation/Tab/Return never carry an accent
        }
#endif
        return juce::TextEditor::keyPressed(key);
    }

    void focusLost(FocusChangeType cause) override {
        composition_.reset(); heldAccent_ = 0;
        juce::TextEditor::focusLost(cause);
    }
    void mouseDown(const juce::MouseEvent& event) override {
        composition_.reset();
        juce::TextEditor::mouseDown(event);
    }
private:
    void insert(AccentComposition::Text text) {
        if (!text.first) return;
        auto value = juce::String::charToString(static_cast<juce::juce_wchar>(text.first));
        if (text.second) value += juce::String::charToString(static_cast<juce::juce_wchar>(text.second));
        insertTextAtCaret(value); // preserves selection replacement and undo
    }
    AccentComposition composition_;
    LinuxKeyboardSymbols keyboard_;
    std::uint32_t heldAccent_ = 0;
};
} // namespace rasgo::ui
