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
    transportBase = static_cast<int>(std::ceil(rate * 0.024));
    ringSize = static_cast<int>(std::ceil(rate * 0.070)) + saturationLatency + 8;
    wet.setSize(2, capacity);
    originalHistory.setSize(2, ringSize);
    tapeHistory.setSize(2, ringSize);
    noiseLpCoefficient = static_cast<float>(1.0 - std::exp(-twoPi * 10000.0 / rate));
    noiseHpCoefficient = static_cast<float>(1.0 - std::exp(-twoPi * 1200.0 / rate));
    for (size_t i = 0; i < smooth.size(); ++i)
        smooth[i].reset(rate, i == 1 || i == 2 ? 0.150 : (i == 7 ? 0.010 : 0.030));
    energyCoefficient = 1.0 - std::exp(-1.0 / (rate * 2 * 0.18));
    gainCoefficient = static_cast<float>(1.0 - std::exp(-1.0 / (rate * 2 * 0.04)));
    bassCoefficient = static_cast<float>(1.0 - std::exp(-twoPi * 140.0 / rate));
    blurCoefficient = static_cast<float>(1.0 - std::exp(-twoPi * 700.0 / rate));
    diffusionLength = { static_cast<int>(rate * 0.0009) + 1, static_cast<int>(rate * 0.0023) + 1 };
    diffusion.setSize(4, diffusionLength[1]);
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
    flutterSeed = 0x134e82abu; dropoutSeed = 0x623b19efu;
    flutterPhase = flutterFrom = 0; flutterTo = randomBipolar(flutterSeed);
    dropoutHazard = 0; dropoutThreshold = 0.6; dropoutPosition = 1; dropoutDepth = 0;
    diffusion.clear(); diffusionPosition = {}; bass = {}; blurLow = {};
    inputEnergy = saturatedEnergy = 0;
    saturationGain = 1.0f / (1.0f + 15.0f * std::pow(juce::jlimit(0.0f, 1.0f, values[0]), 1.2f));
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
    const double wowRate = 1.0 + 2.2 * wow * wow;
    for (size_t i = 0; i < phase.size(); ++i)
    {
        phase[i] += twoPi * frequencies[i] * (i < 3 ? wowRate : 1.0) / rate;
        if (phase[i] >= twoPi) phase[i] -= twoPi;
    }
    // Smoothstep random targets have zero slope at their joins; independent seed from hiss.
    wanderPhase += wowRate * (0.31 + 0.12 * std::sin(phase[2])) / rate;
    if (wanderPhase >= 1.0)
    {
        wanderPhase -= 1.0; wanderFrom = wanderTo; wanderTo = randomBipolar(wanderSeed);
    }
    const double t = wanderPhase * wanderPhase * (3.0 - 2.0 * wanderPhase);
    const double wander = wanderFrom + (wanderTo - wanderFrom) * t;
    const double slow = 0.33 * std::sin(phase[0] + 0.55 * wander)
                      + 0.17 * std::sin(phase[1]) + 0.1 * std::sin(phase[2]) + 0.4 * wander;
    flutterPhase += 8.7 / rate;
    if (flutterPhase >= 1) { flutterPhase -= 1; flutterFrom = flutterTo; flutterTo = randomBipolar(flutterSeed); }
    const double ft = flutterPhase * flutterPhase * (3 - 2 * flutterPhase);
    const double jitter = flutterFrom + (flutterTo - flutterFrom) * ft;
    const double fast = 0.45 * std::sin(phase[3] + 0.35 * wander) + 0.25 * std::sin(phase[4]) + 0.3 * jitter;
    return rate * (0.018 * std::pow(wow, 1.35f) * slow + 0.0007 * std::pow(flutter, 1.2f) * fast);
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

float TapeEngine::wearEnvelope(float age) noexcept
{
    // Independent seeded event clock. Zero Age is exactly clean; no random work per sample.
    if (dropoutPosition >= 1)
    {
        dropoutHazard += (0.02 * age + 2.8 * age * age * age) / rate;
        if (dropoutHazard >= dropoutThreshold)
        {
            dropoutHazard = 0;
            const double u = (randomBipolar(dropoutSeed) + 1.0) * 0.5;
            dropoutThreshold = -std::log(0.05 + 0.95 * u);
            dropoutPosition = 0;
            dropoutStep = 1.0 / (rate * (0.02 + 0.09 * (randomBipolar(dropoutSeed) + 1.0) * 0.5));
            dropoutDepth = 0.4f + 0.45f * (randomBipolar(dropoutSeed) + 1) * 0.5f;
        }
    }
    if (dropoutPosition >= 1) return 0;
    const float shape = static_cast<float>(std::sin(juce::MathConstants<double>::pi * dropoutPosition));
    dropoutPosition = juce::jmin(1.0, dropoutPosition + dropoutStep);
    return age * dropoutDepth * shape * shape;
}

float TapeEngine::blurSample(float sample, int channel) noexcept
{
    // Two short allpass diffusers smear attacks without adding a long reverb tail.
    for (int stage = 0; stage < 2; ++stage)
    {
        const int position = diffusionPosition[static_cast<size_t>(stage)];
        const int lane = channel * 2 + stage;
        const float delayed = diffusion.getSample(lane, position);
        const float output = delayed - 0.45f * sample;
        diffusion.setSample(lane, position, sample + 0.45f * output);
        sample = output;
    }
    return sample;
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
            const float drive = 1.0f + 15.0f * std::pow(warmth, 1.2f);
            double inPower = 0, outPower = 0;
            for (int c = 0; c < channels; ++c)
            {
                auto* samples = up.getChannelPointer(static_cast<size_t>(c));
                const float input = samples[n];
                const float shaped = std::tanh(drive * input);
                inPower += input * input; outPower += shaped * shaped;
                samples[n] = shaped;
            }
            const double divisor = juce::jmax(1, channels);
            inputEnergy += energyCoefficient * (inPower / divisor - inputEnergy);
            saturatedEnergy += energyCoefficient * (outPower / divisor - saturatedEnergy);
            const float compensation = juce::jlimit(1.0f / drive, 1.0f,
                static_cast<float>(std::sqrt((inputEnergy + 1e-12) / (saturatedEnergy + 1e-12))));
            saturationGain += gainCoefficient * (compensation - saturationGain);
            // Bound drive-ramp overshoot using a feed-forward estimate; slow RMS matching
            // alone can lag behind a fast increase in drive. This adds no audio limiter.
            const float reference = static_cast<float>(std::sqrt(2.0 * inputEnergy + 0.0004));
            const float ceiling = juce::jmin(1.0f, 1.414214f * reference / std::tanh(drive * reference));
            for (int c = 0; c < channels; ++c)
                up.getChannelPointer(static_cast<size_t>(c))[n] *= juce::jmin(saturationGain, ceiling);
        }
        oversampling.processSamplesDown(block);
        for (int n = 0; n < count; ++n)
        {
            const auto i = static_cast<size_t>(n);
            const double delay = transportBase + modulation(ramp[1][i], ramp[2][i]);
            const float age = ramp[3][i];
            const float dropout = wearEnvelope(age);
            const double cutoff = 18000.0 * std::pow(850.0 / 18000.0, std::pow(age, 0.8f)) * (1.0 - 0.65 * dropout);
            const float coefficient = static_cast<float>(1.0 - std::exp(-twoPi * juce::jmin(cutoff, rate * 0.45) / rate));
            const float noise = randomBipolar(noiseSeed);
            noiseLow += noiseLpCoefficient * (noise - noiseLow);
            noiseHigh += noiseHpCoefficient * (noiseLow - noiseHigh);
            const float hiss = (noiseLow - noiseHigh) * 0.018f * std::pow(ramp[4][i], 1.35f);
            const float gain = juce::Decibels::decibelsToGain(ramp[6][i]);
            for (int c = 0; c < channels; ++c)
            {
                const float input = clean(buffer.getSample(c, offset + n));
                originalHistory.setSample(c, writePosition, input);
                const float aligned = read(originalHistory, c, saturationLatency);
                const float saturated = aligned + std::sqrt(ramp[0][i]) * (wet.getSample(c, n) - aligned);
                tapeHistory.setSample(c, writePosition, saturated);
                const float transported = read(tapeHistory, c, delay);
                tone1[static_cast<size_t>(c)] += coefficient * (transported - tone1[static_cast<size_t>(c)]);
                tone2[static_cast<size_t>(c)] += coefficient * (tone1[static_cast<size_t>(c)] - tone2[static_cast<size_t>(c)]);
                const auto channel = static_cast<size_t>(c);
                bass[channel] += bassCoefficient * (transported - bass[channel]);
                const float worn = transported + std::sqrt(age) * (tone2[channel] - transported)
                                 + 0.18f * age * bass[channel];
                blurLow[channel] += blurCoefficient * (worn - blurLow[channel]);
                const float high = worn - blurLow[channel];
                const float diffused = blurSample(high, c);
                const float blurred = worn + 0.55f * age * (diffused - high);
                const float instability = 1.0f - age * 0.05f * static_cast<float>(0.5 + 0.5 * std::sin(phase[1]));
                // Bounded amplitude drift only: no crossfeed or inter-channel phase offset.
                const float balance = channels == 2 ? 1.0f + (c == 0 ? 1.0f : -1.0f)
                    * 0.14f * ramp[1][i] * static_cast<float>(std::sin(phase[2] + 0.4 * std::sin(phase[0]))) : 1.0f;
                const float coloured = blurred * (1.0f - dropout) * instability * balance + hiss;
                const float dry = read(originalHistory, c, latency());
                const float mixed = (dry + ramp[5][i] * (coloured - dry)) * gain;
                const float output = ramp[7][i] == 1.0f ? dry : mixed + ramp[7][i] * (dry - mixed);
                buffer.setSample(c, offset + n, clean(output));
            }
            for (size_t stage = 0; stage < 2; ++stage)
                if (++diffusionPosition[stage] >= diffusionLength[stage]) diffusionPosition[stage] = 0;
            if (++writePosition == ringSize) writePosition = 0;
        }
    }
    for (int c = channels; c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, buffer.getNumSamples());
}
