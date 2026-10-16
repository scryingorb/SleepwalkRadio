# SleepwalkRadio project status

Updated: October 16, 2026.

## Current phase

Phase 1 implementation and automated verification complete. **Manual host and standalone playback gates remain pending.** Stop here for foundation review; Phase 2 has not begun.

## Completed

- Established a new C++17 / CMake / JUCE Windows x64 project in the previously empty workspace. No applicable AGENTS.md or prior source/build/identity existed.
- Copied the supplied Downloads plan into `docs/SleepwalkRadio_Plan.md`, correcting all old product spellings while preserving the agreed phased plan. The user's original Downloads file is unchanged.
- Release VST3 effect and standalone development targets, no MIDI, zero latency/tail, matching mono/stereo only.
- Transparent in-place audio; parameters, output trim, mix, and bypass do not change samples in this phase. Extra output channels clear. Callback contains no allocation, locks, I/O, or logging.
- Eight host-automatable APVTS parameters with stable IDs, version hints, readable values/units, and host bypass contract.
- Schema 1 validated state restore, complete candidate before replacement, default fallback for missing parameters, whole-state rejection for unsupported/malformed data. See README for policy.
- Temporary 900 x 480 editor: six primary knobs, smaller Output, Bypass, attachments, text entry, reset, keyboard focus/arrow gestures, accessible names, Shift fine adjustment.
- Focused CTest target, reproducible commands, ignored dependencies/tools/build products, README and manual checklist.

## Stable identity

- Product/build target: `SleepwalkRadio`, version `0.1.0`.
- Manufacturer code: `Swrd`; plugin code: `Swr1`.
- Bundle ID: `local.sleepwalkradio.effect`.
- Manufacturer string: `SleepwalkRadio` (technical project label, not a claimed developer company).
- Generated VST3 component CID: `ABCDEF019182FAEB5377726453777231`.
- Generated VST3 controller CID: `ABCDEF011234ABCD5377726453777231`.
- Parameters, in order: `warmth`, `wobble`, `flutter`, `age`, `hiss`, `mix`, `output`, `bypass`. All parameter version hints are 1. Preserve IDs, order, and identity in future phases.
- State root: `SleepwalkRadioState`; `schemaVersion=1`. No legacy product state or distributed build existed.

## Verified toolchain

- Visual Studio Build Tools 2019 16.11.25; MSBuild 16.11.2.
- MSVC compiler 19.29.30148.0, toolset directory 14.29.30133 (v142).
- Windows SDK 10.0.19041.0, x64 generator.
- CMake 3.31.6 installed inside ignored `.tools/cmake-python`. Installed VS-bundled CMake 3.20.21032501 was below JUCE's minimum 3.22.
- JUCE 8.0.12, commit `29396c22c93392d6738e021b83196283d6e4d850`, downloaded into ignored `.deps/JUCE`. CMake's normal FetchContent path pins that same commit.
- Bundled VST3 SDK reports 3.8.0 in the generated manifest.
- C++17. No new system-wide toolchain installation performed.

## Exact commands run

From the project root, PowerShell:

```powershell
git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git .deps/JUCE
python -m pip install --target .tools/cmake-python cmake==3.31.6
.\.tools\cmake-python\cmake\data\bin\cmake.exe -S . -B build -G 'Visual Studio 16 2019' -A x64 -DFETCHCONTENT_SOURCE_DIR_JUCE="$pwd/.deps/JUCE"
.\.tools\cmake-python\cmake\data\bin\cmake.exe --build build --config Release --parallel 4
.\.tools\cmake-python\cmake\data\bin\ctest.exe --test-dir build -C Release --output-on-failure
```

## Actual results

- Configure: succeeded with Windows x64 MSVC and SDK identified above.
- Final Release build: succeeded, exit 0, VST3 + Standalone + tests. No separate Debug build was needed; tests ran in Release with explicit checks that are not compiled out by NDEBUG.
- Final CTest: **1/1 passed, 78,214 checks**, 0 failed, 0.32 seconds reported. Evidence: `build/Testing/Temporary/LastTest.log`.
- Covered defaults, automation flags, host bypass contract, all-parameter state round-trip, malformed/truncated/wrong-root state, unsupported versions, bad values, duplicate/unknown IDs, missing values/default fallback, nonboolean bypass.
- Real buffers: matching mono and stereo with different channels; 44.1/48/88.2/96 kHz; 0/1/32/64/128/256/512/1024/137 samples; parameters at 0/0.37/1 normalized; exact sample equality, silence, bypass, extra-channel clearing, repeated prepare/release/reset; rejects unsupported buses.
- Ten editor create/destroy cycles, keyboard edits, host-to-editor and editor-to-parameter attachment updates, restore with open editor; rendered editor snapshot.
- Visually inspected `build/editor-preview.png`: readable controls and units. Fixed Windows source-encoding artifacts in separator text, rebuilt, reran tests, and inspected the corrected image.
- JUCE VST3 manifest helper successfully loaded the built module and generated `Contents/Resources/moduleinfo.json`. This is module loading/metadata evidence, not a full plugin validator or DAW test.
- No pluginval executable was found on PATH. No independent plugin validator ran.

## Generated artifacts

Relative to this project root (`<project>`):

- VST3 bundle: `build/SleepwalkRadio_artefacts/Release/VST3/SleepwalkRadio.vst3`
- VST3 binary: `build/SleepwalkRadio_artefacts/Release/VST3/SleepwalkRadio.vst3/Contents/x86_64-win/SleepwalkRadio.vst3`
- Standalone: `build/SleepwalkRadio_artefacts/Release/Standalone/SleepwalkRadio.exe`
- Test executable: `build/Release/SleepwalkRadioTests.exe`
- Editor preview: `build/editor-preview.png`

## Pending manual checks and blockers

REAPER exists at `C:\Program Files\REAPER (x64)\reaper.exe`. A Computer Use launch attempt returned exactly **"Computer Use was not approved to use reaper"**. No REAPER scan, insert, playback, automation, or save/reopen action was performed. Its version and the user's chosen primary DAW are unconfirmed. This is an application-access blocker, not a demonstrated plugin defect. No specific DAW compatibility is claimed.

Before accepting the Phase 1 host gate:

- Record chosen DAW/version and audio device/settings.
- Scan and load the VST3 on an audio track; verify mono/stereo as supported by the host.
- Listen and compare/null against dry audio, including silence and bypass.
- Move every control; test keyboard, fine adjustment, text entry, and reset.
- Write/read automation for every parameter and check UI synchronization.
- Save nondefault values, close/reopen the DAW session, verify full recall.
- Toggle host/plugin bypass, restart transport, and repeatedly reopen the editor.
- Launch standalone, configure input/output, and verify real audio-device playback separately.

No known failing automated checks. Remaining uncertainty: host wrapper behavior under real DAW automation/session restore, actual device audio/listening, high-DPI interaction, and manual accessibility behavior. These are not reported as passed.

Distribution licensing remains undecided. JUCE's AGPLv3/commercial licensing and third-party notices are linked in README; this project does not assume a commercial entitlement or pick the user's distribution licence. Resolve the appropriate licensing route before distributing.

## Deviations and next task

- Local CMake 3.31.6 was necessary because the installed bundled version was too old. This leaves system tools unchanged.
- The plan was copied/renamed inside the workspace, preserving the original supplied Downloads file.
- Manual DAW validation could not proceed because REAPER app access was denied. Standalone playback also remains a manual gate; processor tests do not substitute for it.
- No tape DSP, final artwork, presets, installation, publishing, or Phase 2 work added.
- No commit hash: the starting workspace was not a Git repository; no repository initialization, commit, push, or publication was requested/performed.

Recommended next task: review this foundation and complete the pending host/playback checklist. Address any demonstrated defects, then explicitly authorize Phase 2 (Warmth, Age, Output, and Mix).

Relevant files: `CMakeLists.txt`, `Source/PluginProcessor.h/.cpp`, `Source/PluginEditor.h/.cpp`, `Tests/Phase1Tests.cpp`, `README.md`, `docs/SleepwalkRadio_Plan.md`.
