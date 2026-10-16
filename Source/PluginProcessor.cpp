#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <cstdlib>
#include <set>

juce::AudioProcessorValueTreeState::ParameterLayout SleepwalkRadioProcessor::makeParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& spec : parameterSpecs)
    {
        const juce::ParameterID id { spec.id, 1 };
        if (juce::String(spec.id) == "bypass")
            layout.add(std::make_unique<juce::AudioParameterBool>(id, spec.name, false));
        else
            layout.add(std::make_unique<juce::AudioParameterFloat>(id, spec.name,
                juce::NormalisableRange<float>(spec.min, spec.max, 0.1f), spec.initial,
                juce::AudioParameterFloatAttributes().withLabel(spec.unit)
                    .withStringFromValueFunction([](float value, int) { return juce::String(value, 1); })));
    }
    return layout;
}

SleepwalkRadioProcessor::SleepwalkRadioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "SleepwalkRadioState", makeParameters())
{
    for (size_t i = 0; i < parameterSpecs.size(); ++i)
        rawParameters[i] = parameters.getRawParameterValue(parameterSpecs[i].id);
}

TapeEngine::Controls SleepwalkRadioProcessor::readControls() const noexcept
{
    TapeEngine::Controls values {};
    for (size_t i = 0; i < values.size(); ++i)
        values[i] = rawParameters[i]->load(std::memory_order_relaxed) * (i < 6 ? 0.01f : 1.0f);
    return values;
}

void SleepwalkRadioProcessor::prepareToPlay(double sampleRate, int)
{
    engine.prepare(sampleRate, readControls());
    setLatencySamples(engine.latency());
}

void SleepwalkRadioProcessor::reset() { engine.reset(readControls()); }

bool SleepwalkRadioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto input = layout.getMainInputChannelSet();
    return layout.inputBuses.size() == 1 && layout.outputBuses.size() == 1
        && (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo())
        && input == layout.getMainOutputChannelSet();
}

void SleepwalkRadioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    engine.setTargets(readControls());
    engine.process(buffer, getTotalNumInputChannels());
    publishVisuals(buffer, false);
    midi.clear();
}

void SleepwalkRadioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    // Keep histories alive and use the same smooth, latency-aligned dry transition.
    engine.setTargets(readControls(), true);
    engine.process(buffer, getTotalNumInputChannels());
    publishVisuals(buffer, true);
    midi.clear();
}

void SleepwalkRadioProcessor::publishVisuals(const juce::AudioBuffer<float>& buffer, bool hostBypassed) noexcept
{
    if (visualObservers.load(std::memory_order_relaxed) == 0 || buffer.getNumSamples() == 0) return;
    // A small approximate meter snapshot only: UI never reads the mutable audio buffer.
    visualPeak.store(juce::jlimit(0.0f, 1.0f, buffer.getMagnitude(0, buffer.getNumSamples())), std::memory_order_relaxed);
    visualHostBypass.store(hostBypassed, std::memory_order_relaxed);
    visualSerial.fetch_add(1, std::memory_order_relaxed);
}

juce::AudioProcessorParameter* SleepwalkRadioProcessor::getBypassParameter() const
{
    return parameters.getParameter("bypass");
}

void SleepwalkRadioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto state = parameters.copyState();
    state.setProperty("schemaVersion", 1, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destination);
}

void SleepwalkRadioProcessor::setStateInformation(const void* data, int size)
{
    if (data == nullptr || size <= 0 || size > 1024 * 1024) return;
    auto xml = getXmlFromBinary(data, size);
    if (!xml || !xml->hasTagName("SleepwalkRadioState")
        || xml->getStringAttribute("schemaVersion") != "1") return;
    auto incoming = juce::ValueTree::fromXml(*xml);
    // Build a complete candidate using defaults; never mutate live state during validation.
    juce::ValueTree candidate("SleepwalkRadioState");
    candidate.setProperty("schemaVersion", 1, nullptr);
    std::set<juce::String> seen;
    for (const auto& child : incoming)
    {
        const auto id = child.getProperty("id").toString();
        const auto spec = std::find_if(parameterSpecs.begin(), parameterSpecs.end(),
            [&](const auto& s) { return id == s.id; });
        if (!child.hasType("PARAM") || spec == parameterSpecs.end() || !seen.insert(id).second
            || !child.hasProperty("value") || child.getNumChildren() != 0) return;
        const auto text = child.getProperty("value").toString();
        const auto* start = text.toRawUTF8();
        char* end = nullptr;
        const double value = std::strtod(start, &end);
        if (end == start || *end != '\0' || !std::isfinite(value)
            || value < spec->min || value > spec->max
            || (id == "bypass" && value != 0 && value != 1)) return;
    }
    for (const auto& spec : parameterSpecs)
    {
        juce::ValueTree child("PARAM");
        child.setProperty("id", spec.id, nullptr);
        const auto saved = incoming.getChildWithProperty("id", spec.id);
        child.setProperty("value", saved.isValid() ? saved.getProperty("value") : juce::var(spec.initial), nullptr);
        candidate.appendChild(child, nullptr);
    }
    parameters.replaceState(candidate);
}

juce::AudioProcessorEditor* SleepwalkRadioProcessor::createEditor() { return new SleepwalkRadioEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SleepwalkRadioProcessor(); }
