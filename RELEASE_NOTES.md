# SleepwalkRadio 1.0.0-rc.1 — Midnight Receiver

**Release candidate. Native host and listening validation is incomplete.**

SleepwalkRadio combines warm saturation, slow tape wander, delicate flutter,
softened highs and independent hiss in a dark-metal receiver interface with
amber tuning glass. Eight accessible controls, optional display motion and
proportional resizing keep it straightforward to use.

Windows 10 or newer, x64, and a VST3-compatible x64 DAW are required. No specific
DAW compatibility is certified. Processing supports matching mono/stereo buses;
latency is fixed at 337 samples (7.02 ms) at 48 kHz. Output boost may exceed full
scale. Loudness-match comparisons; intermediate Mix with modulation can produce
intentional chorus coloration. This is not a zero-latency monitoring effect.

## Packages

- SleepwalkRadio-1.0.0-rc.1-Windows-x64-Setup.exe
- SleepwalkRadio-1.0.0-rc.1-Windows-x64-VST3.zip
- SleepwalkRadio-1.0.0-rc.1-Source.zip (matching project and JUCE source)
- SHA256SUMS.txt

Close your DAW, run Setup, then rescan VST3 plugins. The standard destination is
`C:\Program Files\Common Files\VST3\SleepwalkRadio.vst3`. For portable installation,
copy the entire bundle from the ZIP. See INSTALL.txt. These are unsigned builds.
Standalone is built for development but excluded from release packages because
device playback is unverified.

## Release gates

Automated build/test/package evidence is recorded in PROJECT_STATUS.md and the
Phase4 reports. Automated success does not prove musical quality or hardware
playback reliability. Before a stable release, complete and record:

- [ ] REAPER/selected DAW scan, load, playback and all eight controls.
- [ ] Automation, plugin/host bypass, latency compensation, save/reopen recall.
- [ ] Repeated editor lifecycle and multi-instance CPU/audio continuity.
- [ ] Gentle/medium/extreme settings on vocals, drums, synths and full mixes.
- [ ] Native Windows 100/125/150/200% scaling, mouse/fine/reset/keyboard behavior.
- [ ] Standard-path elevated installer install/upgrade/uninstall on clean Windows.

AGPL-3.0-only; see LICENSE and THIRD_PARTY_NOTICES.md. Distribute matching source
alongside binaries. Do not publish a stable v1.0.0 tag until mandatory gates pass.
