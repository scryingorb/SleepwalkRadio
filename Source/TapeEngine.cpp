#include "TapeEngine.h"
#include <cmath>

namespace
{
constexpr double twoPi = juce::MathConstants<double>::twoPi;
float clean(float x) noexcept
{
    // Preserve normal audio/headroom exactly; stop corrupt host samples poisoning histories.
    return std::isfinite(x) ? juce::jlimit(-16.0f, 16.0f, x) : 0.0f;
}
}

void TapeEngine::prepare(double sampleRate, const Controls& values)
{
    rate = std::isfinite(sampleRate) && sampleRate >= 8000 ? sampleRate : 48000;
    oversampling.initProcessing(capacity);
    saturationLatency = juce::roundToInt(oversampling.getLatencyInSamples());
    transportBase = static_cast<int>(std::ceil(rate * 0.006));
    ringSize = static_cast<int>(std::ceil(rate * 0.020)) + saturationLatency + 8;
    wet.setSize(2, capacity);
    originalHistory.setSize(2, ringSize);
    tapeHistory.setSize(2, ringSize);
    noiseLpCoefficient = static_cast<float>(1.0 - std::exp(-twoPi * 10000.0 / rate));
    noiseHpCoefficient = static_cast<float>(1.0 - std::exp(-twoPi * 1200.0 / rate));
    for (size_t i = 0; i < smooth.size(); ++i)
        smooth[i].reset(rate, i == 1 || i == 2 ? 0.150 : (i == 7 ? 0.010 : 0.030));
    ready = true;
    reset(values);
}

void TapeEngine::reset(const Controls& values)
{
    if (!ready) return;
    originalHistory.clear(); tapeHistory.clear(); wet.clear(); oversampling.reset();
    writePosition = 0; phase = {}; tone1 = {}; tone2 = {};
    noiseLow = noiseHigh = 0;
    noiseSeed = 0x81d9a23bu; wanderSeed = 0x5e71c049u;
    wanderPhase = 0; wanderFrom = 0; wanderTo = randomBipolar(wanderSeed);
    setTargets(values);
    for (auto& s : smooth) s.setCurrentAndTargetValue(s.getTargetValue());
}

void TapeEngine::setTargets(const Controls& values, bool hostBypass)
{
    for (size_t i = 0; i < values.size(); ++i)
    {
        const float v = i == 7 && hostBypass ? 1.0f : (std::isfinite(values[i]) ? values[i] : 0.0f);
        smooth[i].setTargetValue(i == 6 ? juce::jlimit(-12.0f, 12.0f, v)
                                      : juce::jlimit(0.0f, 1.0f, v));
    }
}

float TapeEngine::randomBipolar(uint32_t& seed) noexcept
{
    seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
    return static_cast<float>(seed >> 8) * (2.0f / 16777216.0f) - 1.0f;
}

double TapeEngine::modulation(float wow, float flutter) noexcept
{
    constexpr std::array<double, 5> frequencies { 0.23, 0.371, 0.073, 6.31, 9.73 };
    for (size_t i = 0; i < phase.size(); ++i)
    {
        phase[i] += twoPi * frequencies[i] / rate;
        if (phase[i] >= twoPi) phase[i] -= twoPi;
    }
    // Smoothstep random targets have zero slope at their joins; independent seed from hiss.
    wanderPhase += 0.41 / rate;
    if (wanderPhase >= 1.0)
    {
        wanderPhase -= 1.0; wanderFrom = wanderTo; wanderTo = randomBipolar(wanderSeed);
    }
    const double t = wanderPhase * wanderPhase * (3.0 - 2.0 * wanderPhase);
    const double wander = wanderFrom + (wanderTo - wanderFrom) * t;
    const double slow = 0.45 * std::sin(phase[0]) + 0.25 * std::sin(phase[1])
                      + 0.15 * std::sin(phase[2]) + 0.15 * wander;
    const double fast = 0.65 * std::sin(phase[3]) + 0.35 * std::sin(phase[4]);
    return rate * (0.004 * wow * wow * slow + 0.00015 * flutter * flutter * fast);
}

float TapeEngine::read(const juce::AudioBuffer<float>& history, int channel, double delay) const noexcept
{
    double position = writePosition - delay;
    if (position < 0) position += ringSize;
    const int index = static_cast<int>(position);
    const float fraction = static_cast<float>(position - index);
    const auto at = [&](int offset) {
        int n = index + offset;
        if (n < 0) n += ringSize;
        if (n >= ringSize) n -= ringSize;
        return history.getSample(channel, n);
    };
    const float b = at(0);
    if (fraction == 0) return b; // exact integer-delay neutrality
    const float a = at(-1), c = at(1), d = at(2);
    // Four-point cubic Hermite interpolation; reads always stay behind the writer.
    return b + 0.5f * fraction * (c - a + fraction *
        (2*a - 5*b + 4*c - d + fraction * (3*(b-c) + d-a)));
}

void TapeEngine::process(juce::AudioBuffer<float>& buffer, int channels) noexcept
{
    juce::ScopedNoDenormals noDenormals;
    channels = juce::jlimit(0, 2, juce::jmin(channels, buffer.getNumChannels()));
    if (!ready) { buffer.clear(); return; }
    for (int offset = 0; offset < buffer.getNumSamples(); offset += capacity)
    {
        const int count = juce::jmin(capacity, buffer.getNumSamples() - offset);
        for (int n = 0; n < count; ++n)
        {
            for (size_t p = 0; p < smooth.size(); ++p) ramp[p][static_cast<size_t>(n)] = smooth[p].getNextValue();
            for (int c = 0; c < 2; ++c)
                wet.setSample(c, n, c < channels ? clean(buffer.getSample(c, offset + n)) : 0);
        }
        auto block = juce::dsp::AudioBlock<float>(wet).getSubBlock(0, static_cast<size_t>(count));
        auto up = oversampling.processSamplesUp(block);
        for (size_t n = 0; n < up.getNumSamples(); ++n)
        {
            const float warmth = ramp[0][n / 2];
            const float drive = 1.0f + 3.0f * warmth * warmth;
            const float compensation = (1.0f + 0.5f * warmth) / drive;
            for (int c = 0; c < channels; ++c)
            {
                auto* samples = up.getChannelPointer(static_cast<size_t>(c));
                samples[n] = std::tanh(drive * samples[n]) * compensation;
            }
        }
        oversampling.processSamplesDown(block);
        for (int n = 0; n < count; ++n)
        {
            const auto i = static_cast<size_t>(n);
            const double delay = transportBase + modulation(ramp[1][i], ramp[2][i]);
            const float age = ramp[3][i];
            const double cutoff = 18000.0 * std::pow(2500.0 / 18000.0, age);
            const float coefficient = static_cast<float>(1.0 - std::exp(-twoPi * juce::jmin(cutoff, rate * 0.45) / rate));
            const float noise = randomBipolar(noiseSeed);
            noiseLow += noiseLpCoefficient * (noise - noiseLow);
            noiseHigh += noiseHpCoefficient * (noiseLow - noiseHigh);
            const float hiss = (noiseLow - noiseHigh) * 0.004f * ramp[4][i] * ramp[4][i];
            const float gain = juce::Decibels::decibelsToGain(ramp[6][i]);
            for (int c = 0; c < channels; ++c)
            {
                const float input = clean(buffer.getSample(c, offset + n));
                originalHistory.setSample(c, writePosition, input);
                const float aligned = read(originalHistory, c, saturationLatency);
                const float saturated = aligned + ramp[0][i] * (wet.getSample(c, n) - aligned);
                tapeHistory.setSample(c, writePosition, saturated);
                const float transported = read(tapeHistory, c, delay);
                tone1[static_cast<size_t>(c)] += coefficient * (transported - tone1[static_cast<size_t>(c)]);
                tone2[static_cast<size_t>(c)] += coefficient * (tone1[static_cast<size_t>(c)] - tone2[static_cast<size_t>(c)]);
                const float coloured = transported + age * (tone2[static_cast<size_t>(c)] - transported) + hiss;
                const float dry = read(originalHistory, c, latency());
                const float mixed = (dry + ramp[5][i] * (coloured - dry)) * gain;
                const float output = ramp[7][i] == 1.0f ? dry : mixed + ramp[7][i] * (dry - mixed);
                buffer.setSample(c, offset + n, clean(output));
            }
            if (++writePosition == ringSize) writePosition = 0;
        }
    }
    for (int c = channels; c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, buffer.getNumSamples());
}
