#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cstdint>

// Parameter order matches the established APVTS layout; percentages are normalised here.
// Construction/prepare allocate; processing does not. Histories are independent of host block size.
class TapeEngine
{
public:
    using Controls = std::array<float, 8>;
    void prepare(double sampleRate, const Controls&);
    void reset(const Controls&);
    void setTargets(const Controls&, bool hostBypass = false);
    void process(juce::AudioBuffer<float>&, int channels) noexcept;
    int latency() const noexcept { return transportBase + saturationLatency; }
    double tail() const noexcept { return 0.05; }
private:
    static constexpr int capacity = 1024;
    static float randomBipolar(uint32_t&) noexcept;
    float read(const juce::AudioBuffer<float>&, int channel, double delay) const noexcept;
    double modulation(float wow, float flutter) noexcept;
    double rate = 48000;
    int transportBase = 0, saturationLatency = 0, ringSize = 0, writePosition = 0;
    bool ready = false;
    juce::dsp::Oversampling<float> oversampling { 2, 1,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true };
    juce::AudioBuffer<float> wet, originalHistory, tapeHistory;
    std::array<juce::SmoothedValue<float>, 8> smooth;
    std::array<std::array<float, capacity>, 8> ramp {};
    std::array<double, 5> phase {};
    std::array<float, 2> tone1 {}, tone2 {};
    float noiseLow = 0, noiseHigh = 0, noiseLpCoefficient = 0, noiseHpCoefficient = 0;
    uint32_t noiseSeed = 0, wanderSeed = 0;
    double wanderPhase = 0, wanderFrom = 0, wanderTo = 0;
};
