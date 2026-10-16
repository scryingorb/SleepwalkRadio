#include "PluginEditor.h"

SleepwalkRadioEditor::SleepwalkRadioEditor(SleepwalkRadioProcessor& p)
    : AudioProcessorEditor(p), processor(p)
{
    setLookAndFeel(&look);
    setName("SleepwalkRadio Midnight Receiver");
    setTitle("SleepwalkRadio Midnight Receiver");
    addAndMakeVisible(canvas);
    canvas.setComponentID("receiver-panel");
    canvas.addAndMakeVisible(faceplate);
    canvas.addAndMakeVisible(display);
    // Visual ordering groups character, motion, then signal; host parameter ordering is unchanged.
    constexpr std::array<int,8> x {42,382,486,146,250,618,722,826};
    constexpr std::array<int,8> focusOrder {1,4,5,2,3,6,7,8};
    static constexpr std::array<const char*,8> tips {{
        "Smooth tape saturation with compensated drive.", "Slow, shared stereo tape warping.",
        "Faster, delicate transport instability.", "Softens the high frequencies.",
        "Independent shaped tape hiss; zero is silent.", "Linear blend with aligned dry audio.",
        "Final output trim; also applies at Mix zero.", "Off: tape engine active. On: latency-aligned dry audio. Space toggles bypass." }};
    for (size_t i=0;i<knobs.size();++i)
    {
        auto& knob=knobs[i]; const auto& spec=parameterSpecs[i];
        knob.setComponentID(spec.id); knob.setName(spec.name); knob.setTitle(spec.name);
        knob.setDescription(tips[i]); knob.setWantsKeyboardFocus(true);
        knob.setExplicitFocusOrder(focusOrder[i]);
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setMouseDragSensitivity(220);
        knob.setVelocityModeParameters(0.25,1,0,true,juce::ModifierKeys::shiftModifier);
        knob.setTextBoxStyle(juce::Slider::TextBoxBelow,false,82,24);
        if(i<7) knob.setTextValueSuffix(juce::String(" ")+spec.unit);
        knob.setTooltip(juce::String(tips[i])+" Shift-drag: fine. Double-click: reset. Click value: type.");
        labels[i].setText(spec.name,juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        labels[i].setFont(juce::Font(juce::FontOptions(13.5f)));
        labels[i].setColour(juce::Label::textColourId,juce::Colour(0xffe8ddc8));
        // Labels use plain printed text; only value fields use the recessed LookAndFeel label style.
        labels[i].setLookAndFeel(&juce::LookAndFeel::getDefaultLookAndFeel());
        labels[i].setInterceptsMouseClicks(false,false);
        canvas.addAndMakeVisible(knob); canvas.addAndMakeVisible(labels[i]);
        labels[i].setBounds(x[i],358,92,22);
        knob.setBounds(x[i],382,92,112);
        attachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,spec.id,knob);
        knob.setDoubleClickReturnValue(true,spec.initial);
        knob.onValueChange=[this] { refreshDisplay(false); };
    }
    motion.setComponentID("display-motion"); motion.setTitle("Display animation");
    motion.setDescription("Visual motion only. Does not change audio.");
    motion.setTooltip("Disable visual motion. Audio processing and all controls remain available.");
    motion.setClickingTogglesState(true); motion.setWantsKeyboardFocus(true); motion.setExplicitFocusOrder(9);
    motion.setToggleState(processor.motionEnabled.load(),juce::dontSendNotification);
    motion.onClick=[this] { setAnimationEnabled(motion.getToggleState()); };
    canvas.addAndMakeVisible(motion); motion.setBounds(798,513,114,25);
    canvas.setSize(960,560); faceplate.setBounds(canvas.getLocalBounds()); display.setBounds(canvas.getLocalBounds());
    setResizable(true,true); setResizeLimits(768,448,1440,840);
    if(auto* resizeConstraints=getConstrainer()) resizeConstraints->setFixedAspectRatio(960.0/560.0);
    setSize(960,560);
    setAnimationEnabled(motion.getToggleState());
}

SleepwalkRadioEditor::~SleepwalkRadioEditor()
{
    stopTimer();
    if(observing) processor.removeVisualObserver();
    for(auto& knob:knobs) knob.onValueChange=nullptr;
    motion.onClick=nullptr;
    // Detach the member LookAndFeel while all children still exist.
    setLookAndFeel(nullptr);
}

juce::Slider& SleepwalkRadioEditor::control(const juce::String& id)
{
    for(size_t i=0;i<parameterSpecs.size();++i) if(id==parameterSpecs[i].id) return knobs[i];
    jassertfalse; return knobs[0];
}

void SleepwalkRadioEditor::paint(juce::Graphics& g) { g.fillAll(juce::Colour(0xff090c0e)); }
void SleepwalkRadioEditor::resized()
{
    const float scale=juce::jmin(getWidth()/960.0f,getHeight()/560.0f);
    canvas.setTransform(juce::AffineTransform::scale(scale).translated((getWidth()-960*scale)*0.5f,(getHeight()-560*scale)*0.5f));
}
void SleepwalkRadioEditor::visibilityChanged()
{
    if(isAnimationEnabled()) startTimerHz(isShowing()?30:4);
}
void SleepwalkRadioEditor::setAnimationEnabled(bool enabled)
{
    processor.motionEnabled.store(enabled);
    motion.setToggleState(enabled,juce::dontSendNotification);
    if(enabled && !observing) { processor.addVisualObserver(); observing=true; }
    if(!enabled && observing) { processor.removeVisualObserver(); observing=false; }
    if(enabled) { lastTick=juce::Time::getMillisecondCounterHiRes(); startTimerHz(isShowing()?30:4); }
    else { stopTimer(); meter=0; }
    refreshDisplay(false);
}
void SleepwalkRadioEditor::refreshDisplay(bool advance)
{
    const auto snapshot=processor.readVisualSnapshot();
    const double now=juce::Time::getMillisecondCounterHiRes();
    const double seconds=juce::jlimit(0.0,0.25,(now-lastTick)*0.001);
    lastTick=now;
    if(advance)
    {
        const float target=snapshot.serial!=lastSerial?snapshot.peak:0.0f;
        lastSerial=snapshot.serial;
        meter += (target-meter)*static_cast<float>(target>meter?0.65:1.0-std::exp(-seconds*5.0));
        if(meter<0.0001f) meter=0;
    }
    const bool bypass=knobs[7].getValue()>0.5 || (advance && snapshot.hostBypassed && snapshot.serial==lastSerial);
    display.update(meter,static_cast<float>(knobs[1].getValue()*0.01),static_cast<float>(knobs[2].getValue()*0.01),bypass,
                   advance && meter>0.0001f,seconds);
    if(advance) startTimerHz(meter>0.0001f?30:4);
}
void SleepwalkRadioEditor::timerCallback()
{
    if(!isShowing()) { startTimerHz(4); return; }
    refreshDisplay(true);
}
