# Third-party notices

SleepwalkRadio project source and original artwork: Copyright (c) 2026 scryingorb.
Licensed under GNU Affero General Public License version 3 only (AGPL-3.0-only).
The complete license is in LICENSE. No warranty is provided.

JUCE 8.0.12, commit 29396c22c93392d6738e021b83196283d6e4d850, is used under its
AGPLv3 option. Its original license notice and bundled dependency notices are
copied into licenses/JUCE with their relative paths preserved. The matching
source archive includes the full pinned JUCE source and its embedded notices.

The Windows build uses JUCE's bundled VST3 SDK (MIT), HarfBuzz (Old MIT),
SheenBidi (Apache-2.0), libpng/zlib (zlib), JPEG library (IJG), and audio codec
sources including FLAC and Ogg/Vorbis (BSD-style). See the corresponding
notices and source headers for copyright holders and complete terms.
Uncompiled platform/format dependencies retain their own notices in the source
archive; inclusion of their notice does not imply they are enabled. ASIO is
disabled in this build. No external photographs or font files are embedded.

The installer is built with Inno Setup; its license is in licenses/InnoSetup.txt.
The Microsoft C++ runtime is statically linked by the installed MSVC toolchain.

Distributors must provide the corresponding source alongside these packages,
including the pinned dependency source and build/packaging instructions.
