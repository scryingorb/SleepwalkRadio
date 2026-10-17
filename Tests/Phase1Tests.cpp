#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <iostream>
#include <stdexcept>

static int checks = 0;
static void require(bool condition, const char* message)
{
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
static float value(SleepwalkRadioProcessor& p, const char* id) { return p.parameters.getRawParameterValue(id)->load(); }
static void restore(SleepwalkRadioProcessor& p, const juce::ValueTree& tree)
{
    juce::MemoryBlock data;
    juce::AudioProcessor::copyXmlToBinary(*tree.createXml(), data);
    p.setStateInformation(data.getData(), static_cast<int>(data.getSize()));
}
int runTapeEngineTests();
int runEditorTests();
int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 1 && juce::String(argv[1]) == "--dsp") return runTapeEngineTests();
    if (argc > 1 && juce::String(argv[1]) == "--editor") return runEditorTests();
    try
    {
        SleepwalkRadioProcessor p;
        require(p.getParameters().size() == 8, "parameter count");
        require(p.getBypassParameter() == p.parameters.getParameter("bypass"), "host bypass contract");
        require(p.getTailLengthSeconds() == 0.1, "bounded tail declaration");
        for (const auto& spec : parameterSpecs)
        {
            require(value(p, spec.id) == spec.initial, "default");
            auto* parameter = p.parameters.getParameter(spec.id);
            require(parameter->isAutomatable(), "automatable");
            parameter->setValueNotifyingHost(0.75f);
        }
        juce::MemoryBlock saved;
        p.getStateInformation(saved);
        SleepwalkRadioProcessor restored;
        restored.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
        for (const auto& spec : parameterSpecs) require(value(p, spec.id) == value(restored, spec.id), "round trip");
        auto valid = p.parameters.copyState();
        valid.setProperty("schemaVersion", 1, nullptr);
        auto unchanged = [&] {
            for (const auto& spec : parameterSpecs) require(value(p, spec.id) == value(restored, spec.id), "invalid state changed parameters");
        };
        const char garbage[] = "not a state";
        restored.setStateInformation(garbage, sizeof(garbage)); unchanged();
        restored.setStateInformation(nullptr, 0); unchanged();
        restored.setStateInformation(saved.getData(), 7); unchanged();
        juce::ValueTree wrongRoot("OtherPlugin"); wrongRoot.setProperty("schemaVersion", 1, nullptr);
        restore(restored, wrongRoot); unchanged();
        for (const auto* version : { "2", "0", "1junk", "" })
        {
            auto bad = valid.createCopy(); bad.setProperty("schemaVersion", version, nullptr);
            restore(restored, bad); unchanged();
        }
        for (const auto* invalid : { "NaN", "inf", "abc", "101", "-1", "12junk" })
        {
            auto bad = valid.createCopy();
            bad.getChild(0).setProperty("value", 20, nullptr);
            bad.getChild(1).setProperty("value", invalid, nullptr);
            restore(restored, bad); unchanged();
        }
        auto duplicate = valid.createCopy(); duplicate.appendChild(duplicate.getChild(0).createCopy(), nullptr);
        restore(restored, duplicate); unchanged();
        auto unknown = valid.createCopy(); unknown.getChild(0).setProperty("id", "unknown", nullptr);
        restore(restored, unknown); unchanged();
        auto missingValue = valid.createCopy(); missingValue.getChild(0).removeProperty("value", nullptr);
        restore(restored, missingValue); unchanged();
        auto fractionalBypass = valid.createCopy(); fractionalBypass.getChildWithProperty("id", "bypass").setProperty("value", 0.5, nullptr);
        restore(restored, fractionalBypass); unchanged();
        auto missing = valid.createCopy(); missing.removeChild(missing.getChildWithProperty("id", "mix"), nullptr);
        restore(restored, missing); require(value(restored, "mix") == 100, "missing parameter defaults");

        for (int channels : { 1, 2 })
        {
            auto layout = p.getBusesLayout();
            layout.inputBuses.set(0, juce::AudioChannelSet::canonicalChannelSet(channels));
            layout.outputBuses.set(0, juce::AudioChannelSet::canonicalChannelSet(channels));
            require(p.setBusesLayout(layout), "supported layout");
            for (double rate : { 44100., 48000., 88200., 96000., 192000. })
            {
                for (int samples : { 0, 1, 32, 64, 128, 256, 512, 1024, 137 })
                {
                    for (float setting : { 0.0f, 0.37f, 1.0f })
                    {
                        // Phase 2: neutrality still holds at every Mix setting, after reported latency.
                        // Non-neutral parameter effects are characterised by the Phase2DSP target.
                        for (const auto& spec : parameterSpecs)
                        {
                            auto* parameter = p.parameters.getParameter(spec.id);
                            parameter->setValueNotifyingHost(parameter->convertTo0to1(spec.initial));
                        }
                        p.parameters.getParameter("mix")->setValueNotifyingHost(setting);
                        p.prepareToPlay(rate, 1024);
                        require(p.getLatencySamples() > 0, "reported processing latency");
                        juce::AudioBuffer<float> audio(channels + 1, samples), expected(channels, samples);
                        for (int c = 0; c < channels; ++c)
                            for (int s = 0; s < samples; ++s)
                            {
                                const float sample = static_cast<float>(std::sin(s * (c + 1) * 0.13) * (c == 0 ? 0.7 : -0.3));
                                audio.setSample(c, s, sample); expected.setSample(c, s, sample);
                            }
                        for (int s = 0; s < samples; ++s) audio.setSample(channels, s, 0.8f);
                        juce::MidiBuffer midi;
                        p.processBlock(audio, midi);
                        for (int c = 0; c < channels; ++c)
                            for (int s = 0; s < samples; ++s)
                                require(audio.getSample(c, s) == (s >= p.getLatencySamples()
                                    ? expected.getSample(c, s - p.getLatencySamples()) : 0), "bit exact aligned pass through");
                        require(audio.getMagnitude(channels, 0, samples) == 0, "extra output clear");
                        audio.clear();
                        p.reset();
                        p.processBlockBypassed(audio, midi);
                        require(audio.getMagnitude(0, samples) == 0, "silence stays silent");
                    }
                    p.releaseResources();
                }
            }
        }
        auto layout = p.getBusesLayout();
        layout.outputBuses.set(0, juce::AudioChannelSet::mono());
        require(!p.isBusesLayoutSupported(layout), "reject mismatched buses");
        layout.inputBuses.set(0, juce::AudioChannelSet::disabled());
        require(!p.isBusesLayoutSupported(layout), "reject disabled input");
        layout.inputBuses.set(0, juce::AudioChannelSet::create5point1());
        layout.outputBuses.set(0, juce::AudioChannelSet::create5point1());
        require(!p.isBusesLayoutSupported(layout), "reject surround");
        for (int i = 0; i < 10; ++i)
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
            require(editor->getWidth() == 960 && editor->getHeight() == 560, "editor lifecycle");
            auto* slider = &dynamic_cast<SleepwalkRadioEditor&>(*editor).control("warmth");
            require(slider != nullptr, "editor slider");
            require(slider->keyPressed(juce::KeyPress(juce::KeyPress::leftKey)), "keyboard adjustment");
            p.parameters.getParameter("warmth")->setValueNotifyingHost(0.42f);
            require(std::abs(slider->getValue() - 42.0) < 0.01, "host to editor attachment");
            slider->setValue(28.0, juce::sendNotificationSync);
            require(std::abs(value(p, "warmth") - 28.0f) < 0.01f, "editor to parameter attachment");
            p.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            require(std::abs(slider->getValue() - value(p, "warmth")) < 0.01, "restore with editor open");
            if (i == 0)
            {
                const auto screenshot = editor->createComponentSnapshot(editor->getLocalBounds());
                juce::FileOutputStream stream(juce::File::getCurrentWorkingDirectory().getChildFile("editor-preview.png"));
                stream.setPosition(0);
                stream.truncate();
                require(stream.openedOk() && juce::PNGImageFormat().writeImageToStream(screenshot, stream), "editor snapshot");
            }
        }
        std::cout << "PASS: " << checks << " checks; Phase 1 parameters, state, audio matrix, editor lifecycle\n";
        return 0;
    }
    catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
