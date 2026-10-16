# SleepwalkRadio project status

Updated October 16, 2026. Distribution version **1.0.0-rc.1**; numeric host/Windows version **1.0.0**.

## Current phase

**Phase 4 release candidate prepared. Stable release is blocked by mandatory manual validation.** User selected AGPLv3. Native DAW, device playback, Windows display-scale interaction and subjective listening remain unverified. Phase 3 history is preserved in `docs/phase-reports/Phase3.md`; Phase 2 DSP evidence remains in its historical reports.

No DSP or UI redesign was needed: source review and the fresh actual render showed no clipping, overlap or parameter changes. The final build statically links the Microsoft C++ runtime. Added a genuine Inno Setup installer, portable VST3 ZIP, corresponding-source ZIP including pinned JUCE, license/notices, SHA256 checksums, release notes and reproducible packaging script.

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

Product/target SleepwalkRadio; manufacturer Swrd; plugin Swr1; bundle local.sleepwalkradio.effect. Component CID ABCDEF019182FAEB5377726453777231; controller CID ABCDEF011234ABCD5377726453777231. Version 1.0.0 manifest confirms stable IDs. Candidate suffix 1.0.0-rc.1 is carried in CMake release-version metadata, installer display text, documentation and package filenames; JUCE numeric version encoding cannot accept semantic prerelease suffixes.

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

- Fresh Phase 4 Windows x64 Release build **passed, exit 0**: VST3, standalone, shared code and tests, with static MSVC runtime.
- Fresh full suite **3/3 passed, 0 failures, 6.20 seconds**: 97,978 Phase1 + 797,129 DSP + 13,246 editor = **908,353 checks**. These counts are from this phase's execution, not copied as new evidence from Phase 3. Most checks are sample assertions. Log: `docs/phase-reports/Phase4-tests.txt` (machine-specific root replaced with `<project>` for public distribution).
- Matrix covers mono/stereo, 44.1/48/88.2/96/192 kHz, variable/zero/oversized buffers, continuity, extremes, state, bypass, latency and C++ allocation checks. Editor suite covers all eight control attachments, text/keyboard gestures, restoration, resizing, pixel-scale renders, animation off, instance isolation and 30 reopen cycles. Native mouse/host interaction is separately unverified.
- Fresh offscreen base render visually inspected; no clipping or layout defect found. Pixel-scale rendering checks passed. Native Windows DPI interaction remains unverified.
- DSP stress: 60 seconds stereo at 192 kHz processed in **2.29353 seconds**, all finite, peak **1.41811**. Overlay render **1.46429 ms/frame**. Harness measurements are not a DAW CPU/dropout guarantee.
- VST3 manifest helper loaded the fresh module and emitted stable IDs/version 1.0.0. `dumpbin /dependents` lists Windows system DLLs only; no VCRUNTIME/MSVCP DLL dependency. No independent pluginval/SDK validator run.
- Inno Setup compiled an actual PE installer successfully. Portable/source ZIP integrity checked; packaged VST3 SHA256 matches the build. Exact asset hashes are in `dist/SHA256SUMS.txt`.
- Standalone Computer Use launch denied: **"Computer Use was not approved to use sleepwalkradio"**. No retry. Prior REAPER denial also respected; no native playback or subjective quality claim.

## Artifacts

- Installer: `dist/SleepwalkRadio-1.0.0-rc.1-Windows-x64-Setup.exe`.
- Portable: `dist/SleepwalkRadio-1.0.0-rc.1-Windows-x64-VST3.zip`.
- Corresponding source: `dist/SleepwalkRadio-1.0.0-rc.1-Source.zip` (project plus full pinned JUCE source, excluding Git metadata).
- Checksums: `dist/SHA256SUMS.txt`.
- Standalone build only: `build/SleepwalkRadio_artefacts/Release/Standalone/SleepwalkRadio.exe`. Excluded from downloadable packages because playback is unverified.
- Preview: `docs/phase-reports/MidnightReceiver.png`.

Packaging command: `python packaging/package.py`. Inno Setup 6 is required only to build the installer. Use `--reuse-installer` to refresh documentation/source archives without rebuilding an already-tested installer. The installer targets `{commoncf64}\VST3\SleepwalkRadio.vst3`, requires administrator rights, and installs its uninstaller/notices under ProgramData/SleepwalkRadio. It includes no development tools. It uses Inno's tracked-file uninstall, with no recursive wildcard deletion of shared plugin directories. Candidate is unsigned.

## Mandatory pending gates

Native DAW scan/playback, automation/session recall, latency compensation, mouse/fine/reset/keyboard interaction, actual Windows scaling, subjective listening on vocals/drums/synths/full mixes, multi-instance audio continuity and clean-machine installation remain **UNVERIFIED**. Screen-reader behavior and extended sessions are also unverified. Automated tests cannot establish these. Complete RELEASE_NOTES.md's checklist before stable v1.0.0 publication. No stable tag or stable release has been created.

Installer and uninstaller both executed successfully (exit 0) on this development machine. Installed binary matched the build; uninstall removed the bundle and uninstaller. User approved the administrator prompt. Clean-machine and upgrade validation remain pending. Details: `docs/phase-reports/Phase4-packaging.md`.

## Git/GitHub handoff

Initialized a new local main branch; there was no existing repository or remote. Both GitHub connector and git remote lookup found no accessible `scryingorb/SleepwalkRadio` repository. Automatic approval review rejected creating a new public repository because explicit confirmation for that public resource is required. No repository creation, push, tag or asset upload occurred. Once approved, create that repository without initializing unrelated content, set origin, and push main without force. Any downloadable candidate must be marked prerelease and include installer, portable ZIP, source ZIP and checksums. Do not create stable v1.0.0 until manual gates pass.

## Changed files and license

CMake: 1.0.0 numeric version, 1.0.0-rc.1 release metadata, static MSVC runtime. Added packaging scripts, INSTALL.txt, LICENSE (AGPL-3.0-only), third-party notices and 31 license files, CHANGELOG and release notes. Updated README, plan, status and public phase evidence. Source/TapeEngine and existing DSP/control tests unchanged. Historical machine-specific paths normalized before Git review. Build/dependency/tool/dist/staging directories ignored. No presets or additional effects.
