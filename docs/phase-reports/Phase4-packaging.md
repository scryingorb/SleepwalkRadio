# Phase 4 packaging verification

October 16, 2026. Candidate 1.0.0-rc.1; numeric binary version 1.0.0.

- Inno Setup 6 compiled the actual installer successfully. Unsigned PE, not a renamed archive.
- Installer SHA256: d0a1c8c79932197c6ca167869b6b3e49a714ca342994f1586e1561788a911ae2.
- Portable ZIP and full-source ZIP passed CRC/integrity checks. Final hashes are in dist/SHA256SUMS.txt.
- VST3 build, portable ZIP and installed binary SHA256 match: 6e7ccda9966ca0c65935b77bae226676821281e408579e41bbb0e85b20440cef.
- Verified no prior SleepwalkRadio bundle existed at the standard system destination before testing.
- Executed the packaged installer with /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /LOG. User approved the Windows administrator prompt. Exit code 0.
- Verified installed Contents/x86_64-win/SleepwalkRadio.vst3 at the standard Common Files/VST3 path, and ProgramData/SleepwalkRadio/Uninstall/unins000.exe.
- Executed that exact uninstaller with the same silent/no-restart flags. Exit code 0. Plugin bundle and uninstaller no longer exist. No wildcard filesystem deletion was used.
- Install/uninstall logs retained locally in build/installer-install.log and build/installer-uninstall.log; they contain machine paths and are intentionally not published.
- Inspected the fresh VST3 module metadata: version 1.0.0, stable component/controller CIDs. Manifest helper loaded the module successfully.
- dumpbin /dependents reported only Windows system DLLs; no MSVCP or VCRUNTIME dependencies. Static C++ runtime is configured in CMake.

This establishes successful install/uninstall on the development machine only. Clean Windows, upgrades, locked-plugin handling and installer wizard interaction remain UNVERIFIED. Native host/listening gates remain pending. Do not promote to stable solely on this evidence.
