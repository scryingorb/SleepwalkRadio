# SleepwalkRadio Development Plan

Version 1.4 | October 16 2026 | Status: Dream Transmission DSP development; local build/install only

This is the working reference for building SleepwalkRadio in small phases. It defines the agreed product, proposed implementation, and evidence needed before moving forward. Future Codex prompts should reference this file and the repository status rather than redesigning the plugin each time.

## Product direction

SleepwalkRadio is a simple lo-fi audio effect for dreamy, warped tape sound. It should work on melodies, vocals, drums, buses, and entire mixes. The interface looks like a believable vintage radio receiving a strange, dreamy broadcast. SketchCassette is an inspiration for the category; the sound, artwork, controls, and implementation should be original.

The core experience is warm saturation, gently wandering pitch, softened tone, and optional hiss. Low settings must remain useful on full mixes; higher settings should create obvious dreamy damage. It is an insert effect, not a synthesizer or a literal radio simulator.

## Decisions and assumptions

Confirmed: name SleepwalkRadio; dreamy warped tape character; simple panel; broad use across audio sources; Midnight Receiver appearance with Dream Broadcast accents.

Proposed build defaults: Windows x64 VST3, C++ with JUCE and CMake, and a standalone development target. These are implementation defaults, not verified toolchain requirements. Phase 1 must inspect the actual machine and repository, select compatible versions, and document setup and licensing requirements. Other operating systems and plugin formats are outside the first release.

The user's main DAW and version are not yet specified. Record them before host validation. Do not claim host compatibility from standalone tests alone.

## Sound controls

| Control | Intended behavior | Neutral setting |
|---|---|---|
| Warmth | Smooth tape-like saturation with sensible gain compensation | No saturation |
| Wobble | Slow, smooth, irregular pitch drift | No slow modulation |
| Flutter | Faster, gentle pitch instability | No fast modulation |
| Age | Progressively softened high end and subtle tonal wear | Neutral tone |
| Hiss | Independent shaped tape noise | Silent |
| Mix | Blend between original and processed audio | 0 percent is dry |
| Output | Small secondary trim, proposed range minus 12 to plus 12 dB | 0 dB |

Use normalized 0 to 100 percent values for the six main knobs. Internally map these to musically useful ranges through listening tests. Output uses dB. Provide a neutral initialization with character controls and hiss at zero, Mix at 100 percent, and Output at 0 dB. A separate gentle factory preset can demonstrate the effect.

Hiss must not rise automatically with Age. Pitch movement must stay smooth and coherent in stereo at normal settings. Preserve transients enough to keep drums useful. Avoid an abrupt metallic chorus sound or seasick modulation in the lower half of the controls.

## Audio architecture

Proposed path: input → Warmth → Wobble and Flutter variable delay → Age tone shaping → Hiss → dry and wet blend → Output. Confirm the order by listening during development; document any change.

Use interpolated fractional delay for pitch modulation with headroom for the full modulation range. Smooth parameter changes, modulation depth, and delay transitions. Keep left and right modulation linked for the first release. Decide and document how base delay, dry alignment, host latency reporting, and bypass interact before finalizing Mix. At neutral pitch settings, delayed wet audio must not produce unintended comb filtering when mixed with dry audio.

Use a linear dry and wet crossfade initially so identical paths do not gain level at the midpoint. At Mix 0, the wet effects and hiss must be absent; Output still applies. Neutral tone, zero noise and zero modulation should pass audio transparently apart from any documented compensated latency. Host bypass should restore dry audio without a burst or click, subject to the host's bypass mechanism.

Support matching mono and stereo input and output. Initial validation matrix: 44.1, 48, 88.2, and 96 kHz; block sizes 32, 64, 128, 256, 512, and 1024 samples, plus variable and zero-length block handling. Handle silence, reset, sample-rate changes, and repeated transport starts safely.

No locks, file access, logging, or heap allocation in the audio callback. Allocate buffers in preparation. Keep UI animation independent of audio processing. Guard against nonfinite output and denormal slowdowns. Any saturation oversampling must be justified by aliasing and CPU measurements and included in latency accounting.

## Radio interface

The explicit Phase 3 request supersedes the initial ivory/walnut/speaker proposal. The implemented Midnight Receiver is a dark brushed-metal panel with amber glass, a decorative tuning scale, output-level needle, restrained lavender trace, corner screws, and eight machined rotary controls. No preset browser is part of Phase 3.

Base size is 960 x 560 logical pixels, constrained proportionally between 768 x 448 and 1440 x 840. Artwork uses JUCE vector drawing with a cached static faceplate. Visual control order: Warmth, Age, Hiss / Wobble, Flutter / Mix, Output, Bypass. Parameter IDs, registration order, defaults and schema 1 remain unchanged.

All controls support attachments, editable values, fine adjustment, default reset, keyboard focus and automation gestures. The output meter uses an atomic peak snapshot; needle motion illustrates Wobble/Flutter amounts with UI oscillators, not exact DSP delay. The trace is stylized, not an oscilloscope. Motion Off stops the timer; idle/hidden editors poll at 4 Hz without animation, active animation is capped at 30 Hz, and closing destroys the timer. The UI preference is instance-local and not serialized.

## First release scope

Include the six main controls, Output, bypass, radio UI, state recall, a compact preset browser, and eight factory presets. Suggested preset names: Night Reception, Pillow Static, Faded Summer, Slow Orbit, Bedroom Broadcast, Worn Photograph, Ghost Frequency, and Last Train Home. Tune them across different sources; avoid extreme loudness changes between presets.

Defer dropouts, reverse, freeze, reverb, delay echoes, additional tape models, tempo sync, user modulation routing, multiband processing, and online accounts. These are possible later ideas and must not enter a phase without a scope decision.

## Build phases

### Phase 1 Foundation and loadable plugin

Create a clean build, stable plugin identity, VST3 and standalone targets, mono and stereo pass-through, all parameter IDs and ranges, versioned state serialization, and a plain functional editor. Record exact build instructions. Keep parameter IDs stable from this point forward.

Done when: a release build succeeds on the actual Windows machine; standalone audio passes; the VST3 scans and loads in the chosen DAW; all parameters automate and survive project save and reload. If DAW testing cannot be performed, leave that gate explicitly pending. No tape DSP or polished artwork yet.

### Phase 2 Tape engine (scope revised by October 16 implementation request)

Implement Warmth, Age, Output, Mix, Wobble, Flutter, independent shaped Hiss, stereo linking, and final dry alignment and latency behavior. The explicit Phase 2 request brings the original Phase 3 DSP work into this phase. Preserve identifiers and state compatibility. Establish repeatable audio fixtures and measure neutrality, gain behavior, silence, extremes, parameter ramps, modulation, tone, harmonics, aliasing and long-run stability, including 192 kHz.

Automated gate: neutral settings meet the documented latency-compensated transparency check; no demonstrated discontinuities or nonfinite samples occur; modulation, Mix, stereo behavior, loudness and aliasing are characterized. Human gate: musical quality, useful low settings, real-host behavior and listening on varied sources remain separately verified; numerical tests alone do not establish these.

### Phase 3 Midnight Receiver (explicit October 16 request)

Implement the final radio interface, eight custom rotary controls, subtle audio-responsive visualization, scaling, interaction refinement and host-readiness tests. Preserve the tape engine and stable state/identity. This absorbs the original Phase 4 visual work. Build Release VST3/standalone and run all regression, DSP and editor checks. Inspect actual editor renders; clearly distinguish automated pixel-scale renders from native Windows/DAW testing.

Implemented. Manual host, Windows display-scale interaction, device playback and subjective listening remain pending. See PROJECT_STATUS.md for exact evidence.

### Phase 4 Final polish, installer and release (explicit user request)

The user's final-phase request supersedes the previous suggested scope. Preserve functioning DSP/UI, verify the fresh Windows builds and automated suites, package an actual Inno Setup installer plus portable ZIP and matching AGPLv3 source, document limitations, and prepare Git/GitHub delivery. Do not add presets or effects. Stable v1.0.0 publication requires native DAW, listening and installer gates; otherwise stop at a clearly labelled release candidate. Current candidate is 1.0.0-rc.1 (numeric host version 1.0.0).

### Phase 5 Presets and production validation

Tune eight factory presets. Complete preset browsing and state handling, including malformed and older state handling. Validate automation, rendering, reopening sessions, multiple instances, sample-rate changes, mono, stereo, and host bypass. Profile CPU with a repeatable session on the user's machine and record the hardware and results.

Done when: the chosen DAW passes the checklist; preset and session recall match; offline rendering is safe; no confirmed release blockers remain. Do not promise bit-identical random modulation or hiss renders unless explicitly implemented and tested.

### Phase 6 Package and release

Freeze features. Fix demonstrated defects only. Prepare the Windows VST3 package, installation and uninstall instructions, version information, changelog, license notices, screenshots, and a short getting-started guide. Validate the package on a clean Windows setup without development tools if possible.

Done when: the exact packaged build is tested, release limitations are recorded, and artifacts match the tagged source. Testing on a second machine remains pending unless performed. Commit, push, and publish a GitHub release when the user authorizes those actions in the release prompt; do not imply this planning document publishes anything.

## Phase tracking and handoff

Place this file in the repository as docs/SleepwalkRadio_Plan.md. Keep PROJECT_STATUS.md at the root with: current phase, completed work, exact build and test commands, passed checks, pending manual checks, known defects, next task, and relevant commit. Add concise phase reports under docs/phase-reports/ only when they provide useful evidence.

At each phase end, Codex must stop at the defined boundary and give a short handoff. The user can bring that handoff back here for the next prompt. Future prompts should inspect the status and relevant code first, address unresolved blockers, and implement only the next approved phase. Update this plan when an agreed decision changes; preserve a short dated decision log.

Avoid broad repository scans, unrelated refactors, repeated full test runs, and feature expansion. Run focused tests during changes and the relevant phase checks before handing off. Report actual evidence separately from assumptions or pending work.

## Initial Codex prompt

```text
We are starting SleepwalkRadio, a dreamy lo-fi tape effect with a vintage radio UI.

Read docs/SleepwalkRadio_Plan.md and any applicable AGENTS.md. If the plan is not present, ask me to add the supplied planning file before implementing. Inspect the existing repository and toolchain without overwriting unrelated work.

Implement Phase 1 only. Proposed target: Windows x64 VST3 plus a standalone development target, using C++ with JUCE and CMake. Verify compatible tool versions and document dependencies and licensing requirements. Preserve an existing suitable project setup if one exists.

Create matching mono and stereo pass-through processing; stable parameters for Warmth, Wobble, Flutter, Age, Hiss, Mix, Output, and plugin bypass; versioned state save and restore; and a plain functional editor. Use normalized main controls and the neutral defaults described in the plan. Do not implement tape processing or final radio artwork yet. Keep the audio callback free of allocation, locks, file access, and logging.

Add reproducible build instructions and meaningful checks for parameter state recall and pass-through. Build what this environment supports. Verify VST3 scan and load in my DAW if available; otherwise mark it pending and give me concise manual instructions. Never report Windows or DAW checks as passed unless actually run.

Create or update PROJECT_STATUS.md with completed work, exact commands and results, pending checks, known blockers, and the next phase. Stop at the Phase 1 boundary. Summarize the result, what I need to test, and any decisions needed before Phase 2. Do not publish a release.
```

## Current status

The user's Dream Transmission request supersedes the earlier packaging/release task. Improve the existing DSP and add twelve factory voices while retaining eight controls and the Midnight Receiver design. Age combines tone, blur and dropout wear; Wobble combines pitch and gentle stereo amplitude drift. Do not introduce three extra knobs. Preserve parameter IDs/defaults and read old schema-1 states; document intentional sonic changes and the increased fixed delay. Build/test and local installation are complete. The user subsequently authorized a source commit and push. No new installer, package, tag or GitHub release is included.

## Decision log

- October 16 2026: user reported weak/unsatisfactory sound and requested Dream Transmission DSP improvements plus twelve presets. User clarified to keep the panel simple. Revised mappings, stereo-linked saturation compensation, richer bounded motion, Age wear/upper-band diffusion/dropouts and colored hiss; preserved eight IDs. Fixed transport base is now 24 ms plus FIR latency. Optional presetIndex metadata remains in schema 1. Old states recall values but intentionally sound different. Local installation only.

- October 16 2026: explicit Phase 4 request authorized final packaging, source commit/push and gated publication. User chose AGPLv3. Implemented candidate installer/ZIP/source workflow. Native access denial and pending manual gates prevent stable publication; no stable tag is created. Repository creation requires separate approval after automatic review rejection.

- October 16 2026: explicit Phase 3 request authorized Midnight Receiver visual work and host-readiness validation, superseding the prior recommendation and absorbing the original Phase 4 UI. Implemented charcoal/amber vector artwork, eight rotary controls, optional animation and editor regression checks; preserved tape DSP, parameter IDs/defaults and schema 1. Phase 4 is suggested host/listening validation only and has not started.

- October 16 2026: the user explicitly expanded Phase 2 to the full tape engine, absorbing the original Phase 3 motion/hiss implementation. Implemented 2x FIR oversampling, bounded shared stereo transport, a fixed 6 ms transport base plus 49 samples saturation-filter delay, aligned dry/Mix/bypass, independent seeded hiss, tone shaping and smoothed controls. Kept default values and schema 1 unchanged. Recommended next phase is listening refinement and host validation, subject to user approval; not started.

- October 16 2026: corrected the product name consistently to SleepwalkRadio per the implementation request. Established C++17, JUCE 8.0.12 pinned to commit 29396c22c93392d6738e021b83196283d6e4d850, CMake 3.31.6, and the installed Visual Studio 2019 x64 toolchain. Phase 1 implementation and automated verification are complete; manual host/playback gates remain pending. Stable identifiers and state policy are recorded in PROJECT_STATUS.md and README.md.

- October 16 2026: confirmed dreamy warped tape, simple controls, broad source use, SleepwalkRadio name, and vintage radio design with a surreal illuminated signal.
- October 16 2026: proposed Windows VST3 with JUCE and CMake, six phases, and repository status files as the implementation workflow. Toolchain and host details remain to be verified.

- October 16 2026: after the Dream Transmission source push, user requested release with installer. Prepare and publish 1.1.0-rc.1 as a prerelease with matching source and checksums; manual stable-release gates remain unverified.

- October 16 2026: user explicitly corrected the release designation: publish v1.1.0 as a regular release, not a prerelease. This supersedes the earlier prerelease-only publication gate; disclose incomplete manual checks.
