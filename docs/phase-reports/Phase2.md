# SleepwalkRadio project status

Updated October 16, 2026. Product version **0.2.0**.

## Current phase

**Phase 2: Tape Engine implemented; Release build and automated verification passed.** DAW compatibility, standalone device playback and subjective listening remain pending. Stop at this boundary; Phase 3 has not begun.

The user's explicit Phase 2 request brought the original Phase 3 motion/hiss implementation into this phase. The plan records this scope change. No final artwork, presets, additional effects, installer, commit, push or release was added. Phase 1 history is preserved in `docs/phase-reports/Phase1.md`.

## Completed engine

Architecture: input -> 2x FIR-oversampled saturation -> shared variable-delay transport -> tone -> hiss -> aligned linear Mix -> Output -> aligned bypass crossfade.

| Stable ID | DSP mapping (normalized amount) | Range/default unchanged |
|---|---|---|
| warmth | tanh drive `1+3w²`, compensation `(1+0.5w)/drive`, blend by w with aligned clean signal | 0–100%, 0 |
| wobble | quadratic depth up to +/-4 ms, 0.23/0.371/0.073 Hz components plus smooth seeded wandering | 0–100%, 0 |
| flutter | quadratic depth up to +/-0.15 ms, weighted 6.31/9.73 Hz motion | 0–100%, 0 |
| age | two lowpass poles, exponential 18 kHz–2.5 kHz cutoff, blend by Age; zero exactly clean | 0–100%, 0 |
| hiss | independent seeded noise, 10 kHz/1.2 kHz shaping, amplitude `0.004h²`, centre-correlated | 0–100%, 0 |
| mix | linear blend with original aligned to neutral wet; zero excludes effects/hiss | 0–100%, 100 |
| output | dB gain after Mix, including dry Mix | -12 to +12 dB, 0 |
| bypass | smooth aligned unity dry, ignores Output; same host callback behavior | boolean, false |

Wobble/Flutter depth ramps over 150 ms, bypass over 10 ms, other controls over 30 ms. Output ramps in dB. APVTS values are sampled once per host block; smoothing advances per sample. Shared four-point cubic transport preserves stereo coherence, with no cross-channel signal mixing. Preallocated 1024-sample chunks support arbitrary host block sizes. No callback allocation, I/O, locking or logging. Nonfinite input becomes zero; extreme inputs/outputs are safety-bounded to +/-16. Normal Output boost can exceed full scale.

## Latency and lifecycle

Fixed reported latency is `ceil(sampleRate * 0.006) + 49` samples. The 49 samples are from 2x integer-latency FIR oversampling; the 6 ms transport base provides modulation headroom. Knobs and bypass never change latency.

| Rate | Samples | Milliseconds |
|---|---:|---:|
| 44.1 kHz | 314 | 7.12018 |
| 48 kHz | 337 | 7.02083 |
| 88.2 kHz | 579 | 6.56463 |
| 96 kHz | 625 | 6.51042 |
| 192 kHz | 1201 | 6.25521 |

Neutral audio is bit-exact after latency at Mix 0/50/100%. Settled bypass is aligned unity dry, including the separate host bypass callback. Nonzero modulation intentionally offsets wet relative to dry and can produce chorus/comb coloration at intermediate Mix. No zero-latency monitoring/bypass mode.

Declared finite tail: 50 ms for buffer/filter decay. Hiss continues on silence while the host processes audio but does not request an infinite tail. Prepare/reset clears histories and restarts independent fixed transport/noise seeds. State stores parameters, not evolving random/filter/delay histories. Reset renders are deterministic; arbitrary session resumes are not guaranteed sample-identical. Host reset must not run concurrently with processing.

## Stable identity and state

- Product/target `SleepwalkRadio`; manufacturer code `Swrd`; plugin code `Swr1`; bundle ID `local.sleepwalkradio.effect`.
- Manufacturer string `SleepwalkRadio` is a technical project label, not a claimed user company.
- Component CID `ABCDEF019182FAEB5377726453777231`; controller CID `ABCDEF011234ABCD5377726453777231`, confirmed unchanged in the new manifest.
- VST3 Fx, matching mono/stereo buses, no MIDI; double precision not advertised.
- Parameter IDs/order/ranges/defaults/version hints (all 1) unchanged. State root `SleepwalkRadioState`, schemaVersion 1; existing serializer/validator preserved.
- Phase 1 states recall unchanged parameter values; nondefault settings now activate DSP. Missing values use original defaults; invalid/unsupported data rejects as a whole. Live restoration is smoothed, not a sample-atomic multi-parameter transaction.

## Toolchain and commands

C++17, Visual Studio 2019 Build Tools 16.11.25, MSVC 19.29.30148.0 (v142), Windows SDK 10.0.19041.0, project-local CMake 3.31.6. JUCE 8.0.12 pinned to `29396c22c93392d6738e021b83196283d6e4d850`, including its DSP module and bundled VST3 SDK 3.8.0. No new dependency or system installation.

Exact PowerShell commands from the project root:

```powershell
.\.tools\cmake-python\cmake\data\bin\cmake.exe -S . -B build -G 'Visual Studio 16 2019' -A x64 -DFETCHCONTENT_SOURCE_DIR_JUCE="$pwd/.deps/JUCE"
.\.tools\cmake-python\cmake\data\bin\cmake.exe --build build --config Release --target SleepwalkRadioTests --parallel 4
.\.tools\cmake-python\cmake\data\bin\ctest.exe --test-dir build -C Release -R Phase2DSP --output-on-failure
.\.tools\cmake-python\cmake\data\bin\cmake.exe --build build --config Release --parallel 4
.\.tools\cmake-python\cmake\data\bin\ctest.exe --test-dir build -C Release --output-on-failure
```

## Actual verification results

- Final Release build **passed**, exit 0: VST3, standalone, shared processor and tests. Explicit test checks remain active in Release.
- Full suite **2/2 passed**, 0 failures, 3.90 seconds. Regression **97,978 checks** (0.72 seconds); DSP **797,129 checks** (3.17 seconds); **895,107 total**. Most are sample assertions, not independent scenarios.
- Exact output saved in `docs/phase-reports/Phase2-tests.txt`; working log `build/Testing/Temporary/LastTest.log`.
- Matrix: mono/stereo, 44.1/48/88.2/96/192 kHz, zero/tiny/variable/oversized blocks, extremes, silence, aligned neutrality, output bounds, channel clearing, reset/reprepare. Rendering with 32 vs 2053 sample blocks is identical, including noise.
- State/UI regressions: malformed-state rejection, missing defaults, automation flags, attachments, ten editor lifecycles. New tests: fixed schema-1 fixture, runtime restore, deterministic recall after prepare, separate host bypass callback, regular/aligned C++ allocation interception (zero allocations in checked callbacks).
- Maximum wow pitch RMS deviation on 1 kHz tone: **2.18081 Hz**; flutter **3.55162 Hz**. Successive-cycle changes **0.00410408 vs 0.1759 Hz**, confirming faster flutter.
- Maximum Warmth third/fundamental **-12.2376 dB** at 0.8-peak 1 kHz input. RMS gain at 0.05/0.25/0.8 peak: **+3.4356 / +1.72885 / -4.59266 dB**. Compensation is approximate and level-dependent, not automatic loudness normalization.
- Maximum Age: **-23.3576 dB at 10 kHz**, **-0.013763 dB at 100 Hz**, at 48 kHz.
- 2x FIR folded-third-harmonic reduction vs native tanh: **97.7927 dB at 44.1 kHz / 78.762 dB at 48 kHz**, 10 kHz/0.8-peak/max-Warmth fixture. Supports the extra 49-sample latency and CPU cost; not an alias-free claim for all signals.
- Maximum Hiss **-56.4268 dBFS RMS** at 48 kHz. Deterministic, independent of Age, exactly absent when disabled/fully dry/settled bypassed.
- Rapid automation DC maximum adjacent step **0.000193194**, below threshold 0.01. Isolates parameter discontinuities; does not guarantee inaudibility on all sources/hosts.
- **60 seconds stereo at 192 kHz, maximum effects and +12 dB Output:** **2.4929 seconds wall time (4.15483% of real-time duration)**, peak **1.41811**, all finite. Intel Core i5-13600K identified via registry. Timing includes the harness; it is not a DAW CPU or multi-instance guarantee.
- VST3 manifest helper successfully loaded version 0.2.0 and generated metadata with stable IDs. No pluginval on PATH or independent SDK validator executable available; no full external validator ran.
- Temporary editor snapshot rendered and visually inspected; readable Phase 2 heading, controls and units.
- Initial build failed when a test-target update during compilation caused competing CMake regenerations/stale linking. Separate configure and incremental rebuild resolved it. Final build passed; no outstanding build defect.

## Generated artifacts

Relative to `<project>`:

- VST3 bundle: `build/SleepwalkRadio_artefacts/Release/VST3/SleepwalkRadio.vst3` (copy the whole folder).
- Standalone: `build/SleepwalkRadio_artefacts/Release/Standalone/SleepwalkRadio.exe`.
- Tests: `build/Release/SleepwalkRadioTests.exe`.
- Editor preview: `build/editor-preview.png`.

## Known limitations and manual checklist

No failing automated checks are known. Real DAW automation/session recall/latency compensation, device playback, subjective musical quality, high-DPI interaction and hours-long/many-instance runs remain unverified. C++ allocation interception is not a general OS/malloc/locking profiler. Source review supports the real-time callback design.

The prior REAPER denial **"Computer Use was not approved to use reaper"** was respected and not retried. No DAW compatibility or listening pass is claimed.

1. Record DAW/version and audio settings; scan/load 0.2.0, test mono/stereo and standalone separately.
2. Null-test neutral Mix 0/50/100% with latency compensation; confirm settled bypass is unity dry despite Output changes.
3. Loudness-match low/medium/high Warmth and Age on vocals, guitars, synths, bass, drums and full mixes; listen for high-frequency artifacts and transient loss.
4. Check Wobble/Flutter separately/together on sustained notes, stereo imaging and intermediate Mix coloration.
5. Verify Hiss zero on silence, Age independence, dry Mix excluding effects/noise, and Output on dry Mix.
6. Automate every parameter, toggle host/plugin bypass, restart transport, reopen editors, save/reopen sessions and verify recall; change sample rates/block sizes and test multiple instances.

Distribution licensing remains undecided; README retains JUCE/third-party links. No commercial entitlement or release readiness is assumed.

## Changed files and next task

Added `Source/TapeEngine.h/.cpp`, `Tests/TapeEngineTests.cpp`, `Tests/RealtimeAllocation.cpp`, and phase reports. Updated `Source/PluginProcessor.h/.cpp`, `Source/PluginEditor.cpp`, `Tests/Phase1Tests.cpp`, `CMakeLists.txt`, README, plan and this status. Existing state validation and editor architecture were preserved.

Recommended Phase 3: listening refinement and real-host validation, fixing demonstrated defects and calibrating ranges before the final Midnight Receiver UI. This replaces the original Phase 3 motion/hiss implementation expressly moved into Phase 2. Confirm the next scope with the user; do not start automatically. No commit/push/publication performed; the workspace was originally not a Git repository.
