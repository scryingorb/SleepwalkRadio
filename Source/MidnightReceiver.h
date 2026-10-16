#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class ReceiverLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ReceiverLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    juce::Label* createSliderTextBox(juce::Slider&) override;
    void drawLabel(juce::Graphics&, juce::Label&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawCornerResizer(juce::Graphics&, int, int, bool, bool) override;
};

// Cached independently of the moving overlay; redrawn at the current device scale by JUCE.
class ReceiverFaceplate final : public juce::Component
{
public:
    ReceiverFaceplate();
    void paint(juce::Graphics&) override;
};

class BroadcastDisplay final : public juce::Component
{
public:
    BroadcastDisplay();
    void update(float level, float wobble, float flutter, bool bypass, bool moving, double seconds);
    void paint(juce::Graphics&) override;
private:
    float level = 0, wobble = 0, flutter = 0;
    bool bypass = false;
    double phase = 0;
    juce::Path trace, indicator; // geometry storage reserved/built once
};
