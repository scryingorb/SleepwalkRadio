#include "../Source/PluginEditor.h"
#include <iostream>
#include <stdexcept>
#include <chrono>

void beginAllocationCheck();
size_t endAllocationCheck();

namespace
{
int checks=0;
void check(bool ok,const char* message) { ++checks; if(!ok) throw std::runtime_error(message); }
struct GestureProbe final : juce::AudioProcessorParameter::Listener
{
    int starts=0,ends=0,changes=0;
    void parameterValueChanged(int,float) override { ++changes; }
    void parameterGestureChanged(int,bool starting) override { starting?++starts:++ends; }
};
void snapshot(juce::Component& component,const juce::String& name,float scale)
{
    const auto image=component.createComponentSnapshot(component.getLocalBounds(),true,scale);
    check(image.getWidth()==juce::roundToInt(component.getWidth()*scale),"snapshot pixel width");
    check(image.getHeight()==juce::roundToInt(component.getHeight()*scale),"snapshot pixel height");
    juce::FileOutputStream stream(juce::File::getCurrentWorkingDirectory().getChildFile(name));
    stream.setPosition(0); stream.truncate();
    check(stream.openedOk() && juce::PNGImageFormat().writeImageToStream(image,stream),"write actual editor render");
}
}

int runEditorTests()
{
    try
    {
        SleepwalkRadioProcessor processor, other;
        processor.prepareToPlay(48000,128); other.prepareToPlay(48000,128);
        {
            SleepwalkRadioEditor editor(processor);
            check(processor.getVisualObserverCount()==1,"editor visual observer registered");
            check(editor.isAnimationTimerRunning(),"motion polling active");
            for(const auto& spec:parameterSpecs)
            {
                auto* parameter=processor.parameters.getParameter(spec.id);
                auto& control=editor.control(spec.id);
                check(control.getName()==spec.name && control.getTitle()==spec.name,"accessible parameter names");
                check(control.getWantsKeyboardFocus() && control.getExplicitFocusOrder()>0,"keyboard focus configured");
                check(parameter->isAutomatable(),"parameter still automatable");
                parameter->setValueNotifyingHost(0.72f);
                check(std::abs(control.getValue()-processor.parameters.getRawParameterValue(spec.id)->load())<0.001,"host updates every control");
                const double typed=juce::String(spec.id)=="bypass"?0.0:(spec.min+spec.max)*0.5;
                juce::Label* box=nullptr;
                for(int i=0;i<control.getNumChildComponents();++i)
                    if(auto* label=dynamic_cast<juce::Label*>(control.getChildComponent(i))) box=label;
                check(box!=nullptr,"text entry available");
                GestureProbe probe; parameter->addListener(&probe);
                box->showEditor();
                auto* entry=box->getCurrentTextEditor();
                check(entry!=nullptr,"value text editor opens");
                entry->setText(control.getTextFromValue(typed));
                box->hideEditor(false);
                check(std::abs(control.getValue()-typed)<0.001,"text entry commits to knob");
                check(std::abs(processor.parameters.getRawParameterValue(spec.id)->load()-typed)<0.001,"text entry commits to parameter");
                check(probe.starts==1 && probe.ends==1,"text entry automation gesture");
                check(control.keyPressed(juce::KeyPress(juce::KeyPress::rightKey)),"arrow key handled");
                check(probe.starts==2 && probe.ends==2,"keyboard automation gesture");
                check(control.getDoubleClickReturnValue()==spec.initial,"double click default");
                check(control.getVelocityModeIsSwappable() && control.getVelocitySensitivity()==0.25,"fine adjustment configured");
                if(juce::String(spec.id)=="bypass")
                {
                    check(control.keyPressed(juce::KeyPress(juce::KeyPress::spaceKey)),"bypass space toggle");
                    check(control.getValue()==0 && processor.parameters.getRawParameterValue("bypass")->load()==0,"bypass toggle synchronized");
                }
                parameter->removeListener(&probe);
                check(other.parameters.getRawParameterValue(spec.id)->load()==spec.initial,"instances isolated");
            }
            // Save nondefault host settings, change them, and restore with all controls open.
            for(auto* parameter:processor.getParameters()) parameter->setValueNotifyingHost(0.61f);
            juce::MemoryBlock state; processor.getStateInformation(state);
            for(auto* parameter:processor.getParameters()) parameter->setValueNotifyingHost(0.15f);
            processor.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
            for(const auto& spec:parameterSpecs)
                check(std::abs(editor.control(spec.id).getValue()-processor.parameters.getRawParameterValue(spec.id)->load())<0.001,"state restore updates every control");
            for(auto size:{juce::Point<int>(768,448),juce::Point<int>(960,560),juce::Point<int>(1200,700),juce::Point<int>(1440,840)})
            {
                editor.setSize(size.x,size.y);
                std::array<juce::Rectangle<int>,8> bounds;
                for(size_t i=0;i<parameterSpecs.size();++i)
                {
                    auto& control=editor.control(parameterSpecs[i].id);
                    bounds[i]=editor.getLocalArea(&control,control.getLocalBounds());
                    check(editor.getLocalBounds().contains(bounds[i]),"control inside resized editor");
                    check(bounds[i].getWidth()>=73 && bounds[i].getHeight()>=89,"usable minimum control dimensions");
                    for(size_t j=0;j<i;++j) check(!bounds[i].intersects(bounds[j]),"no overlapping controls");
                }
            }
            check(editor.getConstrainer()->getMinimumWidth()==768 && editor.getConstrainer()->getMaximumWidth()==1440,"resize limits");
            check(std::abs(editor.getConstrainer()->getFixedAspectRatio()-960.0/560.0)<1e-6,"fixed panel aspect ratio");
            editor.setAnimationEnabled(false);
            check(!editor.isAnimationTimerRunning() && processor.getVisualObserverCount()==0,"motion off stops timer and publication");
            check(other.motionEnabled.load(),"motion preference instance isolation");
            for(const auto& spec:parameterSpecs)
            {
                auto* p=processor.parameters.getParameter(spec.id); p->setValueNotifyingHost(p->convertTo0to1(spec.initial));
                check(editor.control(spec.id).getValue()==spec.initial,"controls functional with animation disabled");
            }
            editor.setSize(960,560);
            for(float scale:{1.0f,1.25f,1.5f,2.0f})
                snapshot(editor,"receiver-"+juce::String(juce::roundToInt(scale*100))+".png",scale);
            editor.setSize(768,448); snapshot(editor,"receiver-minimum.png",1);
            editor.setSize(1440,840); snapshot(editor,"receiver-maximum.png",1);

            // Observe real audio without changing a single output sample, including editor closure.
            processor.reset(); other.reset(); editor.setAnimationEnabled(true);
            juce::AudioBuffer<float> a(2,128),b(2,128); juce::MidiBuffer midi;
            for(int block=0;block<50;++block)
            {
                for(int c=0;c<2;++c) for(int s=0;s<128;++s) a.setSample(c,s,0.4f*static_cast<float>(std::sin((block*128+s)*0.1)));
                b.makeCopyOf(a);
                beginAllocationCheck(); processor.processBlock(a,midi); other.processBlock(b,midi);
                check(endAllocationCheck()==0,"audio publication allocates nothing with editor open");
                for(int c=0;c<2;++c) for(int s=0;s<128;++s) check(a.getSample(c,s)==b.getSample(c,s),"editor and metering leave DSP unchanged");
            }
            check(processor.readVisualSnapshot().peak>0.3f && processor.readVisualSnapshot().serial>0,"real audio level published");
            check(other.readVisualSnapshot().serial==0,"no visual work without observer");
            editor.setAnimationEnabled(false);
            const auto serial=processor.readVisualSnapshot().serial;
            processor.processBlock(a,midi);
            check(processor.readVisualSnapshot().serial==serial,"disabled display stops audio publication");
        }
        check(processor.getVisualObserverCount()==0,"closing editor releases observer");
        for(int i=0;i<30;++i)
        {
            SleepwalkRadioEditor editor(processor);
            check(!editor.isAnimationEnabled() && !editor.isAnimationTimerRunning(),"motion preference survives reopening");
            editor.setAnimationEnabled(true);
            check(processor.getVisualObserverCount()==1,"reopen exactly one observer");
            editor.setAnimationEnabled(false);
        }
        check(processor.getVisualObserverCount()==0,"lifecycle no leaked observer");
        // Actual dynamic-overlay renders, independent of a native window or audio buffer access.
        BroadcastDisplay display; display.setSize(960,560);
        display.update(0.4f,0.6f,0.3f,false,true,0.033);
        const auto initial=display.createComponentSnapshot(display.getLocalBounds());
        const auto started=std::chrono::steady_clock::now();
        juce::Image frame(juce::Image::ARGB,960,560,true);
        for(int i=0;i<120;++i)
        {
            display.update(0.4f,0.6f,0.3f,false,true,1.0/30);
            frame.clear(frame.getBounds()); juce::Graphics g(frame); display.paint(g);
        }
        const auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count()/120;
        bool differs=false;
        for(int x=540;x<590;++x) if(initial.getPixelAt(x,200)!=frame.getPixelAt(x,200)) differs=true;
        check(differs,"animated indicator moves");
        std::cout<<"Overlay CPU render: "<<elapsed<<" ms/frame (offscreen, 960x560)\n";
        std::cout<<"PASS: "<<checks<<" editor checks; all controls, gestures, state, layout, renders, lifecycle and audio isolation\n";
        return 0;
    }
    catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
