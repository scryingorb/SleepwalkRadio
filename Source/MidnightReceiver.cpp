#include "MidnightReceiver.h"
#include <cmath>

namespace
{
const juce::Colour ivory(0xffe8ddc8), amber(0xffe9a34b), brass(0xff8f7851);
void text(juce::Graphics& g, const juce::String& value, juce::Rectangle<float> bounds,
          float size, juce::Colour colour, int justification = juce::Justification::centredLeft, bool bold = false)
{
    g.setColour(colour);
    g.setFont(juce::Font(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain)));
    g.drawText(value, bounds, justification);
}
juce::Point<float> radial(float cx, float cy, float radius, float angle)
{
    return { cx + radius * std::sin(angle), cy - radius * std::cos(angle) };
}
void screw(juce::Graphics& g, float x, float y)
{
    g.setColour(juce::Colours::black.withAlpha(0.65f)); g.fillEllipse(x-6, y-5, 13, 13);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff707577), x-4, y-5, juce::Colour(0xff212629), x+5, y+5, false));
    g.fillEllipse(x-5,y-5,10,10);
    g.setColour(juce::Colour(0xff111416)); g.drawLine(x-3,y+2,x+3,y-2,1.6f);
    g.setColour(juce::Colours::white.withAlpha(0.16f)); g.drawLine(x-3,y+3,x+3,y-1,0.6f);
}
}

ReceiverLookAndFeel::ReceiverLookAndFeel()
{
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff171a1c));
    setColour(juce::PopupMenu::textColourId, ivory);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff57432a));
    setColour(juce::PopupMenu::highlightedTextColourId, ivory);
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff151719));
    setColour(juce::TextEditor::textColourId, ivory);
    setColour(juce::TextEditor::highlightColourId, amber.withAlpha(0.3f));
    setColour(juce::TextEditor::focusedOutlineColourId, amber);
    setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(0xff24282b));
    setColour(juce::TooltipWindow::textColourId, ivory);
    setColour(juce::TooltipWindow::outlineColourId, brass);
}

void ReceiverLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float value, float start, float end, juce::Slider& slider)
{
    const float cx = x + width * 0.5f, cy = y + height * 0.5f;
    const float radius = juce::jmin(width, height) * 0.5f - 12.0f;
    const float angle = start + (end - start) * value;
    const bool focus = slider.hasKeyboardFocus(true), hover = slider.isMouseOverOrDragging();
    const bool binary = slider.getName() == "Bypass";
    const int divisions = binary ? 1 : 10;
    for (int i=0; i<=divisions; ++i)
    {
        const float a = start + (end-start) * static_cast<float>(i) / divisions;
        g.setColour(i <= value * divisions ? amber.withAlpha(0.7f) : ivory.withAlpha(0.26f));
        g.drawLine({ radial(cx,cy,radius+5,a), radial(cx,cy,radius+(i%5==0?10.0f:8.0f),a) }, 1.0f);
    }
    g.setColour(juce::Colours::black.withAlpha(0.5f)); g.fillEllipse(cx-radius-3,cy-radius+1,2*radius+6,2*radius+6);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff747979),cx-radius,cy-radius,
                                        juce::Colour(0xff101214),cx+radius,cy+radius,false));
    g.fillEllipse(cx-radius,cy-radius,2*radius,2*radius);
    for (int i=0;i<64;++i)
    {
        const float a = juce::MathConstants<float>::twoPi*i/64.0f;
        const float light = 0.16f + 0.14f*std::cos(a+0.8f);
        g.setColour(juce::Colours::white.withAlpha(light));
        g.drawLine({radial(cx,cy,radius-3,a),radial(cx,cy,radius-0.7f,a)},0.8f);
    }
    const float cap = radius-5;
    g.setColour(juce::Colour(0xff080b0d)); g.fillEllipse(cx-cap-1,cy-cap-1,2*cap+2,2*cap+2);
    g.setGradientFill(juce::ColourGradient(juce::Colour(hover?0xff5e6466:0xff505658),cx-cap,cy-cap,
                                         juce::Colour(0xff202426),cx+cap,cy+cap,false));
    g.fillEllipse(cx-cap,cy-cap,2*cap,2*cap);
    // Fine radial machining highlights on a dark metal cap.
    for (int i=0;i<72;++i)
    {
        const float a=juce::MathConstants<float>::twoPi*i/72.0f;
        g.setColour(juce::Colours::white.withAlpha(0.018f+0.035f*std::abs(std::cos(a-0.7f))));
        g.drawLine({radial(cx,cy,cap*0.08f,a),radial(cx,cy,cap-1,a)},0.45f);
    }
    g.setColour(juce::Colours::white.withAlpha(0.14f)); g.drawEllipse(cx-cap,cy-cap,2*cap,2*cap,0.7f);
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.drawLine({radial(cx+0.8f,cy+1,cap*0.48f,angle),radial(cx+0.8f,cy+1,cap-3,angle)},3.2f);
    g.setColour(hover || focus ? amber.brighter(0.2f) : ivory);
    g.drawLine({radial(cx,cy,cap*0.48f,angle),radial(cx,cy,cap-3,angle)},2.3f);
    if (focus)
    {
        g.setColour(amber.withAlpha(0.9f)); g.drawEllipse(cx-radius-12,cy-radius-12,2*radius+24,2*radius+24,1.3f);
    }
}

juce::Label* ReceiverLookAndFeel::createSliderTextBox(juce::Slider& slider)
{
    auto* label = juce::LookAndFeel_V4::createSliderTextBox(slider);
    label->setFont(juce::Font(juce::FontOptions(13.5f)));
    label->setColour(juce::Label::textColourId, ivory);
    label->setColour(juce::Label::textWhenEditingColourId, ivory);
    label->setColour(juce::Label::backgroundWhenEditingColourId, juce::Colour(0xff151719));
    label->setColour(juce::Label::outlineWhenEditingColourId, amber);
    return label;
}

void ReceiverLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    const auto bounds=label.getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(juce::Colour(0xff171a1c)); g.fillRoundedRectangle(bounds,3);
    g.setColour(juce::Colour(0xff414649)); g.drawRoundedRectangle(bounds,3,0.7f);
    if (!label.isBeingEdited())
        text(g,label.getText(),bounds,13.5f,ivory,juce::Justification::centred);
}

void ReceiverLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&, bool over, bool)
{
    auto b=button.getLocalBounds().toFloat().reduced(1);
    g.setColour(juce::Colour(0xff161a1c)); g.fillRoundedRectangle(b,4);
    g.setColour(button.hasKeyboardFocus(true) || over ? amber : brass.withAlpha(0.6f)); g.drawRoundedRectangle(b,4,1);
    g.setColour(button.getToggleState()?amber:brass.darker(0.5f)); g.fillEllipse(9,10,5,5);
}
void ReceiverLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    text(g,button.getComponentID()=="factory-preset" ? button.getButtonText() + "  v" : (button.getToggleState()?"MOTION ON":"MOTION OFF"),button.getLocalBounds().toFloat().withTrimmedLeft(18),button.getComponentID()=="factory-preset"?13.0f:10.5f,ivory,juce::Justification::centred);
}
void ReceiverLookAndFeel::drawCornerResizer(juce::Graphics& g,int width,int height,bool,bool)
{
    g.setColour(brass.withAlpha(0.5f));
    for(int i=4;i<12;i+=4) g.drawLine(static_cast<float>(width-i),static_cast<float>(height-2),static_cast<float>(width-2),static_cast<float>(height-i),1);
}

ReceiverFaceplate::ReceiverFaceplate() { setInterceptsMouseClicks(false,false); setBufferedToImage(true); }
void ReceiverFaceplate::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff090c0e)); g.fillRoundedRectangle(6,9,948,547,22);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff555c5f),0,0,juce::Colour(0xff111517),0,560,false));
    g.fillRoundedRectangle(8,6,944,544,22);
    g.setColour(juce::Colour(0xff111618)); g.fillRoundedRectangle(17,15,926,526,16);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff343a3d),0,28,juce::Colour(0xff202426),0,530,false));
    g.fillRoundedRectangle(25,23,910,510,10);
    g.saveState(); g.reduceClipRegion(juce::Rectangle<int>(27,27,906,502));
    for (int y=28;y<530;y+=2)
    {
        g.setColour(juce::Colours::white.withAlpha(y%6==0?0.019f:0.009f)); g.drawHorizontalLine(y,28,932);
    }
    juce::Random grain(0x52414449);
    for(int i=0;i<1800;++i)
    {
        const float x=28+grain.nextFloat()*904, y=28+grain.nextFloat()*500;
        g.setColour(juce::Colours::white.withAlpha(0.025f)); g.drawLine(x,y,x+2+grain.nextFloat()*8,y,0.5f);
    }
    g.restoreState();
    g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawRoundedRectangle(25,23,910,510,10,0.7f);
    text(g,"SLEEPWALKRADIO",{50,42,590,42},32,ivory,juce::Justification::centredLeft,true);
    text(g,"M I D N I G H T   R E C E I V E R",{53,86,520,20},11.5f,brass.brighter(0.35f));
    for(int i=0;i<17;++i)
    {
        g.setColour(juce::Colour(0xff131719)); g.fillRoundedRectangle(681.0f+i*13,47,6,18,3);
        g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawLine(682.0f+i*13,66,685.0f+i*13,66,1);
    }
    text(g,"STEREO TAPE  /  MR-03",{678,84,168,18},10,ivory.withAlpha(0.55f));
    // Deep bezel, warm illuminated glass, restrained edge reflections.
    g.setColour(juce::Colours::black.withAlpha(0.4f)); g.fillRoundedRectangle(40,129,880,180,9);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff111415),0,124,juce::Colour(0xff59564d),0,306,false));
    g.fillRoundedRectangle(42,124,876,178,8);
    g.setColour(juce::Colour(0xff080b0c)); g.fillRoundedRectangle(47,129,866,168,5);
    juce::ColourGradient glass(juce::Colour(0xff30291c),0,134,juce::Colour(0xff3b2b1c),0,292,false);
    glass.addColour(0.48,juce::Colour(0xff79512b));
    g.setGradientFill(glass); g.fillRoundedRectangle(51,133,858,160,3);
    g.setGradientFill(juce::ColourGradient(amber.withAlpha(0.15f),500,209,amber.withAlpha(0.0f),870,209,true));
    g.fillRect(220,134,688,158);
    for(int y=137;y<291;y+=3)
    { g.setColour(juce::Colours::black.withAlpha(0.06f)); g.drawHorizontalLine(y,53,907); }
    // The signal meter is level-driven; the station scale is explicitly decorative.
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffb6a06e),140,145,juce::Colour(0xff5a492d),140,279,false));
    g.fillRoundedRectangle(64,147,153,131,3);
    g.setColour(juce::Colour(0xff26271f)); g.drawRoundedRectangle(65,148,151,129,3,1);
    text(g,"SIGNAL",{75,156,130,19},10,juce::Colour(0xff302c21),juce::Justification::centred,true);
    for(int i=0;i<=20;++i)
    {
        const float a=-0.95f+i*0.095f;
        g.setColour(i>16?juce::Colour(0xff713a21):juce::Colour(0xff383426));
        g.drawLine({radial(140,260,i%5==0?63.0f:67.0f,a),radial(140,260,73,a)},i%5==0?1.3f:0.65f);
        if(i%5==0)
        {
            const auto p=radial(140,260,84,a);
            text(g,juce::String(i/5),{p.x-9,p.y-6,18,12},9,juce::Colour(0xff332e22),juce::Justification::centred);
        }
    }
    text(g,"OUTPUT LEVEL",{76,259,128,12},8.5f,juce::Colour(0xff302a20),juce::Justification::centred);
    text(g,"D R E A M   B R O A D C A S T",{244,145,420,16},11,ivory);
    text(g,"NIGHT BAND",{778,145,106,16},9.5f,amber,juce::Justification::centredRight);
    for(int i=0;i<=100;++i)
    {
        const float x=246+6.36f*i;
        const float bottom=i%10==0?220.0f:(i%5==0?213.0f:207.0f);
        g.setColour(ivory.withAlpha(i%5==0?0.85f:0.48f)); g.drawLine(x,199,x,bottom,i%10==0?1.2f:0.6f);
        if(i%10==0) text(g,juce::String(i),{x-15,173,30,20},13,ivory,juce::Justification::centred);
    }
    g.setColour(amber.withAlpha(0.45f)); g.drawLine(246,223,882,223,0.7f);
    g.setColour(ivory.withAlpha(0.13f)); g.drawLine(246,249,882,249,0.6f);
    text(g,"DECORATIVE TUNING SCALE",{246,273,350,12},8.5f,ivory.withAlpha(0.62f));
    text(g,"SLOW SIGNALS / LATE HOURS",{656,273,226,12},8.5f,ivory.withAlpha(0.62f),juce::Justification::centredRight);
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.12f),0,134,juce::Colours::white.withAlpha(0.0f),0,168,false));
    g.fillRect(53,134,854,35);
    g.setColour(amber.withAlpha(0.19f)); g.drawRoundedRectangle(51,133,858,160,3,0.8f);
    const std::array<juce::Rectangle<float>,3> groups {{{32,320,320,184},{368,320,224,184},{608,320,320,184}}};
    const std::array<const char*,3> names {"TAPE CHARACTER","TAPE MOVEMENT","OUTPUT / SIGNAL"};
    for(size_t i=0;i<groups.size();++i)
    {
        auto b=groups[i];
        g.setColour(juce::Colours::black.withAlpha(0.15f)); g.fillRoundedRectangle(b,5);
        g.setColour(juce::Colours::white.withAlpha(0.06f)); g.drawRoundedRectangle(b,5,0.6f);
        text(g,names[i],{b.getX()+16,330,b.getWidth()-32,16},10,brass.brighter(0.5f));
        g.setColour(brass.withAlpha(0.22f)); g.drawLine(b.getX()+16,352,b.getRight()-16,352,0.7f);
    }
    text(g,"STEREO TAPE PROCESSOR",{48,515,240,18},9.5f,ivory.withAlpha(0.5f));

    screw(g,28,28); screw(g,932,28); screw(g,28,532); screw(g,932,532);
}

BroadcastDisplay::BroadcastDisplay()
{
    setInterceptsMouseClicks(false,false); trace.preallocateSpace(1024);
    indicator.addTriangle(-4,160,4,160,0,167);
}
void BroadcastDisplay::update(float newLevel,float newWobble,float newFlutter,bool newBypass,bool moving,double seconds)
{
    const float oldLevel=level;
    const bool changed = bypass!=newBypass || wobble!=newWobble || flutter!=newFlutter;
    level=newLevel; wobble=newWobble; flutter=newFlutter; bypass=newBypass;
    if(moving) phase=std::fmod(phase+seconds,1000.0);
    if(moving || changed || std::abs(level-oldLevel)>0.0001f)
    { repaint(51,133,858,160); repaint(844,75,67,34); }
}
void BroadcastDisplay::paint(juce::Graphics& g)
{
    const float energy=juce::jlimit(0.0f,1.0f,std::sqrt(level));
    const float offset=static_cast<float>(std::sin(phase*1.45)*wobble*12 + std::sin(phase*39.6)*flutter*1.4);
    const float needle=563+offset;
    g.setColour(amber.withAlpha(bypass?0.04f:0.12f)); g.fillRect(needle-4,165.0f,8.0f,106.0f);
    g.setColour(bypass?brass:amber.brighter(0.4f)); g.fillRect(needle-0.8f,163.0f,1.6f,109.0f);
    g.fillPath(indicator,juce::AffineTransform::translation(needle,0));
    trace.clear();
    for(int i=0;i<=160;++i)
    {
        const float x=246+636.0f*i/160;
        const double u=i/160.0;
        const float amplitude=(bypass?2.0f:4.0f+energy*10.0f)*static_cast<float>(std::sin(u*juce::MathConstants<double>::pi));
        const float y=248+amplitude*static_cast<float>(0.65*std::sin(u*27-phase*0.7)+0.35*std::sin(u*49+phase*0.4));
        if(i==0) trace.startNewSubPath(x,y); else trace.lineTo(x,y);
    }
    g.setColour(juce::Colour(0xffb7a5c2).withAlpha(0.08f)); g.strokePath(trace,juce::PathStrokeType(5));
    g.setColour(juce::Colour(0xffc3b2ca).withAlpha(bypass?0.2f:0.36f+energy*0.24f)); g.strokePath(trace,juce::PathStrokeType(1));
    const float a=-0.95f+energy*1.9f;
    g.setColour(juce::Colour(0xff3a241c)); g.drawLine({radial(140,260,8,a),radial(140,260,69,a)},1.4f);
    g.setColour(juce::Colour(0xff4b3e2a)); g.fillEllipse(136,256,8,8);
    g.setColour(juce::Colour(0xff131719)); g.fillEllipse(851,81,11,11);
    g.setColour(bypass?brass.darker(0.6f):amber); g.fillEllipse(854,84,5,5);
    if(!bypass) { g.setColour(amber.withAlpha(0.14f)); g.fillEllipse(850,80,13,13); }
    text(g,bypass?"BYPASS":"ACTIVE",{866,79,47,16},8.5f,ivory.withAlpha(0.7f));
}
