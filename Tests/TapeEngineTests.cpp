#include "../Source/PluginProcessor.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <limits>

void beginAllocationCheck();
size_t endAllocationCheck();

namespace
{
using Controls = TapeEngine::Controls;
const Controls neutral { 0, 0, 0, 0, 0, 1, 0, 0 };
int checks = 0;
void check(bool ok, const char* message)
{
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
using Signal = std::vector<float>;
Signal sine(double rate, double seconds, double hz, float level = 0.5f)
{
    Signal result(static_cast<size_t>(rate * seconds));
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = level * static_cast<float>(std::sin(juce::MathConstants<double>::twoPi * hz * i / rate));
    return result;
}
Signal render(const Signal& input, double rate, Controls controls, int blockSize = 137)
{
    TapeEngine engine;
    engine.prepare(rate, controls);
    juce::AudioBuffer<float> buffer(1, blockSize);
    Signal result(input.size());
    for (size_t offset = 0; offset < input.size(); offset += static_cast<size_t>(blockSize))
    {
        const int count = static_cast<int>(std::min(static_cast<size_t>(blockSize), input.size() - offset));
        buffer.setSize(1, count, false, false, true); // test harness only; engine never resizes in process
        for (int i = 0; i < count; ++i) buffer.setSample(0, i, input[offset + static_cast<size_t>(i)]);
        engine.process(buffer, 1);
        for (int i = 0; i < count; ++i) result[offset + static_cast<size_t>(i)] = buffer.getSample(0, i);
    }
    return result;
}
double amplitude(const Signal& signal, double rate, double hz, size_t start, size_t count)
{
    double re = 0, im = 0;
    for (size_t n = 0; n < count; ++n)
    {
        const double phase = juce::MathConstants<double>::twoPi * hz * n / rate;
        re += signal[start+n] * std::cos(phase); im += signal[start+n] * std::sin(phase);
    }
    return 2.0 * std::hypot(re, im) / count;
}
double rms(const Signal& signal, size_t start = 0)
{
    double sum = 0;
    for (size_t i = start; i < signal.size(); ++i) sum += signal[i] * signal[i];
    return std::sqrt(sum / (signal.size() - start));
}
std::pair<double, double> pitchStats(const Signal& signal, double rate)
{
    double previous = -1, lastFrequency = 1000, sum = 0, differences = 0;
    int count = 0;
    for (size_t i = static_cast<size_t>(rate); i < signal.size(); ++i)
        if (signal[i-1] <= 0 && signal[i] > 0)
        {
            const double crossing = i - 1 + (-signal[i-1]) / static_cast<double>(signal[i] - signal[i-1]);
            if (previous >= 0)
            {
                const double f = rate / (crossing - previous);
                sum += (f - 1000) * (f - 1000);
                if (count > 0) differences += (f - lastFrequency) * (f - lastFrequency);
                lastFrequency = f; ++count;
            }
            previous = crossing;
        }
    return { std::sqrt(sum / count), std::sqrt(differences / (count - 1)) };
}

void matrix()
{
    for (double rate : { 44100., 48000., 88200., 96000., 192000. })
    {
        TapeEngine latencyProbe; latencyProbe.prepare(rate, neutral);
        std::cout << "latency " << rate << " Hz: " << latencyProbe.latency() << " samples ("
                  << 1000.0 * latencyProbe.latency() / rate << " ms)\n";
        for (int channels : { 1, 2 })
        {
            for (int size : { 0, 1, 32, 64, 128, 256, 512, 1024, 2053 })
            {
                TapeEngine engine;
                Controls maximum { 1, 1, 1, 1, 1, 1, 12, 0 };
                engine.prepare(rate, maximum);
                juce::AudioBuffer<float> buffer(channels + 1, size);
                for (int cycle = 0; cycle < 6; ++cycle)
                {
                    auto target = maximum;
                    if (cycle % 2) target = neutral;
                    engine.setTargets(target);
                    for (int c = 0; c < channels; ++c)
                        for (int s = 0; s < size; ++s)
                            buffer.setSample(c, s, 0.9f * static_cast<float>(std::sin((s + cycle * size) * (c+1) * 0.07)));
                    for (int s = 0; s < size; ++s) buffer.setSample(channels, s, 1);
                    beginAllocationCheck();
                    engine.process(buffer, channels);
                    check(endAllocationCheck() == 0, "engine callback allocates no C++ memory");
                    for (int c = 0; c < channels; ++c)
                        for (int s = 0; s < size; ++s)
                            check(std::isfinite(buffer.getSample(c, s)) && std::abs(buffer.getSample(c, s)) < 5,
                                  "matrix finite bounded output");
                    check(buffer.getMagnitude(channels, 0, size) == 0, "extra output cleared");
                }
                engine.reset(neutral); buffer.clear(); engine.process(buffer, channels);
                check(buffer.getMagnitude(0, size) == 0, "reset neutral silence");
            }
        }
        const auto source = sine(rate, 0.12, 997);
        for (float mix : { 0.0f, 0.5f, 1.0f })
        {
            auto controls = neutral; controls[5] = mix;
            const auto result = render(source, rate, controls);
            const auto latency = static_cast<size_t>(latencyProbe.latency());
            for (size_t i = 0; i < result.size(); ++i)
                check(result[i] == (i < latency ? 0.0f : source[i-latency]), "neutral exact after latency at all mixes");
        }
    }
}

void characterisation()
{
    constexpr double rate = 48000;
    auto tone = sine(rate, 12, 1000);
    auto wow = neutral; wow[1] = 1;
    auto flutter = neutral; flutter[2] = 1;
    const auto wowStats = pitchStats(render(tone, rate, wow), rate);
    const auto flutterStats = pitchStats(render(tone, rate, flutter), rate);
    std::cout << "wow frequency RMS deviation " << wowStats.first << " Hz, successive-cycle change " << wowStats.second
              << "; flutter " << flutterStats.first << " Hz, change " << flutterStats.second << '\n';
    check(wowStats.first > 0.5 && wowStats.first < 15, "wow measurable bounded pitch");
    check(flutterStats.first > 1 && flutterStats.first < 15, "flutter measurable bounded pitch");
    check(flutterStats.second > wowStats.second * 5, "flutter faster than wow");

    auto warmth = neutral; warmth[0] = 1;
    const auto saturated = render(sine(rate, 2, 1000, 0.8f), rate, warmth);
    const double fundamental = amplitude(saturated, rate, 1000, 48000, 48000);
    const double harmonic = amplitude(saturated, rate, 3000, 48000, 48000);
    std::cout << "saturation third/fundamental " << 20 * std::log10(harmonic / fundamental) << " dB\n";
    check(harmonic / fundamental > 0.03 && harmonic / fundamental < 0.4, "controlled harmonic coloration");
    for (float level : { 0.05f, 0.25f, 0.8f })
    {
        const auto input = sine(rate, 2, 1000, level);
        const double gain = 20 * std::log10(rms(render(input, rate, warmth), 48000) / rms(input, 48000));
        std::cout << "warmth max gain at " << level << " peak: " << gain << " dB\n";
        check(gain < 4 && gain > -9, "compensated saturation gain range");
    }
    auto age = neutral; age[3] = 1;
    const double highGain = rms(render(sine(rate, 2, 10000), rate, age), 48000) / (0.5 / std::sqrt(2.0));
    const double lowGain = rms(render(sine(rate, 2, 100), rate, age), 48000) / (0.5 / std::sqrt(2.0));
    std::cout << "age max: 10kHz " << 20*std::log10(highGain) << " dB; 100Hz " << 20*std::log10(lowGain) << " dB\n";
    check(highGain < 0.15 && lowGain > 0.98, "age darkens highs preserves bass");

    for (double fs : { 44100., 48000. })
    {
        const auto input = sine(fs, 2, 10000, 0.8f);
        const auto output = render(input, fs, warmth);
        Signal naive(input.size());
        for (size_t i = 0; i < naive.size(); ++i) naive[i] = std::tanh(4 * input[i]) * 0.375f;
        const double aliasHz = fs - 30000;
        const double alias = amplitude(output, fs, aliasHz, static_cast<size_t>(fs), static_cast<size_t>(fs));
        const double reference = amplitude(naive, fs, aliasHz, static_cast<size_t>(fs), static_cast<size_t>(fs));
        const double improvement = 20 * std::log10(reference / std::max(alias, 1.0e-12));
        std::cout << "2x FIR saturation alias reduction " << fs << " Hz at " << aliasHz << " Hz: " << improvement << " dB\n";
        check(improvement > 20, "oversampling reduces folded third harmonic");
    }
}

void mixNoiseStereoAndContinuity()
{
    constexpr double rate = 48000;
    const auto input = sine(rate, 2, 317);
    Controls full { 0.7f, 0.8f, 0.6f, 0.4f, 0.7f, 1, 0, 0 };
    auto half = full; half[5] = 0.5f;
    auto dry = full; dry[5] = 0;
    const auto wet = render(input, rate, full), halfway = render(input, rate, half), dryResult = render(input, rate, dry);
    const auto reference = render(input, rate, neutral);
    for (size_t i = 0; i < input.size(); ++i)
    {
        check(dryResult[i] == reference[i], "mix zero excludes wet including hiss");
        check(std::abs(halfway[i] - 0.5f*(wet[i]+dryResult[i])) < 2e-7f, "linear aligned mix");
    }
    auto trim = dry; trim[6] = 6;
    const auto trimmed = render(input, rate, trim);
    check(std::abs(rms(trimmed) / rms(dryResult) - std::pow(10.0, 6.0/20)) < 1e-5, "output applies at dry mix");
    auto bypass = full; bypass[7] = 1; bypass[6] = 12;
    check(render(input, rate, bypass) == reference, "bypass dry ignores trim and effects");
    check(render(input, rate, full, 32) == render(input, rate, full, 2053), "block partition invariant including noise");

    Signal silence(96000, 0);
    auto noHiss = full; noHiss[4] = 0;
    check(rms(render(silence, rate, noHiss)) == 0, "hiss disabled silence exact");
    auto hiss = neutral; hiss[4] = 1;
    const auto noise = render(silence, rate, hiss);
    std::cout << "hiss max RMS " << 20*std::log10(rms(noise)) << " dBFS\n";
    check(rms(noise) > 0.0001 && rms(noise) < 0.003, "quiet bounded hiss");
    check(noise == render(silence, rate, hiss), "deterministic seeded noise");
    auto darkNoise = hiss; darkNoise[3] = 1;
    check(noise == render(silence, rate, darkNoise), "hiss independent of age");

    TapeEngine engine; engine.prepare(rate, full);
    juce::AudioBuffer<float> stereo(2, 256);
    for (int b = 0; b < 100; ++b)
    {
        for (int s = 0; s < 256; ++s)
            for (int c = 0; c < 2; ++c) stereo.setSample(c, s, static_cast<float>(std::sin((b*256+s)*0.07)) * 0.4f);
        engine.process(stereo, 2);
        for (int s = 0; s < 256; ++s) check(stereo.getSample(0,s) == stereo.getSample(1,s), "linked stereo image");
    }
    engine.reset(noHiss);
    for (int b = 0; b < 20; ++b)
    {
        stereo.clear();
        for (int s = 0; s < 256; ++s) stereo.setSample(0, s, 0.3f);
        engine.process(stereo, 2);
        check(stereo.getMagnitude(1, 0, 256) == 0, "no channel crosstalk");
    }
    // DC isolates parameter-step discontinuities from ordinary audio slopes.
    engine.prepare(rate, neutral);
    juce::AudioBuffer<float> dc(2, 64);
    float previous = 0.2f; double maxStep = 0;
    for (int b = 0; b < 1000; ++b)
    {
        auto target = b % 2 ? full : neutral;
        target[4] = 0; target[6] = b % 2 ? -12.0f : 12.0f;
        target[7] = b % 3 == 0 ? 1.0f : 0.0f;
        engine.setTargets(target);
        for (int s = 0; s < 64; ++s) { dc.setSample(0,s,0.2f); dc.setSample(1,s,0.2f); }
        engine.process(dc, 2);
        for (int s = 0; s < 64; ++s)
        {
            if (b > 30) maxStep = std::max(maxStep, static_cast<double>(std::abs(dc.getSample(0,s) - previous)));
            previous = dc.getSample(0,s);
        }
    }
    std::cout << "rapid automation DC max adjacent step " << maxStep << '\n';
    check(maxStep < 0.01, "smoothed automation has no abrupt DC jump");
}

void longRunAndState()
{
    constexpr double rate = 192000;
    Controls full { 1, 1, 1, 1, 1, 1, 12, 0 };
    TapeEngine engine; engine.prepare(rate, full);
    juce::AudioBuffer<float> buffer(2, 512);
    const auto start = std::chrono::steady_clock::now();
    double peak = 0;
    const int blocks = static_cast<int>(rate * 60 / 512);
    for (int b = 0; b < blocks; ++b)
    {
        for (int s = 0; s < 512; ++s)
        {
            buffer.setSample(0,s, 0.8f * static_cast<float>(std::sin((b*512+s)*0.13)));
            buffer.setSample(1,s, 0.7f * static_cast<float>(std::sin((b*512+s)*0.037)));
        }
        engine.process(buffer, 2);
        for (int c = 0; c < 2; ++c)
            for (int s = 0; s < 512; ++s)
            {
                const float x = buffer.getSample(c,s);
                if (!std::isfinite(x)) throw std::runtime_error("nonfinite long-run output");
                peak = std::max(peak, static_cast<double>(std::abs(x)));
            }
    }
    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout << "60s stereo/192k max settings: " << elapsed << "s wall time (" << elapsed/60*100
              << "% of realtime, harness included), peak " << peak << '\n';
    check(peak < 5, "long duration bounded peak");
    buffer.clear(); buffer.setSample(0,0,std::numeric_limits<float>::quiet_NaN());
    buffer.setSample(1,0,std::numeric_limits<float>::infinity()); engine.process(buffer,2);
    for (int b=0; b<20; ++b)
    {
        buffer.clear(); engine.process(buffer,2);
        check(std::isfinite(buffer.getMagnitude(0,512)), "bad input cannot poison histories");
    }

    SleepwalkRadioProcessor first, second;
    for (size_t i = 0; i < parameterSpecs.size(); ++i)
        first.parameters.getParameter(parameterSpecs[i].id)->setValueNotifyingHost(i == 7 ? 0.0f : 0.64f);
    juce::MemoryBlock state; first.getStateInformation(state);
    second.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    first.prepareToPlay(48000,128); second.prepareToPlay(48000,128);
    juce::AudioBuffer<float> a(2,128), b(2,128); juce::MidiBuffer midi;
    for (int block=0; block<100; ++block)
    {
        for (int c=0;c<2;++c) for(int s=0;s<128;++s) a.setSample(c,s,0.2f*static_cast<float>(std::sin((block*128+s)*0.1)));
        b.makeCopyOf(a);
        beginAllocationCheck();
        first.processBlock(a,midi); second.processBlock(b,midi);
        check(endAllocationCheck() == 0, "processor callback allocates no C++ memory");
        for(int c=0;c<2;++c) for(int s=0;s<128;++s) check(a.getSample(c,s)==b.getSample(c,s), "restored state deterministic after prepare");
    }
    // Frozen schema-1 shape from the Phase 1 serializer, independent of the current writer.
    const auto legacy = juce::parseXML(R"(<SleepwalkRadioState schemaVersion="1">
        <PARAM id="warmth" value="25"/><PARAM id="wobble" value="40"/>
        <PARAM id="flutter" value="30"/><PARAM id="age" value="20"/>
        <PARAM id="hiss" value="0"/><PARAM id="mix" value="65"/>
        <PARAM id="output" value="-3"/><PARAM id="bypass" value="0"/>
        </SleepwalkRadioState>)");
    juce::MemoryBlock legacyBytes; juce::AudioProcessor::copyXmlToBinary(*legacy, legacyBytes);
    first.setStateInformation(legacyBytes.getData(), static_cast<int>(legacyBytes.getSize()));
    constexpr std::array<float,8> expected {25,40,30,20,0,65,-3,0};
    for (size_t i=0;i<expected.size();++i)
        check(first.parameters.getRawParameterValue(parameterSpecs[i].id)->load()==expected[i], "Phase 1 schema fixture compatible");
    // Restore while running, then force the separate host bypass callback with non-neutral settings.
    for (int block=0;block<40;++block)
    {
        for(int c=0;c<2;++c) for(int s=0;s<128;++s) a.setSample(c,s,0.2f);
        beginAllocationCheck();
        first.processBlockBypassed(a,midi);
        check(endAllocationCheck()==0, "host bypass callback no allocations");
        for(int c=0;c<2;++c) for(int s=0;s<128;++s)
        {
            check(std::isfinite(a.getSample(c,s)), "runtime restore/bypass finite");
            if(block>10) check(a.getSample(c,s)==0.2f, "host bypass returns aligned unity dry");
        }
    }
}
}

int runTapeEngineTests()
{
    try
    {
        matrix(); characterisation(); mixNoiseStereoAndContinuity(); longRunAndState();
        std::cout << "PASS: " << checks << " DSP checks\n"; return 0;
    }
    catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
