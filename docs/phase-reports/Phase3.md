# SleepwalkRadio project status

Updated October 16, 2026. Product version **0.3.0**.

## Current phase

**Phase 3: Midnight Receiver implemented and automated checks passed.** Native DAW, device playback, Windows display-scale interaction and subjective listening remain unverified. Phase 4 has not begun. Phase 2 history and DSP measurements are preserved in `docs/phase-reports/Phase2.md` and `Phase2-tests.txt`.

## Implemented interface

- Original JUCE vector receiver: charcoal brushed metal, bevels, screws, vents, amber tuning glass, decorative ticks, output-level needle, subtle lavender trace and custom machined knobs. No external artwork, assets or dependencies added.
- All eight existing controls: **Warmth, Age, Hiss / Wobble, Flutter / Mix, Output, Bypass**, grouped by function. Bypass is a two-position rotary with Off/On text. Header indicates ACTIVE/BYPASS.
- APVTS attachments, editable values, Shift-drag fine adjustment, double-click reset, keyboard arrows, Space for Bypass, visual tab order, focus rings, accessible names and tooltips.
- Base 960 x 560, proportional bounds 768 x 448 to 1440 x 840. Cached static faceplate and independently updated overlay; vector drawing at device scale.
- Lock-free atomic snapshot of actual output peak; no UI access to mutable DSP buffers. Decorative needle illustrates Wobble/Flutter settings with UI oscillators, not the exact delay trajectory. Trace is stylized level feedback, not an oscilloscope. No additional audio generator.
- Active animation 30 Hz; idle/hidden polling 4 Hz without continuous repaint. Motion Off stops the timer and observer publication. Closing unregisters observation and destroys the timer. Motion preference survives editor reopening per instance, but is not serialized or a ninth parameter.

## Preserved engine, identity and state

TapeEngine.cpp/.h are unchanged from Phase 2 (SHA256 B462A3EBDF849EE7B5B2BACB84F59D33EAF91831CBBEAD0C6CB4E63DBC51F354 / 3813A6BA70587788F5BEE2DBC5D6603E3B541BE8A8F570A25DE5507BD22C3FEC). Processing remains oversampled saturation -> shared variable-delay transport -> tone -> hiss -> aligned Mix -> Output -> aligned bypass. Fixed latency remains 337 samples / 7.02083 ms at 48 kHz. See README and Phase2 report for all mappings, smoothing, latency rates and sound limitations.

IDs/order remain warmth, wobble, flutter, age, hiss, mix, output, bypass. Character controls default zero, Mix 100%, Output 0 dB, Bypass false. Ranges, parameter version hints and schema-1 validator/serializer unchanged. State root SleepwalkRadioState. Mono/stereo Fx, no MIDI.

Product/target SleepwalkRadio; manufacturer Swrd; plugin Swr1; bundle local.sleepwalkradio.effect. Component CID ABCDEF019182FAEB5377726453777231; controller CID ABCDEF011234ABCD5377726453777231. Version 0.3.0 manifest confirms stable IDs.

## Build and verification

Toolchain: C++17, VS 2019 Build Tools 16.11.25 / MSVC 19.29, Windows SDK 10.0.19041, local CMake 3.31.6, JUCE 8.0.12 pinned to 29396c22c93392d6738e021b83196283d6e4d850. No system installation.

Exact PowerShell commands from the project root:

```powershell
.\.tools\cmake-python\cmake\data\bin\cmake.exe -S . -B build -G 'Visual Studio 16 2019' -A x64 -DFETCHCONTENT_SOURCE_DIR_JUCE="$pwd/.deps/JUCE"
.\.tools\cmake-python\cmake\data\bin\cmake.exe --build build --config Release --target SleepwalkRadioTests --parallel 4
.\.tools\cmake-python\cmake\data\bin\ctest.exe --test-dir build -C Release -R Phase3Editor --output-on-failure
.\.tools\cmake-python\cmake\data\bin\cmake.exe --build build --config Release --parallel 4
.\.tools\cmake-python\cmake\data\bin\ctest.exe --test-dir build -C Release --output-on-failure
```

- Windows x64 Release build **passed, exit 0**: VST3, standalone, shared code and test executable.
- Full suite **3/3 passed, 0 failures, 4.33 seconds**: 97,978 Phase1 + 797,129 DSP + 13,246 editor = **908,353 checks**. Most are sample assertions, not independent scenarios. Exact log: `docs/phase-reports/Phase3-tests.txt`.
- Editor checks cover all eight host-to-UI values, text/keyboard begin/end gestures, restoration, independent instances, bounds/nonoverlap, pixel-scale renders, motion-off behavior, 30 reopen cycles and identical audio samples with/without visualization observation. Checked callbacks allocate no C++ heap memory; this is not a general OS/malloc/locking profiler.
- Visually inspected actual JUCE offscreen images at 100/125/150/200% and minimum/maximum sizes. Labels and values remain accessible without clipping. These are actual component renders, not native OS/DAW screenshots and not proof of Windows display-scale interaction.
- Overlay rendering measured **1.63028 ms/frame** at 960 x 560 in the offscreen harness. Not a DAW CPU or multi-instance guarantee. DSP 60-second stereo 192 kHz stress: 2.44635 seconds wall time, peak 1.41811, all finite (Intel Core i5-13600K).
- VST3 manifest helper loaded version 0.3.0 successfully and emitted unchanged class IDs. No independent pluginval/SDK validation run.
- Initial editor compile found ambiguous integer alpha overloads; corrected to float values. Focused tests and final suite pass.

## Artifacts

- VST3: `build/SleepwalkRadio_artefacts/Release/VST3/SleepwalkRadio.vst3` (copy the whole bundle).
- Standalone: `build/SleepwalkRadio_artefacts/Release/Standalone/SleepwalkRadio.exe`.
- Tests: `build/Release/SleepwalkRadioTests.exe`.
- Preview: `docs/phase-reports/MidnightReceiver.png`; additional scale/size images: `build/receiver-*.png`.

## Remaining gates and suggested Phase 4

No failing automated checks are known. Real DAW automation/session recall/latency compensation, actual Windows 100/125/150/200% scaling and keyboard/mouse behavior, standalone device playback, screen-reader behavior, multi-instance CPU/audio continuity, and hours-long sessions remain pending. The UI tests do not prove dropout-free hardware playback or native host timer behavior. Subjective sound quality on vocals, drums, synths and full mixes remains unverified.

The prior REAPER denial **"Computer Use was not approved to use reaper"** was respected and not retried. No host/listening pass is claimed.

Suggested Phase 4: record the chosen DAW/version and device settings, complete README's manual host checklist, exercise animation on/off and resize at native display scales, audition gentle/medium/extreme settings with loudness matching, and fix demonstrated defects. No new effects or DSP calibration without evidence. Presets and packaging remain later work. Distribution licensing and clean-machine package validation remain release gates.

## Changed files

Added Source/MidnightReceiver.h/.cpp and Tests/EditorTests.cpp; replaced the temporary PluginEditor.h/.cpp; added optional visualization atomics/publication to PluginProcessor.h/.cpp; updated CMakeLists.txt (0.3.0 and editor test), Tests/Phase1Tests.cpp, README, plan, status and phase evidence. TapeEngine and Phase2 DSP tests retained unchanged. No commit, push, installer or release publication performed.
