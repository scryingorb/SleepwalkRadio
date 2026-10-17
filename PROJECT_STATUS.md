# SleepwalkRadio - Dream Transmission status

Updated October 16, 2026. Numeric version 1.1.0, development designation 1.1.0.

## Scope and diagnosis

User reported weak and unsatisfactory sound in real-world use. The original quadratic depth curves gave only 25% maximum depth at half travel; low Wobble/Flutter and Hiss settings were particularly restrained. Warmth combined quadratic drive with an additional linear clean blend and fixed level-dependent compensation. Age was only a two-pole lowpass, with no transient diffusion, wear events or stereo drift. No preset system existed. These effects were measurable, but the mappings hid much of the useful range.

The user's panel clarification was to keep it simple. All eight IDs, ranges/defaults and knob positions remain unchanged. Age combines tone wear, upper-band blur and dropout; Wobble combines slow pitch motion and bounded stereo amplitude drift. No extra audio parameters or knobs. Footer now has a radio-styled preset button. Existing faceplate, meter, animation, bypass, scaling and control gestures retained.

## Engine changes

- Wobble: maximum delay depth 18 ms, power curve 1.35, irregular seeded smooth wander plus phase modulation. Rate grows toward extreme settings (multiplier 1 + 2.2w^2).
- Flutter: maximum 0.7 ms, power curve 1.2, two fast components plus independent smooth jitter. Both share the existing cubic fractional-delay transport.
- Warmth: drive 1 + 15w^1.2, square-root character blend, retained 2x FIR oversampling. Stereo-linked input/output energy matching with smoothed compensation and a feed-forward drive-ramp ceiling. No independent channel gain pumping.
- Age: 18 kHz to 850 Hz exponential tone, mild low-band body, upper-band allpass diffusion, mild dynamic wear and seeded irregular 20-110 ms gain/high-frequency dropouts. Maximum event loss is bounded below 85%, with smooth sin-squared fades. Low settings have rare events. Diffusion deliberately excludes the low band after an initial test caught bass cancellation.
- Hiss: retained colored/noise generator architecture; stronger perceptual curve 0.018h^1.35. Independent of Age and silent at zero.
- Stereo drift is amplitude-only (+/-14% times Wobble), with no differential channel delay or crossfeed. Mono stays mono. Original center-correlated Hiss retained.

Fixed reported latency increased to ceil(sampleRate * 0.024) + 49: 1108/1201/2166/2353/4657 samples at 44.1/48/88.2/96/192 kHz. At 48 kHz this is 25.0208 ms. Extra delay headroom is required for the deeper transport range; dry/wet/bypass remain aligned. Finite tail is now 100 ms. Old parameter values remain readable, but intentionally produce different sound. Keep the previous binary for exact historical sound.

All buffers allocate in prepare. No processBlock allocation, locks, filesystem operations or rendering. Fixed random seeds reset deterministically. Noise/delay histories are not serialized. Positive Output can exceed full scale; the plugin is not a mastering limiter.

## Presets and recall

Neutral Init plus twelve factory voices: Warm Memories, Midnight Radio, Melted Cassette, Dream Sequence, Broken Walkman, VHS Sunset, Underwater, Ghost Transmission, Old Photograph, Lost Signal, Analog Heaven, Sleepwalking. Values are explicit and reviewable in Source/FactoryPresets.h; no random preset generation.

Selection changes APVTS parameters and brackets UI host automation gestures. Host programs expose the same list. An asterisk marks any deviation from the selected voice. The optional presetIndex property (0-12) lives in schema-1 state; old states default to Init while retaining every saved value. Restore applies actual saved values, never resets edited values to factory defaults. Invalid preset metadata rejects restoration. Editor notifications use a deferred message-thread refresh independent of animation, including program changes with unchanged parameter values.

## Verification

Exact commands from project root:

```powershell
.\.tools\cmake-python\cmake\data\bin\cmake.exe -S . -B build -G 'Visual Studio 16 2019' -A x64 -DFETCHCONTENT_SOURCE_DIR_JUCE="$pwd/.deps/JUCE"
.\.tools\cmake-python\cmake\data\bin\cmake.exe --build build --config Release --parallel 4
.\.tools\cmake-python\cmake\data\bin\ctest.exe --test-dir build -C Release --output-on-failure
```

Tests preserve existing state, latency-aligned neutrality, variable/zero/oversized blocks, mono/stereo, reset, extremes, nonfinite input, deterministic partitioning, bypass, channel clearing and callback allocation checks. Updated characterization assertions match intentional sound changes rather than requiring the old sound. Additional tests cover quarter/half/three-quarter/max modulation, 100 Hz bass retention, upper-band blur impulse energy, dropout continuity/non-silencing, impulse-measured latency, audible midrange modification, pairwise differences between all twelve preset renders, every preset/rate combination, edited recall, invalid metadata and motion-off name/modified updates. All eight registration IDs/defaults remain unchanged.

Fresh render: docs/phase-reports/DreamTransmission.png. Native UI access was previously denied, so no retry or native interaction/listening pass is claimed. Actual offscreen rendering and scale/layout tests are distinguished from OS/host testing.

Final verification:

- Windows x64 Release VST3, standalone and tests built successfully, exit 0. VST3 manifest helper loaded version 1.1.0 with unchanged class IDs.
- Full suite: **3/3 passed, 10.71 seconds, 912,478 assertions**: 97,978 foundation + 797,214 DSP + 17,286 editor/preset. Saved output: docs/phase-reports/DreamTransmission-tests.txt. Most are sample assertions, not independent scenarios.
- 1 kHz pitch RMS variation at 25/50/75/100%: Wobble **1.46/4.46/12.25/25.75 Hz**, Flutter **2.22/5.10/8.29/11.71 Hz** (8-second fixture). Separate 12-second maximum fixture: wow 25.31 Hz versus prior 2.18; flutter 11.75 versus prior 3.55.
- Maximum Warmth steady 1 kHz RMS gain at 0.05/0.25/0.8 peak input: **-0.0015/-0.0002/-0.0019 dB**. Transient/source-dependent behavior remains a listening check.
- Maximum Age: **-54.48 dB at 10 kHz**, **+0.25 dB at 100 Hz**. This includes the combined tone/blur/wear macro, not isolated static EQ. First diffusion design failed bass preservation and was corrected before this pass.
- Maximum Hiss: **-43.36 dBFS RMS**, versus prior -56.43 dBFS; still independent and exactly disabled at zero/dry/bypass.
- Maximum Age on 0.25 DC: minimum **0.0467**, largest adjacent change **0.000481**; rapid parameter automation DC maximum step **0.000349**. No forced silence or abrupt event edge in these fixtures.
- Same-drive, RMS-normalized folded-third-harmonic reduction versus native tanh at 10 kHz: **110.77 dB (44.1 kHz), 29.65 dB (48 kHz)**. This is a specific anti-alias fixture, not a broadband alias-free claim.
- 60-second stereo 192 kHz maximum-settings stress: **5.273 seconds** wall time (**8.79%** of duration), peak **1.229**, all finite; harness included, concurrent build activity may affect timing. No DAW CPU guarantee. Actual overlay render 2.61 ms/frame.
- Preset suite caught a stale name with Motion Off; fixed by deferred processor-change notification. All factory choices, modified labels and edited recall now pass.
- Built plugin SHA256: **98605003229B72AB272027D3DC442ECDF98384EE7496DEFBC2B152F53416A3E6**.
- Local installation completed successfully after the user authorized terminating the leftover FL Studio process and its exit was confirmed. Installed complete bundle at `C:\Program Files\Common Files\VST3\SleepwalkRadio.vst3`; both files verified against the build by SHA256, metadata version 1.1.0. Prior installed bundle preserved at `build/installed-before-DreamTransmission/SleepwalkRadio.vst3`. No rebuild, package or Git operation was needed for installation.

## Limitations and delivery boundary

Subjective voicing, perceived loudness on transient sources, hardware playback, host automation/session recall and native Windows display scaling require manual validation. Source tests show measurable changes, not musical quality. Presets are starting points pending listening. Saturation compensation is intentionally level-adaptive and can change transient envelopes. More extreme modulation/deterioration is intentionally audible; intermediate Mix may have chorus coloration.

Dream Transmission source was pushed as 809616b. The user subsequently authorized a release with installer. Distribution designation is now 1.1.0, with installer, portable VST3, matching source including JUCE, and checksums. The user explicitly chose a regular release rather than a prerelease; incomplete manual validation remains disclosed. Numeric plugin version and tested audio/UI source are unchanged. Historical 1.0.0-rc.1 packages are separate older builds.
