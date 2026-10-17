# Changelog

## 1.1.0 - Dream Transmission

- Revised Warmth, Wobble, Flutter, Age and Hiss for stronger tape character.
- Integrated upper-band blur/dropouts into Age and gentle stereo drift into Wobble.
- Added twelve factory presets plus Init while preserving eight controls.
- Retained state/parameter identity; old settings intentionally sound different.
- Increased fixed latency to 1201 samples at 48 kHz.
- Updated installer, portable/source packages and release evidence. Native validation remains incomplete; published as a regular release at the user's direction.

## 1.0.0-rc.1 — Midnight Receiver

Release candidate for the first Windows x64 VST3 release. Not a stable release.

- Retains the completed tape engine and scalable Midnight Receiver interface.
- Preserves all eight parameter IDs/defaults, schema-1 state and plugin IDs.
- Adds an Inno Setup installer, portable VST3 ZIP, matching source archive,
  checksums, AGPLv3 licensing and installation/troubleshooting documentation.
- Statically links the Microsoft C++ runtime for deployment without build tools.
- Numeric VST3/Windows product version is 1.0.0; distribution version is
  1.0.0-rc.1 because mandatory native validation remains incomplete.

Manual DAW/listening, native display scaling, standalone playback and
clean-machine installer validation remain UNVERIFIED. No stable v1.0.0 tag.

## 0.3.0

Midnight Receiver vector interface, eight custom knobs, optional visualization,
scaling and editor integration checks.

## 0.2.0

Complete tape engine, aligned Mix/bypass, fixed latency, allocation and DSP tests.

## 0.1.0

Windows VST3/standalone foundation, stable parameters and versioned state.
