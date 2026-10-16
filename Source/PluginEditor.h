#pragma once
#include "PluginProcessor.h"
#include "MidnightReceiver.h"

class ParameterKnob final : public juce::Slider
{
public:
    bool keyPressed(const juce::KeyPress& key) override
    {
        const bool toggle = getName() == "Bypass" && key == juce::KeyPress::spaceKey;
        if (!key.getModifiers().isAnyModifierKeyDown()
            && (toggle || key == juce::KeyPress::leftKey || key == juce::KeyPress::rightKey
                || key == juce::KeyPress::upKey || key == juce::KeyPress::downKey))
        {
            // JUCE's arrow-key path needs explicit begin/end gesture notifications.
            juce::Slider::ScopedDragNotification gesture(*this);
            if (toggle) { setValue(getValue() > 0.5 ? 0.0 : 1.0, juce::sendNotificationSync); return true; }
            return juce::Slider::keyPressed(key);
        }
        return juce::Slider::keyPressed(key);
    }
};

class SleepwalkRadioEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SleepwalkRadioEditor(SleepwalkRadioProcessor&);
    ~SleepwalkRadioEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;
    void setAnimationEnabled(bool);
    bool isAnimationEnabled() const noexcept { return motion.getToggleState(); }
    bool isAnimationTimerRunning() const noexcept { return isTimerRunning(); }
    juce::Slider& control(const juce::String& id);
private:
    void timerCallback() override;
    void refreshDisplay(bool advance);
    SleepwalkRadioProcessor& processor;
    ReceiverLookAndFeel look;
    juce::Component canvas;
    ReceiverFaceplate faceplate;
    BroadcastDisplay display;
    std::array<ParameterKnob, 8> knobs;
    std::array<juce::Label, 8> labels;
    juce::TextButton motion { "Motion" };
    juce::TooltipWindow tooltips { this, 600 };
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 8> attachments;
    uint32_t lastSerial = 0;
    float meter = 0;
    double lastTick = 0;
    bool observing = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SleepwalkRadioEditor)
};
