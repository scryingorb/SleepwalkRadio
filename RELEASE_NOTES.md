# SleepwalkRadio 1.1.0 - Dream Transmission

**Windows x64 release. Native DAW/listening validation remains incomplete.**

- Stronger, smoother Wobble and Flutter with irregular tape motion.
- Level-adaptive oversampled Warmth, more audible Hiss, and Age combining tone wear, upper-band blur and smooth dropouts.
- Twelve factory voices plus Init, with a preset selector and modified-state indicator.
- Retains the eight-control Midnight Receiver interface, parameter IDs/defaults and schema-1 state compatibility.

Old saved settings load but intentionally sound different. Preserve the previous binary for exact historical sound. Fixed latency is now 1201 samples (25.02 ms) at 48 kHz. Output boost may exceed full scale; intermediate Mix can create chorus coloration.

## Install

Requires Windows 10 or newer, x64, and an x64 VST3 host. Close your DAW, run **SleepwalkRadio-1.1.0-Windows-x64-Setup.exe**, approve the administrator prompt, then rescan plugins. Installs the full bundle at `C:\Program Files\Common Files\VST3\SleepwalkRadio.vst3`.

For manual installation, extract **SleepwalkRadio-1.1.0-Windows-x64-VST3.zip** and copy the complete bundle. The installer is unsigned. No development tools are needed. Standalone is excluded from downloads because device playback remains unverified.

## Assets and source

- Windows x64 Setup installer
- Portable VST3 ZIP
- Matching Source ZIP including pinned JUCE source and build scripts
- SHA256SUMS.txt

Licensed AGPL-3.0-only. See LICENSE and THIRD_PARTY_NOTICES.md. Distribute matching source alongside binaries.

## Validation and limitations

The Dream Transmission Release build passed all three automated suites (912,478 assertions); the complete locally installed VST3 bundle matched the build by SHA256. Tests cover DSP, presets, state, allocation, latency and offscreen editor rendering. See PROJECT_STATUS.md and docs/phase-reports/DreamTransmission-tests.txt.

Native DAW scan/playback, host automation/session recall, listening on real material, actual Windows DPI interaction, extended multi-instance sessions and clean-machine installer testing remain unverified. The previous installer workflow passed development-machine install/uninstall; that is not a fresh execution test of this release. This release does not certify any particular DAW or musical quality.
