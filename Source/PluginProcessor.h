#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "TapeEngine.h"
#include "FactoryPresets.h"

struct ParameterSpec { const char* id; const char* name; float min, max, initial; const char* unit; };
inline constexpr std::array<ParameterSpec, 8> parameterSpecs {{
    { "warmth", "Warmth", 0, 100, 0, "%" },
    { "wobble", "Wobble", 0, 100, 0, "%" },
    { "flutter", "Flutter", 0, 100, 0, "%" },
    { "age", "Age", 0, 100, 0, "%" },
    { "hiss", "Hiss", 0, 100, 0, "%" },
    { "mix", "Mix", 0, 100, 100, "%" },
    { "output", "Output", -12, 12, 0, "dB" },
    { "bypass", "Bypass", 0, 1, 0, "" }
}};

class SleepwalkRadioProcessor final : public juce::AudioProcessor
{
public:
    SleepwalkRadioProcessor();
    const juce::String getName() const override { return "SleepwalkRadio"; }
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return engine.tail(); }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    int getNumPrograms() override { return static_cast<int>(factoryPresets.size()); }
    int getCurrentProgram() override { return selectedPreset.load(std::memory_order_relaxed); }
    void setCurrentProgram(int index) override { applyPreset(index, false); }
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}
    void applyPreset(int index, bool gestures = true);
    bool isPresetModified() const noexcept;
    juce::String presetDisplayName();
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState parameters;
    struct VisualSnapshot { float peak; uint32_t serial; bool hostBypassed; };
    VisualSnapshot readVisualSnapshot() const noexcept
    { return { visualPeak.load(std::memory_order_relaxed), visualSerial.load(std::memory_order_relaxed), visualHostBypass.load(std::memory_order_relaxed) }; }
    void addVisualObserver() noexcept { visualObservers.fetch_add(1, std::memory_order_relaxed); }
    void removeVisualObserver() noexcept { visualObservers.fetch_sub(1, std::memory_order_relaxed); }
    int getVisualObserverCount() const noexcept { return visualObservers.load(std::memory_order_relaxed); }
    // Per-instance UI preference survives editor reopening, but is not an audio parameter/state schema field.
    std::atomic<bool> motionEnabled { true };
private:
    static_assert(std::atomic<float>::is_always_lock_free && std::atomic<uint32_t>::is_always_lock_free
                  && std::atomic<int>::is_always_lock_free && std::atomic<bool>::is_always_lock_free,
                  "Visualization requires lock-free atomics");
    std::atomic<float> visualPeak { 0 };
    std::atomic<uint32_t> visualSerial { 0 };
    std::atomic<bool> visualHostBypass { false };
    std::atomic<int> visualObservers { 0 };
    void publishVisuals(const juce::AudioBuffer<float>&, bool hostBypassed) noexcept;
    TapeEngine engine;
    std::atomic<int> selectedPreset { 0 };
    std::array<std::atomic<float>*, 8> rawParameters {};
    TapeEngine::Controls readControls() const noexcept;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SleepwalkRadioProcessor)
};
