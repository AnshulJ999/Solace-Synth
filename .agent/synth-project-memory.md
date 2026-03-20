# Solace Synth — Project Memory

**Created:** 2026-03-08
**Last Updated:** 2026-03-21 (Phase 7.5: standalone binary packaging + final SVG assets + LFO discrepancy audit)
**Status:** Active — All DSP phases 6.1-6.9 + 6.8b + pitch bend/mod wheel COMPLETE. Phase 7 UI 7.1-7.5 (including dropdowns, faders, and icons) COMPLETE. **Standalone Packaging (Binary Data Embedding) is now COMPLETE and portable.** 
**Next Focus:** Preset system (GUI + file management) and final UI/UX polishing.
**Pending Decisions:** LFO target list reconciliation (see discrepancies section).


BUILD WAS VERIFIED as of 2026-03-11

---

---

**Pending item:** LFO target list in `main.js` does NOT match Vision Document (Nabeel's answer 5). Discrepancy found: Code has split Osc Pitch and missing Distortion/Mix/AmpAttack targets. Verification needed to finalize V1 list.

**Standalone Packaging:** COMPLETED (2026-03-21) — `juce_add_binary_data()` implemented in CMake and used in `PluginEditor.cpp`. The EXE is now portable and can be shared without the local repo path.

**SVG icons:** COMPLETED — Final SVG assets (logo, waveforms, UI buttons) have been exported/added and integrated into the `UI/assets/icons/` folder.

**Master Distortion:** REFINED — Fixed the "volume jump" bug at low drive settings by updating the mapping formula to be mathematically transparent at `drive=0`. Output is now smooth and "organic" across the entire range.

**Next Major Step:** **Preset System.** While JUCE handles state persistence (DAW save/load), we need a custom GUI and file management system (Preset Browser) to allow users to save/load `.solace` patches.

**Ctrl+drag fine fader control:** PENDING — deferred from Phase 7.5. Marked for a later pass.

**UI plan:** `.agent/plans/Phase-7-UI-Master-Plan.md` — this is the authoritative merged plan. `.agent/plans/Standalone-Packaging-Plan.md` covers the portable standalone handoff path. Old files (`Phase 7 — UI Roadmap.md`, `Phase-7.4-Pixel-Perfect-Audit.md`) are kept as references only.

---

## 🎯 Project Overview

A free, open-source polyphonic soft synthesizer. Being built by Anshul (backend/DSP/code) and a friend (UI/UX designer, Figma). Goals:

- At minimum: a portfolio project
- At best: a fully functional, open-source, publishable synth — VST3 + Standalone app
- Target platforms: Windows (primary), cross-platform (Mac/Linux desirable later)
- Target formats: VST3 + Standalone (via JUCE's single-codebase multi-format output)
- Intended to be freely available / open-source

---

## 🏷️ Project Name — CONFIRMED

**Solace Synth** — confirmed name as of March 2026.
- GitHub repo: **`AnshulJ999/Solace-Synth`** (created, owned by Anshul)
- Local path: `G:\GitHub\Solace-Synth`
- Project memory: `.agent/synth-project-memory.md` (hardlinked to `G:\GitHub\Personal-Stuff\Synth-Project\synth-project-memory.md`)
- Repo status: **Phases 0-7.5 complete** — working polyphonic synth with bridge, MIDI keyboard, and portable embedded-UI standalone build.
- The "SS" monogram logo is now finalized and integrated.

---

## 👥 Team Roles

| Person | Role |
|---|---|
| **Anshul** | Owns the repo. Backend/DSP: C++ audio engine, voice management, DSP, parameter bridge |
| **Friend (designer)** | UI/UX design in Figma. If WebView UI selected: also owns HTML/CSS/JS frontend |

---

## 📋 Vision Document (v0.1 — March 2026)

### Core Requirements
- Simple to use polyphonic synthesizer, possibly multi-platform
- At least two oscillators with basic tuning and octave shifts
- Amplifier envelope (ADSR)
- Filter types: LP 12dB and LP 24dB; possibly HP too
- Filter envelope (ADSR)
- LFO with assignable modulation targets
- Unison
- Velocity sensitivity and modulation
- Computer-friendly UI

### Nice-To-Haves
- Voice count config (to limit CPU — especially for mobile/lower-end devices)
- Portamento / Glide
- Preset library with factory presets

### UI Notes
- Knob-based UI was tried first — **rejected** (users found dragging knobs unintuitive)
- **Slider-based UI (second iteration) was accepted** — more viable, moving forward with sliders
- Designer working in **Figma** and has produced mockups
- Color scheme: Light/white background with orange accent sliders — clean, minimal, modern
- Logo: "SS" monogram (working title branding)

### Platforms Considered
- JUCE (primary framework choice — confirmed)
- Audio Plugin Coder (APC) — evaluated, verdict: useful for scaffolding only, not DSP (see APC section)

---

## 🎨 UI Design (Current Mockup — Friend's Figma Design)

| Section | Parameters |
|---|---|
| Oscillator 1 | Waveform selector (arrows), Octave, Transpose, Tuning slider |
| Oscillator 2 | Waveform selector (arrows), Octave, Transpose, Tuning slider |
| Osc Mix | Osc 1 level slider, Osc 2 level slider |
| Amplifier Envelope | Attack, Decay, Sustain, Release sliders |
| Master | Level slider |
| Filter | Cutoff slider, Resonance slider |
| Filter Configuration | Filter Type (LP 24dB, etc. — arrow selector), Filter ADSR (Attack, Decay, Sustain, Release) |
| LFO | Waveform selector, Rate slider, Amount slider, 3 assignable target dropdowns |
| Voicing | No. of Voices (arrow selector, default 16), Unison (arrow selector, default 3), Velocity Range slider, 2 Velocity Mod Target dropdowns |

---

## 🗺️ Proposed V1 / V2 Scope

### V1 — MVP / Portfolio-Ready
```
├── 2 Oscillators (Sine, Saw, Square, Triangle + Noise)
├── Osc Mix (crossfade / level sliders)
├── Amplifier ADSR + Master Level
├── Filter: LP12 / LP24 / HP12 (State Variable Filter)
├── Filter ADSR
├── 1 LFO (Sine, Saw, Square, Triangle)
│   └── 3 assignable targets (from a defined fixed list)
├── Polyphony: 1–16 voices (configurable)
├── Unison: 1–4 voices + basic detune
├── Velocity → Amp Level (standard)
├── Velocity → 1 other assignable target
└── Standalone + VST3 on Windows
```

### V2 — Nice-To-Haves (post V1 stable)
```
├── More filter types (BP, Notch)
├── Portamento / Glide
├── Preset system (save/load + factory bank)
├── MIDI CC learn / mapping
├── Cross-platform (Mac AU, Linux LV2)
└── More LFO waveforms and targets
```

### Open Spec Gaps (need decisions before implementation)
- **Waveforms:** Not yet explicitly listed — standard set: Sine, Triangle, Sawtooth, Square, Pulse, Noise
- **Filter HP:** Vision doc says "maybe" — suggest including LP12/LP24/HP12 in V1 (trivial with State Variable Filter)
- **LFO target list:** **RECONCILIATION PENDING.** Currently 8 targets (0-7): None, FilterCutoff, Osc1Pitch, Osc2Pitch, Osc1Level, Osc2Level, AmpLevel, FilterRes. (Discrepancy found vs Vision Doc, pending final Nabeel review).
- **Velocity mod target list:** **RESOLVED/COMPLETE.** 8 targets (0-7): None, AmpLevel, AmpAttack, FilterCutoff, FilterResonance, Distortion, OscPitch, OscMix.
- **"Computer-friendly UI":** Does this imply an onscreen keyboard? Needs clarification
- **Portamento complexity:** In polyphonic mode this is non-trivial — should be V2, not V1

---

## 🖼️ Figma → Plugin UI Workflow

This is a **critical design decision** because it determines whether the friend can contribute code directly.

### The Core Problem
There is **no automated Figma → JUCE plugin pipeline.** Figma-to-React/HTML tools don't speak C++. The gap has to be bridged manually, but the *size* of that gap depends on the UI framework chosen.

### Three Options Evaluated

**Option A: Native JUCE Components**
- Developer manually translates Figma into C++ `LookAndFeel` classes
- Figma is a static reference only — friend cannot write UI code
- Iteration: friend updates Figma → developer manually re-implements → slow cycle
- Best performance, industry standard, perfect DAW compatibility
- **Con: Friend is locked out of contributing to UI code**

**Option B: WebView (HTML/CSS/JS embedded in JUCE 8)**
- JUCE 8 has native WebView2 (Windows) / WKWebView (Mac) support
- DSP stays in C++; UI is a full HTML/CSS/JS web frontend embedded in the plugin window
- Figma Dev Mode generates CSS values (spacing, colors, fonts) friend can paste directly
- **Friend can own and edit the HTML/CSS frontend directly — no JUCE knowledge needed**
- Hot-reload style iteration: open HTML in browser for instant design preview
- Parameter data passed via JUCE's C++↔JS relay (one-time bridge code you write)
- Slightly heavier (embedded Chromium), some very old DAWs have WebView issues
- **Used by APC templates, increasingly used by modern commercial plugins**

**Option C: SVG-Native JUCE**
- Export every UI element from Figma as SVG, render via `juce::Drawable`
- Pixel-perfect match to Figma design
- Still C++ for all interaction — friend can't contribute code
- Good middle ground for fidelity without WebView overhead

### ✅ Decision: WebView (Option B) — CONFIRMED

| Criterion | Native JUCE | WebView | SVG-Native |
|---|---|---|---|
| Friend can write UI code | ❌ | ✅ YES | ❌ |
| Figma → Code fidelity | Medium | High | Highest |
| Design iteration speed | Fast | Fast | Slow |
| Performance | Best | Good | Best |
| DAW compatibility | Perfect | Very good | Perfect |

**Rationale:** Given the team split (Anshul = C++ DSP, friend = design/frontend), WebView enables genuine collaboration. Friend designs in Figma → exports CSS values → writes/edits HTML/CSS files in the repo. Anshul writes DSP + the C++↔JS bridge once. Clean separation of concerns.

**The iteration loop with WebView:**
```
Friend changes Figma → Updates HTML/CSS in repo → Preview in browser → 
Anshul rebuilds plugin → Full test in DAW → Feedback to friend → Repeat
```

**Note on APC's WebView templates:** APC has production-quality JUCE 8 WebView integration code (battle-tested member-ordering, relay pattern, resource provider, binary data embedding). Even if we don't use APC as a workflow tool, its WebView template code is worth referencing directly.

### Figma's Role Going Forward
- **Figma = source of truth for design intent** — always maintained by friend, never becomes obsolete
- **Figma Dev Mode** (inspect panel) gives exact CSS values, spacing, colors — direct bridge to HTML/CSS
- Live sync is not possible (no tool does Figma → live running plugin), but with WebView the handoff is so direct it barely matters
- Friend can iterate on Figma and then update the HTML/CSS frontend himself — minimal developer bottleneck

---

## Bugs, Patterns & Pre-Release Checklist

*Full-codebase review -- Claude Code + Gemini, 2026-03-10*

### Fixed

| # | File | Issue | Fix Applied |
|---|------|-------|-------------|
| 1 | `SolaceVoice.h` | `filter.reset()` missing in `startNote()` -- stale LadderFilter delay state bleeds on voice reuse at high resonance | `filter.reset()` added before `filter.setMode()` |
| 2 | `PluginEditor.cpp:57` | `callAfterDelay([this])` -- dangling pointer if editor closes within 500ms | `SafePointer<SolaceSynthEditor>` with null check |
| 3 | `PluginEditor.cpp:312` | Comment claimed "no heap allocation" when `new DynamicObject()` IS heap | Comment corrected |
| 4 | `PluginEditor.cpp:170` | Path traversal: `getChildFile(path)` without boundary check | `file.isAChildOf(uiDir)` guard added |

### Tracked -- V2 / Pre-Release

| # | File | Issue | Plan |
|---|------|-------|------|
| 5 | `SolaceFilter.h` | Cutoff hard-clamped to 20,000 Hz -- safe at 44.1kHz+ but unstable if sample rate < 40kHz | V2: `min(20000.0f, sampleRate * 0.45f)` dynamic clamp in `prepare()` |
| 6 | `CMakeLists.txt` | `PLUGIN_CODE "Ss01"` -- collision risk in large-studio DAW scans | Pre-release: UUID-derived unique 4-char code |
| 7 | `SolaceLogger.h` | Multi-instance: concurrent log file writes | Pre-release: instance-stamped filenames |
| 8 | `PluginProcessor.cpp` | `masterVolume` via `buffer.applyGain()` once per block -- zippers under rapid DAW automation | Phase 6.9: `juce::LinearSmoothedValue<float>` per-sample gain |

### Established Patterns -- Never Break

**Audio thread:**
- Params only via `std::atomic<float>*` from `getRawParameterValue()` -- no APVTS lookups on audio thread
- Block-level reads outside the sample loop via `.load()`; only `getNextSample()` calls inside the loop
- `processBlock()` always opens with `juce::ScopedNoDenormals`

**Cross-thread lambdas:**
- Audio-to-message thread: `callAsync()` + `SafePointer`
- Deferred callbacks (callAfterDelay etc.): `SafePointer`, never raw `[this]` (Issue 2 was exactly this)

**Voice lifecycle reset rules:**
- `ampEnvelope` -- NO reset in `startNote()` -- JUCE ADSR ramps from current level on steal (prevents click on stolen note)
- `filterEnvelope` -- ALWAYS `reset()` before `trigger()` -- pluck sweep must start from zero even on stolen voice
- `filter` -- ALWAYS `reset()` before `setMode()` -- LadderFilter delay state persists after natural release; self-oscillation bleeds at high resonance if not cleared
- `stopNote(allowTailOff=false)` -- ALL THREE: `ampEnvelope.reset()` + `filterEnvelope.reset()` + `filter.reset()` + `clearCurrentNote()`

**LFO (Phase 6.6):**
- Do NOT call `lfo.reset()` in `startNote()` -- each voice has a random initial phase from construction; resetting to 0 would produce key-synced behaviour
- Per-voice LFO phase diversity guaranteed by seeding `currentAngle` randomly in `SolaceLFO`'s constructor (via `getSystemRandom()`)
- "Free-running" means "random initial phase + never reset at note-on" -- LFO does NOT advance while the voice is idle (only during active rendering), but the perceptual goal is fully achieved
- `juce::Random` re-seeded per-instance in constructor: default `juce::Random()` uses seed=1 for all instances; without re-seeding, all S&H sequences would be identical across voices

**kVoiceGain:**
- Now: `0.15f` static (V1 acceptable with mixed velocities)
- Phase 6.7: replace with `1.0f / sqrt(voiceCount * unisonCount)` dynamic normalisation

### Confirmed-Clean Architecture

| Decision | Rationale |
|----------|-----------|
| Filter before amp envelope in signal chain | Self-resonance decays naturally through amp release phase |
| Both oscs always call `getNextSample()` regardless of `oscMix` | Phase-continuous crossfader -- swept mix never reveals a phase jump |
| `getTailLengthSeconds()` returns `ampRelease` only | Voice is silent at amp zero; filter tail is inaudible thereafter |
| `LadderFilter` via 1-sample `AudioBlock` on stack | Works around protected API in JUCE 8; zero heap; drives `updateSmoothers()` per sample |
| `oscMix` read per-block | One atomic load; live knob response; V2 candidate for SmoothedValue under heavy automation |
| `SafePointer` for all escaped lambdas | Component may be destroyed before a deferred callback fires |
| APVTS listeners removed in destructor before members destruct | Prevents in-flight parameterChanged() on partially-destroyed object |
| `visibilityChanged()` calls `sendAllParametersToJS()` | Compensates for events dropped by emitEventIfBrowserIsVisible() while editor hidden |

---

## 🛠️ Technical Approach

### Framework Decision
**JUCE 8** — confirmed. Reasons:
- Industry standard for VST/AU/standalone from one codebase
- Built-in MIDI, polyphony voice management, filter/DSP classes, GUI + WebView
- Huge community, best documentation

### JUCE Licensing — CONFIRMED
**Using JUCE Starter (Free Commercial License)**, NOT AGPLv3.
- JUCE Starter is free for individuals/companies under $50K annual revenue
- This means the Solace Synth project can use ANY license we choose — MIT, GPL, proprietary, or undecided
- We do NOT have to use AGPLv3 for our project
- If the project ever exceeds $50K revenue, a paid JUCE license would be needed
- **Project license: DEFERRED** — will decide when going public. Repo is currently private.

### Build Approach
**Build from scratch** with references. Not forking an existing synth.
- No suitable MIT-licensed synth that matches the full feature set exists
- Starting fresh gives clean ownership and licensing
- AI assistance (Antigravity, Claude Code) makes this very feasible
- JUCE's built-in DSP primitives handle the hard maths — not truly from zero

### Build System
CMake (not Projucer — CMake is the modern JUCE 7+ standard)

### JUCE Inclusion Method — CONFIRMED
**CMake FetchContent** (not git submodule). Reasons:
- Simpler — 4 lines in CMakeLists.txt, no submodule commands to learn
- Keeps repo small (JUCE ~100MB not committed)
- Downside: needs internet on first build — not an issue for this team
- Switching to submodules later is trivial if needed

### CMakeLists.txt Strategy
**Based on pamplejuce patterns, not a clone.** We write our own CMakeLists.txt informed by pamplejuce's patterns — correct JUCE module list, compiler flags, `juce_add_plugin()` call structure — but configured specifically for Solace Synth (our plugin name, FetchContent, our folder structure).

### Dev Environment
- Windows 11
- Visual Studio 2026 with "Desktop development with C++" workload — installed and verified
- CMake 4.2.3 (latest stable) — installed and verified
- Git (already installed)
- WebView2 NuGet package 1.0.1901.177 — installed via PowerShell (required for JUCE WebView2 support)
- **Status: Fully operational**

### Key References / Learning Resources
- **[synth-plugin-book](https://github.com/hollance/synth-plugin-book)** — MIT, 206⭐ — companion code for "Code Your Own Synth Plug-Ins With C++ and JUCE" — closest match to our synth type
- **[pamplejuce](https://github.com/sudara/pamplejuce)** — MIT, 649⭐ — gold-standard JUCE CMake template (CI/CD on GitHub Actions, pluginval integration)
- **[Helm (mtytel)](https://github.com/mtytel/helm)** — GPL-3.0, 2474⭐ — architecture reference only; abandoned 7 years ago on JUCE 4/5 era, not to fork
- **[Odin2](https://github.com/TheWaveWarden/odin2)** — Custom license, 743⭐ — modern JUCE codebase to study for UI component architecture
- **[Wavetable (FigBug)](https://github.com/FigBug/Wavetable)** — BSD-3-Clause, 191⭐ — permissive license, actively maintained, good general reference
- **[awesome-juce](https://github.com/sudara/awesome-juce)** — master list of all JUCE plugins, modules, templates

### UI Research Notes
- **Neural DSP (reference for premium UI):** Uses JUCE + custom OpenGL GPU rendering + a large team of professional C++ graphics engineers. NOT WebView. Their premium feel comes from GPU-accelerated rendering — a Tier 1 approach requiring expert C++ graphics devs.
- **For Solace Synth:** WebView (Tier 3) gives access to the same visual design language (shadows, rounded cards, smooth color palettes, modern typography) without needing OpenGL expertise. The result looks premium — it just won't have Neural DSP's silky 120fps knob-spinning animations, which is irrelevant for a slider-based UI anyway.

---

## 🤖 Audio Plugin Coder (APC) — Assessment

**Repo:** [github.com/Noizefield/audio-plugin-coder](https://github.com/Noizefield/audio-plugin-coder)
**Stars:** ~210 (March 2026) | **License:** MIT | **Created:** Jan 30, 2026

### What It Is
An AI-first "vibe-coding" framework for building JUCE plugins. Provides structured workflow (Dream → Plan → Design → Implement → Ship), pre-built domain skills, and state tracking via `status.json`. Agent-agnostic — works with Antigravity, Claude Code, Codex, Kilo, Cursor.

### Deep-Dive Assessment

**What APC is GOOD at (7/10 for general JUCE plugins):**
- Project scaffolding + 5-phase workflow organization
- Production-quality WebView2/JUCE integration templates (relay pattern, binary data, resource provider)
- Build automation (CMake + PowerShell scripts, GitHub Actions CI)
- Self-improving troubleshooting (12 documented JUCE issues in YAML, auto-capture after 3 failures)
- Packaging/shipping (Inno Setup Win, DMG/PKG Mac, AppImage Linux)

**What APC is BAD at for our synth (4/10 for subtractive poly synth):**
- ZERO oscillator code (no wavetable, no virtual analog DSP)
- ZERO polyphonic voice management
- ZERO envelope generators
- ZERO LFO patterns
- ZERO unison/detune
- ZERO modulation routing
- ZERO MIDI handling patterns
- Basic DSP knowledge only (gain, basic IIR filter, compressor, SmoothedValue)
- Showcase plugin (CloudWash) is a granular effects processor — completely different domain

**Verdict: Cherry-pick, don't adopt wholesale.**
- ✅ Use its WebView templates as reference for the JUCE 8 WebView bridge
- ✅ Organizational model (Dream/Plan/Design/Implement/Ship phases) is good
- ✅ Troubleshooting database worth reading
- ❌ Don't use for DSP, voices, envelopes, LFO, modulation — use JUCE docs + synth-plugin-book + AI

### Caveats
- Created Jan 30, 2026 — only ~6 weeks old with no stable release
- ~37 commits, then quiet since Feb 15 — single developer, burst pattern
- Tested Windows 11 + Linux; macOS not yet tested

---

## 📋 License Summary

| License | Examples | Implication |
|---|---|---|
| MIT | pamplejuce, synth-plugin-book, APC | Fully free, use however we want |
| BSD-3-Clause | Wavetable (FigBug) | Same as MIT essentially |
| GPL-3.0 | Helm, Surge XT, Vital, OB-Xd | Fine if our synth is also open-source |
| AGPLv3 | JUCE 8 (open-source tier) | Fine for open-source projects |

**Decision:** Using JUCE Starter (free commercial license) — project can use any license. License choice deferred until repo goes public.

---

## 🏗️ Build System Details

- **Dev Environment:** Windows 11, Visual Studio 2026 (MSVC 19.50), CMake 4.2.3, Git
- **CMake Generator:** Visual Studio 18 2026 (auto-detected)
- **JUCE Version:** 8.0.4 (via FetchContent)
- **Build Outputs:**
  - VST3: `build/SolaceSynth_artefacts/Release/VST3/Solace Synth.vst3`
  - Standalone: `build/SolaceSynth_artefacts/Release/Standalone/Solace Synth.exe`
- **Build Commands:**
  ```
  cmake -B build
  cmake --build build --config Release
  ```
- **Log files (multi-level):** `%TEMP%\SolaceSynth\`
  - `info.log` — INFO + WARN + ERROR (clean lifecycle view)
  - `debug.log` — DEBUG + INFO + WARN + ERROR (bridge calls)
  - `trace.log` — everything from C++ side only (JS TRACE stays in UI panel, not forwarded)
- **JS debug panel:** Visible in UI, shows all levels with color coding
  - Grey=TRACE, Blue=DEBUG, Green=INFO, Orange=WARN, Red=ERROR
  - JS TRACE/DEBUG: panel only. JS INFO+: forwarded to C++ files via bridge.
- **Audio thread safety:** No logging or I/O on audio thread. `parameterChanged()` bounces everything to message thread via `callAsync`.
- **Logger class:** `Source/SolaceLogger.h` — custom `juce::Logger` subclass with 3 `FileLogger` instances
- **Usage:** `SolaceLog::trace()`, `SolaceLog::debug()`, `SolaceLog::info()`, `SolaceLog::warn()`, `SolaceLog::error()`
- **Pre-release TODO:** Wrap all `SolaceLog::` calls in `#ifdef SOLACE_LOGGING_ENABLED` or `JUCE_DEBUG` guard before shipping. Logging is dev-only infrastructure.
- **COPY_PLUGIN_AFTER_BUILD:** FALSE (requires admin for C:\Program Files\Common Files\VST3)
- **Known Issue:** Em dash (—) doesn't render in JUCE's default font — use plain dashes in JUCE text
- **Initialization Plan:** `.agent/plans/Solace Synth — Initialization Plan.md`

---

## 📌 Current Status & Next Steps

### Done
- [x] UI design prototype completed by friend (Figma, slider-based, second iteration)
- [x] Initial research: framework, approach, reference repos, licensing
- [x] Framework confirmed: JUCE 8
- [x] Build approach: from scratch with references, not a fork
- [x] License education and decision — using JUCE Starter (free commercial), project license deferred
- [x] APC deep-dive evaluation (verdict: good scaffolding, zero synth DSP)
- [x] Vision doc v0.1 completed
- [x] V1 / V2 scope defined
- [x] Figma → plugin UI workflow discussed and decided
- [x] Team roles defined: Anshul = backend DSP, friend = UI/design + HTML/CSS frontend
- [x] Project name confirmed: **Solace Synth**
- [x] GitHub repo created: **AnshulJ999/Solace-Synth** (G:\GitHub\Solace-Synth)
- [x] UI framework confirmed: **WebView (HTML/CSS/JS in JUCE 8)**
- [x] JUCE inclusion method confirmed: **CMake FetchContent** (not submodule)
- [x] JUCE licensing confirmed: **Starter (free commercial)** — NOT AGPLv3
- [x] CMakeLists.txt strategy confirmed: **based on pamplejuce patterns** (not a repo clone)
- [x] Neural DSP UI research completed — believed to use JUCE + custom OpenGL rendering. WebView is the right choice for our team.
- [x] Initialization plan approved — 5-phase approach (scaffolding → JUCE setup → Hello World → WebView → First Sound)
- [x] **Phase 0: Dev environment** — VS2026 + CMake 4.2.3 installed and verified
- [x] **Phase 1: Repo scaffolding** — .gitignore, README.md created
- [x] **Phase 2: JUCE project setup** — CMakeLists.txt (FetchContent), PluginProcessor (with APVTS), PluginEditor (placeholder)
- [x] **Phase 3: Hello World** — VST3 + Standalone build and run successfully (2026-03-09)
- [x] **Phase 4: WebView Integration** — COMPLETE (2026-03-09)
  - WebBrowserComponent with ResourceProvider, WebView2 backend
  - C++↔JS bridge: setParameter/uiReady/log (JS→C++) + parameterChanged/syncAllParameters (C++→JS)
  - UI/index.html with masterVolume slider, bridge.js, main.js, styles.css
  - **Bug history (all fixed):**
    1. Build break: `NEEDS_WEBVIEW2 TRUE` missing in CMakeLists.txt → added
    2. WebView2 NuGet package not installed → installed via PowerShell
    3. `JUCE_USE_WIN_WEBVIEW2=1` compile definition missing → added
    4. Asset loading: exe-relative `UI/` lookup fails in VST3 hosts → replaced with `SOLACE_DEV_UI_PATH` compile-time path
    5. Use-after-free: raw `this` in `callAsync` → replaced with `SafePointer`
    6. Visibility drift: events dropped when editor hidden → added `visibilityChanged()` resync
    7. C4390 warning: `DBG` macro on single-line `if` → added braces
    8. Heap alloc: unnecessary `new` for paramsArray → stack-allocated
    9. JS bridge API mismatch (`window.__JUCE__.backend.getNativeFunction`) → rewrote bridge.js to JUCE's `__juce__invoke` pattern; added `try/catch` in `main.js` init
  - Multi-level logging: SolaceLogger (trace/debug/info files)
  - Audio thread safety verified (no disk I/O on audio thread)
  - **Verified in trace.log:** round-trip values 0.08, 0.30, 0.44, 0.64, 0.78 all correct
  - **Production note:** **COMPLETED** — UI files are now embedded via `juce_add_binary_data()` in the Release build for portability.

### Phase 6.1 — Amp ADSR (2026-03-09) — COMPLETE

**New files:**
- `Source/DSP/SolaceADSR.h` — Thin wrapper around `juce::ADSR`. Methods: `prepare(sampleRate)`, `setParameters(attack, decay, sustain, release)`, `trigger()`, `release()`, `getNextSample()`, `isActive()`, `reset()`.

**Modified files:**
- `Source/DSP/SolaceVoice.h` — Major rewrite:
  - Introduced `SolaceVoiceParams` struct (at top of file) holding `const std::atomic<float>*` pointers for all APVTS params the voice reads. Currently: `ampAttack`, `ampDecay`, `ampSustain`, `ampRelease`.
  - Constructor is now `explicit SolaceVoice(const SolaceVoiceParams&)` — stores params by value, jasserts all pointers non-null.
  - New `void prepare(const juce::dsp::ProcessSpec&)` method — calls `setCurrentPlaybackSampleRate()` + `ampEnvelope.prepare(sampleRate)`.
  - `startNote()` — resets angle, stores velocity (0–1 float), snapshots APVTS via `.load()`, calls `ampEnvelope.setParameters()` + `trigger()`, computes angleDelta.
  - `stopNote()` — calls `ampEnvelope.release()` for tail-off; `ampEnvelope.reset()` + `clearCurrentNote()` for immediate cut.
  - `renderNextBlock()` — per sample: `getNextSample()` × `kVoiceGain` × `velocityScale` × `envValue`. Checks `isActive()` AFTER advancing to catch envelope completion. Calls `clearCurrentNote()` + resets `angleDelta` when done.
  - Removed: manual `tailOff` exponential decay.
  - `kVoiceGain = 0.15f` static constexpr (same as Phase 5; TODO 6.7 for dynamic normalization).
- `Source/PluginProcessor.cpp`:
  - `createParameterLayout()` — added `ampAttack` (0.001–5s, default 0.01), `ampDecay` (0.001–5s, default 0.1), `ampSustain` (0–1, default 0.8), `ampRelease` (0.001–10s, default 0.3).
  - Constructor — builds `SolaceVoiceParams` from `apvts.getRawParameterValue()`, bumped voice count from 8 → **16**, passes voiceParams to each `new SolaceVoice(voiceParams)`.
  - `prepareToPlay()` — calls `synth.setCurrentPlaybackSampleRate()` first, then builds `juce::dsp::ProcessSpec`, iterates via `synth.getVoice(i)` + `dynamic_cast<SolaceVoice*>`, calls `voice->prepare(spec)` on each.

**Pattern used:** Standard production JUCE pattern from BlackBird/ProPhat: `prepareToPlay` → build `ProcessSpec` → iterate voice pool with `getVoice(i)` + `dynamic_cast` → call `voice->prepare(spec)`.

**Build verification STATUS:** COMPLETE & VERIFIED.

### Next Up
- [x] **Phase 5: First Sound** — COMPLETE (2026-03-09)
  - `Source/DSP/SolaceSound.h` — `SynthesiserSound` tag class (all notes/channels)
  - `Source/DSP/SolaceVoice.h` — `SynthesiserVoice` with sine oscillator, velocity scaling, exponential tail-off on release
  - `PluginProcessor.h` — `juce::Synthesiser synth` + `juce::MidiKeyboardState keyboardState` added; `juce_audio_utils` included
  - `PluginProcessor.cpp` — 8 SolaceVoice + 1 SolaceSound in constructor; `prepareToPlay` sets sample rate; `processBlock`: clear → `keyboardState.processNextMidiBuffer()` → `synth.renderNextBlock()` → apply masterVolume
  - `PluginEditor.h/cpp` — `juce::MidiKeyboardComponent midiKeyboard` added as 80px strip at bottom of window; auto-grabs keyboard focus 500ms after launch
  - **Verified:** polyphonic sine tones play correctly. Mouse click on piano strip ✅. Computer keyboard (A/S/D/F/etc.) ✅. Polyphony ✅. Volume slider controls output level ✅.
  - **Architecture note:** MidiKeyboardState lives in Processor (audio thread access). Editor holds MidiKeyboardComponent (UI thread). Cross-thread: documented as safe by JUCE.
- [x] **Phase 6: Core synth shaping** — COMPLETE
  - [x] 6.1 Amp ADSR
  - [x] 6.2 Oscillator waveforms + Osc1 tuning
  - [x] 6.3 Filter (LadderFilter LP24)
  - [x] 6.4 Filter Envelope
  - [x] 6.5 Second Oscillator + Osc Mix
  - [x] 6.6 LFO (3 targets, per-voice free-running)
  - [x] 6.7 Unison
  - [x] 6.8 Voicing params
  - [x] 6.9 Master Distortion
- [x] **Phase 7: UI & Packaging** — COMPLETE (up to 7.5)
- [x] **Standalone Packaging** — COMPLETE

### Pre-Release Backlog (do before shipping)
- [ ] Conditional logging guard (`SOLACE_LOGGING_ENABLED` or `JUCE_DEBUG`)
- [ ] Multi-instance logger safety (currently global `setCurrentLogger`)
- [ ] Embed UI files via `juce_add_binary_data()` instead of disk path
- [ ] Full Figma UI implementation

### Pending — Spec Gaps (need decisions before DSP implementation)
- [x] **Waveform list:** Sine, Saw, Square, Triangle in V1. **Noise deferred to V2** (per-voice PRNG complexity).
- [x] **Filter in V1:** LP12, LP24, HP12 using `juce::dsp::LadderFilter`. **LP24 is the default** (matches Figma design).
- [x] **LFO target list:** None, FilterCutoff, Osc1Pitch, Osc2Pitch, Osc1Level, Osc2Level, AmpLevel, FilterResonance (8 targets, 0-7).
- [x] **Velocity mod target list:** None, AmpLevel, AmpAttack, FilterCutoff, FilterResonance (5 targets, 0-4).
- [x] **Osc Mix:** Single vertical crossfader (`oscMix`, 0.0=Osc1 only, 1.0=Osc2 only, 0.5=equal blend). Confirmed by user. **Not** two separate level faders.
- [x] **Osc2 defaults:** Waveform=Square (index 2), Octave=1 (one octave above Osc1). Confirmed from Figma screenshot.
- [x] **Both oscillators get tuning params:** `osc1Octave`, `osc1Transpose`, `osc1Tuning` added in 6.2. `osc2Octave`, `osc2Transpose`, `osc2Tuning` in 6.5.
- [x] **LFO has 3 target slots** (`lfoTarget1/2/3`) and one shared amount slider (`lfoAmount`). Matches 3 dropdown targets in Figma.
- [x] **Plugin window:** Resizable (`setResizable(true, false)` + `setResizeLimits()`). CSS uses relative units. Fallback: 3 size presets.
- [x] **UI theme:** Light / white background, orange accent sliders. Dark theme is a V2 nice-to-have.
- [x] **LFO scope: per-voice, free-running.** Confirmed by Nabeel. Each `SolaceVoice` owns its own `SolaceLFO`. LFO runs continuously from voice allocation — do NOT reset phase in `startNote()`. This produces organic drift when playing chords (each voice's LFO is at a different phase).
- [ ] **Filter Env Depth** — NOT in Figma design (confirmed 2026-03-09 by Anshul). Hidden in HTML (`hidden` attr on `#filter-env-depth-fader`). DO NOT surface this unless Nabeel explicitly adds it to the design. AI agents must not add it without approval.
- [ ] **unisonDetune / unisonSpread** — confirmed PENDING. Not to be added to UI until designer decides. No controls in HTML. Do not add without approval.
- [ ] **Plugin title in UI:** \"Solace Soft Synth\" (Figma) vs \"Solace Synth\" (shorter)? Ask designer.

### ✅ SVG Icon Status — FINALIZED
`UI/assets/icons/` now contains final SVG assets for the logo, waveforms, and UI navigation buttons. Placeholders have been replaced.

**Note:** The Figma MCP tool is no longer needed for asset discovery as the local icons folder is now the source of truth.

### ⚠️ Agent Rule: Confirm Design Decisions With Anshul, Not AI Peers
Do NOT implement features suggested solely by Claude Code/Codex reviews without Anshul's explicit approval. Example: `filterEnvDepth` was added based on Claude Code's review, then had to be hidden. Claude Code reviews are input, not authority.
- [ ] **Plugin title in UI:** "Solace Soft Synth" (Figma) vs "Solace Synth" (shorter)? Ask designer.

### Pending — Implementation (after Phase 5)
- [x] **Phase 6.1: Amp ADSR** — COMPLETE (2026-03-10)
  - `Source/DSP/SolaceADSR.h` — thin wrapper around `juce::ADSR`: prepare / setParameters / trigger / release / getNextSample / isActive / reset
  - `SolaceVoiceParams` struct — holds `const std::atomic<float>*` pointers; grows cleanly with each phase
  - `SolaceVoice` rewritten — ADSR replaces manual tailOff, snapshots params at note-on, 16 voices, velocity scaling via `kVoiceGain`
  - `createParameterLayout()` — 4 amp params: `ampAttack` (0.001–5s, def 0.01), `ampDecay` (0.001–5s, def 0.1), `ampSustain` (0–1, def 0.8), `ampRelease` (0.001–10s, def 0.3)
  - `getTailLengthSeconds()` — fixed: now returns live `ampRelease` value (not 0.0); TODO 6.4: update to `max(ampRelease, filterEnvRelease)`
  - Post-review fix 1: removed redundant `setCurrentPlaybackSampleRate()` in `voice->prepare()` — JUCE Synthesiser already propagates it
  - Post-review fix 2: `getTailLengthSeconds()` was `return 0.0` — would have silently dropped release tails in DAW renders
  - Build: successful (pre-fix). Manual listening test: pending (rebuild after fixes).
- [x] **Phase 6.2: Oscillator Waveforms + Osc1 Tuning** — COMPLETE (2026-03-10)
  - `Source/DSP/SolaceOscillator.h` — NEW. Phase accumulator, 4 waveforms (Sine/Saw/Square/Triangle), setTuningOffset (2^oct * 2^(semi/12) * 2^(cents/1200)), setFrequency, reset, getNextSample. All waveforms output [-1, +1] (guaranteed by full phase wrap).
  - `Source/DSP/SolaceVoice.h` — replaced inline `std::sin()` with `SolaceOscillator osc1`. `SolaceVoiceParams` extended with `osc1Waveform/Octave/Transpose/Tuning`. `startNote()` order: reset → setWaveform → setTuningOffset → setFrequency.
  - `PluginProcessor.cpp` — 4 new params: `osc1Waveform` (int 0-3, def 0), `osc1Octave` (int -3 to +3, def 0), `osc1Transpose` (int -12 to +12, def 0), `osc1Tuning` (float -100 to +100 cents, def 0).
  - Post-review fix: phase wrap `if` → `while` — with max tuning offset, `angleDelta` can be ~30 rad (>> 2π), a single subtraction was insufficient for Saw/Triangle correctness.
  - Build: successful. Waveforms audibly distinct — listening test passed.
  - Note: Naïve Saw/Square alias at high frequencies — intentional V1 behaviour. PolyBLEP tracked for V2.

- [x] **Phase 6.3: Filter** — COMPLETE (2026-03-10)
  - `Source/DSP/SolaceFilter.h` — NEW. Wrapper around `juce::dsp::LadderFilter<float>`. Per-sample via 1-sample AudioBlock (LadderFilter::processSample is `protected` in JUCE 8, not public). AudioBlock is non-allocating stack wrapper. Modes: LP12 (0), LP24 (1, default), HP12 (2). Cutoff clamped [20, 20000 Hz]. Filter reset on voice steal.
  - `Source/DSP/SolaceVoice.h` — added `SolaceFilter filter` member + `float baseCutoffHz`. Signal order: osc1 → filter → amp ADSR (correct subtractive order). Live per-block refresh of filterCutoff and filterResonance from APVTS atomics, so knob moves immediately affect held notes. filterType stays at note-on snapshot (mode switches mid-note click). Phase 6.4 integration point commented in the sample loop.
  - `PluginProcessor.cpp` — 3 new params: `filterCutoff` (20-20000Hz, skew=0.3, default 20000 = fully open), `filterResonance` (0-1, default 0), `filterType` (int 0-2, default 1=LP24).
  - Post-review fixes: (1) processSample → AudioBlock (protected API fix); (2) live per-block filter refresh added (was note-on snapshot only).
  - UI: filterCutoff, filterResonance, filterType controls already wired in main.js (Phase 7.2). Will activate automatically once params are registered.


- [x] **Phase 6.4: Filter Envelope** — COMPLETE (2026-03-10)
  - `Source/DSP/SolaceVoice.h` — `SolaceVoiceParams` extended with 5 new atomics. `SolaceADSR filterEnvelope` member added. `prepare()`: `filterEnvelope.prepare(sampleRate)`. `startNote()`: `filterEnvelope.reset()` → `setParameters()` → `trigger()`. `stopNote()`: `filterEnvelope.release()` on tail-off; `filterEnvelope.reset()` on hard cut. `renderNextBlock()`: per-block reads `baseCutoffHz` + `envDepth`; per-sample: `modulatedCutoff = baseCutoffHz + filterEnv.getNextSample() * envDepth * 10000.0f` → `filter.setCutoff()`.
  - `PluginProcessor.cpp` — 5 new APVTS params registered.
  `filterEnvDepth` (-1 to +1, def 0), `filterEnvAttack` (0.001-5s, def 0.01), `filterEnvDecay` (0.001-5s, def 0.3), `filterEnvSustain` (0-1, def 0.0), `filterEnvRelease` (0.001-10s, def 0.3). `getTailLengthSeconds()` updated to `std::max(ampRelease, filterEnvRelease)` — resolves the Phase 6.1 TODO.
  - UI: all 5 filter env params already wired in `main.js` (Phase 7.2 scaffold). Active on build.
  - Out-of-the-box behaviour: `filterEnvDepth=0` → no envelope effect, sounds identical to 6.3 on launch. Move depth to hear the envelope.
  
  - `getTailLengthSeconds()` returns `ampRelease` only (not max of both — filter env does not extend audible output since amp multiplies signal to zero first).
  - Post-review fixes: (1) `filterEnvelope.reset()` added before `trigger()` in `startNote()` — prevents ADSR::noteOn() restart from non-zero level when voice reused mid-release (would have caused audible artifact on pluck attack); (2) Reverted `getTailLengthSeconds()` to `ampRelease` only — architecturally consistent with voice lifetime model.
  - Build: successful. Listening test: passed. Gemini approved ✅.


- [x] **Phase 6.5: Second Oscillator + `oscMix` crossfader** — code complete (2026-03-10), code review complete (2026-03-10), awaiting build + listening test
  - `Source/DSP/SolaceVoice.h` — `SolaceVoiceParams` extended with 5 atomics (`osc2Waveform/Octave/Transpose/Tuning`, `oscMix`). `SolaceOscillator osc2` private member added. `startNote()`: `osc2.reset() → setWaveform() → setTuningOffset() → setFrequency(baseHz)` — same pattern as osc1, same MIDI base frequency, independent tuning offsets. `oscMix` read per-block in `renderNextBlock()` (live crossfader response while notes held). Sample loop: both oscillators always call `getNextSample()` (phase-continuous crossfader sweep); `blendedOsc = osc1 * (1-mix) + osc2 * mix` feeds into filter unchanged.
  - `PluginProcessor.cpp` — 5 new APVTS params: `osc2Waveform` (int 0-3, def **2=Square**), `osc2Octave` (int -3 to +3, def **1=+1 octave**), `osc2Transpose` (int -12 to +12, def 0), `osc2Tuning` (float -100 to +100 cents, def 0), `oscMix` (float 0-1, def **0.5=equal blend**). 5 new `voiceParams` atomics.
  - Default patch: Osc1=Sine (unison) + Osc2=Square (+1 octave), 50/50 blend — immediately interesting, good demo of dual-osc capability.
  - Design note: no `SolaceOscillator` changes needed — class already supports second instance reuse. UI bindings for all 5 params already in `main.js` from Phase 7.2 scaffold.
  - **Code review verdict (Claude Code + Codex, 2026-03-10):** Phase 6.5 code correct. All decisions validated. No 6.5 fixes needed.
  - **Post-review fix applied:** `filter.reset()` added in `startNote()` before `filter.setMode()` — closes Phase 6.3 bug found during 6.5 review. At high resonance (near self-oscillation), the LadderFilter delay state persists after a natural note release; without `filter.reset()`, a reused voice starts with a ringing delay line. Fix is 1 line, same pattern as `filterEnvelope.reset()` immediately below it.
  - **SolaceFilter.h first review:** All design decisions confirmed correct (LadderFilter choice, monoSpec, setCutoff/setResonance clamps, AudioBlock processSample workaround). No issues.
  - **Non-blocking tracked items:** (1) `oscMix` per-block stepping could zip under aggressive automation — V2 candidate for `SmoothedValue`; (2) `kVoiceGain=0.15f` normalisation — tracked in Phase 6.7 TODO.
  - **Build: successful (2026-03-10). Listening test: passed (2026-03-10). COMPLETE ✅**


- [x] **Phase 6.6: LFO** — code complete (2026-03-10), awaiting build + listening test
  - `Source/DSP/SolaceLFO.h` — NEW. Free-running per-voice LFO. 5 waveforms (Sine/Triangle/Saw/Square/S&H via `Shape` enum 0-4). Per-instance `juce::Random` for S&H (each voice gets independent random sequence). `getCurrentValue()` reads LFO without phase advance (for per-block pitch target setup). `getNextSample()` advances phase + detects cycle boundary for S&H latch. `reset()` exists but must NOT be called in `startNote()` — LFO is free-running by design.
  - `Source/DSP/SolaceOscillator.h` — added `setLFOPitchMultiplier(double)` method and `double lfoMultiplier = 1.0` member. `getNextSample()` now uses `angleDelta * lfoMultiplier` (was `angleDelta`). Default 1.0 = no modulation. Cheaper than re-calling `setFrequency()` per block.
  - `Source/DSP/SolaceVoice.h` — `SolaceVoiceParams` extended with 6 new atomics (`lfoWaveform/Rate/Amount/Target1/2/3`). `SolaceLFO lfo` private member added. Constructor: 6 new `jassert` checks. `renderNextBlock()`: per-block sets shape/rate/amount/targets, pre-computes 7 `bool` target flags (compiler-hoistable), sets pitch multipliers via `getCurrentValue()` before sample loop; per-sample advances LFO, applies level mod (jlimit 0-2), cutoff mod (+-10000 Hz), resonance mod (+-0.5), amp mod (jlimit 0-2).
  - `PluginProcessor.cpp` — 6 new APVTS params: `lfoWaveform` (int 0-4, def 0=Sine), `lfoRate` (float 0.01-50Hz, skew 0.3, def 1.0), `lfoAmount` (float 0-1, def 0.0=no effect), `lfoTarget1` (int 0-7, def 1=FilterCutoff), `lfoTarget2` (int 0-7, def 0=None), `lfoTarget3` (int 0-7, def 0=None). 6 `voiceParams` atomics populated before voice creation loop.
  - LFO target enum (in SolaceVoiceParams comment): 0=None, 1=FilterCutoff, 2=Osc1Pitch, 3=Osc2Pitch, 4=Osc1Level, 5=Osc2Level, 6=AmpLevel, 7=FilterRes.
  - Key design: `lfoAmount=0.0` default means zero LFO effect on launch — user opts in. `lfoTarget1=1` (FilterCutoff) is the most natural default first target.
  - **Post-Codex-review fixes (2026-03-10):** 3 bugs fixed in `SolaceLFO.h`:
    - (1) LFO phase alignment: all voices started at `currentAngle=0.0` → simultaneous chord had no phase diversity. Fixed: constructor seeds `currentAngle` via `getSystemRandom().nextDouble() * 2π`.
    - (2) S&H initial silence: `sAndHValue=0.0f` default → up to 1 full LFO cycle of silent modulation on first S&H use. Fixed: constructor seeds `sAndHValue` via `getSystemRandom().nextFloat() * 2 - 1`.
    - (3) `juce::Random` seed=1 default: all instances produced identical S&H sequences. Fixed: constructor calls `random.setSeed(getSystemRandom().nextInt64())` for true per-voice independence.
- [x] **Phase 6.7: Unison** — code complete (2026-03-11), awaiting build + listening test
  - `Source/DSP/SolaceVoice.h` — `SolaceVoiceParams` extended with 3 new atomics (`unisonCount/Detune/Spread`). 3 new `jassert` checks in constructor. Private `osc1` + `osc2` members replaced by `UnisonVoice unisonVoices[8]` array + `kMaxUnison=8` constant. `UnisonVoice` is a nested struct with `SolaceOscillator osc1, osc2, float panL, panR`. `activeUnisonCount` (int) is snapshotted at note-on. `voiceGain` (float) replaces old `static constexpr kVoiceGain`, computed dynamically at note-on. Old single `SolaceFilter filter` replaced by `SolaceFilter filterL, filterR` (dual instances for true stereo).
  - `startNote()` — reads `unisonCount` (clamp 1-8) and `unisonDetune` atomics at note-on (snapshotted; not safe to resize mid-note). For each active unison voice: computes `detuneOffset = ((u / (N-1)) - 0.5) * detuneCents` (symmetric, N=1 → 0 cents). Sets waveform, tuning, and frequency for both osc1 and osc2 in every unison slot. Computes `voiceGain = kBaseVoiceGain / sqrt(activeUnisonCount)` for equal-power normalisation. Both `filterL` and `filterR` reset and initialised identically.
  - `renderNextBlock()` — per-block: reads `unisonSpread`, recomputes `panL/panR` for each slot via constant-power pan law: `pan_i = ((u / (N-1)) * 2 - 1) * spread`, `panL = sqrt(0.5 * (1 - pan))`, `panR = sqrt(0.5 * (1 + pan))`. Per-sample: each unison voice advances its own detuned osc pair, accumulates into `preFiltL` and `preFiltR` separately (using `panL/panR` weighting). `filterL.processSample(preFiltL)` and `filterR.processSample(preFiltR)` independently. Then amp env + gain scalar applied to both channels. L and R written to output buffer separately.
  - `PluginProcessor.cpp` — 3 new APVTS params: `unisonCount` (int 1-8, def 1), `unisonDetune` (float 0-100 cents, step 0.1, def 0.0), `unisonSpread` (float 0-1, step 0.01, def 0.5). 3 `voiceParams` atomics populated before voice creation loop. `prepareToPlay` log string updated: `unison(3)`.
  - **`prepare()`** — now calls `filterL.prepare(spec)` + `filterR.prepare(spec)` instead of single `filter.prepare()`.
  - **`stopNote()` hard-cut** — resets both `filterL.reset()` and `filterR.reset()`.
  - **Architecture decision — dual filter vs mono filter:**
    - True stereo spread requires genuinely different content in L and R channels. This requires panning oscillator signals **before** the filter input, giving filterL and filterR different pre-filter signals.
    - Pre-filter accumulation: `preFiltL += uMixed * panL`, `preFiltR += uMixed * panR`. This means each channel's filter input is a different blend of detuned oscillators — filterL and filterR produce genuinely different outputs.
    - At N=1 spread=0: `panL = panR = sqrt(0.5)`, so `preFiltL = preFiltR`, and both filter outputs are identical — centred mono, backward-compatible with Phase 6.6.
  - **`kBaseVoiceGain = 0.15 * sqrt(2) = 0.2121`:** At N=1 spread=0, `preFiltL = osc * sqrt(0.5) = osc * 0.707`. After filter and gain: `sampleL = filter(osc) * 0.707 * kBaseVoiceGain = filter(osc) * 0.15`. Matches pre-6.7 `kVoiceGain=0.15` mono output exactly.
  - **Post-review fixes (2026-03-11, caught by Claude Code + Codex):**
    - **(BUG - MEDIUM)** Stereo spread was non-functional: original code computed one `scalar = filteredMono * voiceGain * ...` then summed `scalar * panL[u]` across all voices. Because scalar was identical per voice, `sampleL = scalar * Σ(panL)` and `sampleR = scalar * Σ(panR)`. Symmetric spread means `Σ(panL) == Σ(panR)` always → dual-mono output regardless of spread knob. **Fixed** by moving pan application before the filter (dual preFiltL/preFiltR + filterL/filterR as described above).
    - **(LOW)** `outputBuffer.getNumChannels()` was called inside the `while` loop (every sample). Hoisted above the loop.
    - **(LOW)** Double `pow()` call: when both `lfoToOsc1Pitch` and `lfoToOsc2Pitch` were true, `getCurrentValue()` and `pow()` were called twice with identical inputs. Merged into single `if (lfoToOsc1Pitch || lfoToOsc2Pitch)` block with one `pow()`.
    - **(LOW)** Signal flow comment referenced old `kVoiceGain` (deleted). Updated to `voiceGain`.
  - **unisonSpread is live per-block** (affects panL/panR in pre-filter accumulation). unisonCount and unisonDetune snapshotted at note-on (resizing active oscillator array mid-note is not audio-thread safe; user hears change on next note).
  - **Default unisonCount=1:** Confirmed by user. Figma shows 3, but plan says 1. Default 1 chosen for backward compatibility and no loudness surprise on first launch.
  - **Post-review fixes (2026-03-11, caught by Claude Code — after Phase 6.8 landed):**
    - **(MEDIUM)** Mono host fold-down bug: when `numChannels == 1`, code wrote only `sampleL`. With spread>0, right-panned unison voices were dropped. Fixed: explicit `else if (numChannels == 1)` now writes `0.5f * (sampleL + sampleR)`.
    - **(LOW)** Architecture header comment (lines 138-141) still referenced old "mono first, shared filter" approach. Updated to describe actual dual-filter pre-filter panning architecture.
    - **(LOW)** `kBaseVoiceGain` comment header said "= 0.15 (not 0.15 * sqrt(2))" but actual value is `0.15 * sqrt(2)` and body proved this. Fixed to "same value as before the dual-filter change".
- [x] **Phase 6.8: Voicing Parameters** — COMPLETE (2026-03-11)
  - **Architecture: SolaceSynthesiser subclass for polyphony cap (steal mode)**
    - `juce::Synthesiser synth` in `PluginProcessor.h` replaced with `SolaceSynthesiser synth` (new subclass defined in same header, above `SolaceSynthProcessor`).
    - `SolaceSynthesiser` overrides `noteOn()`. Per-call: counts active voices via `isVoiceActive()`. In both below-cap and at-cap cases, calls `Synthesiser::noteOn()`. At cap, JUCE's `findVoiceToSteal()` picks the oldest voice, stops it, and assigns it to the new note — **steal mode**: player always hears the new note, oldest note dies. (Contrast drop mode where new note would be silent beyond cap.)
    - `setVoiceLimit(int)` called in `processBlock()` before `synth.renderNextBlock()`, syncing the APVTS `voiceCount` value each block. All 16 `SolaceVoice` instances are always allocated; the limit is dynamic at the synth-allocation level, not voice level.
    - Rationale for synth subclass: voice allocation is a synthesiser-level concern. A per-voice counter (e.g. shared `atomic<int>`) is fragile because JUCE's voice stealer calls `stopNote(false)` then `startNote()` on the same voice in sequence — a bail in `startNote()` would drop the new note instead of stealing.
  - **Velocity Modulation:**
    - `SolaceVoiceParams` extended with 4 pointers: `velocityRange`, `velocityModTarget1`, `velocityModTarget2`, `voiceCount`.
    - 4 new `jassert` checks in `SolaceVoice` constructor.
    - In `startNote()`: reads `velocityRange` and both targets. Computes booleans `velToAmpLevel`, `velToAmpAttack`, `velToFilterCut`, `velToFilterRes`.
    - **Target 1 — AmpLevel:** `velocityScale = jmap(velRange, 0, 1, 1.0, velocity)`. Range=0 → all notes at full level. Range=1 → level scales with velocity.
    - **Target 2 — AmpAttack:** `modAttack = baseAttack * jmap(vel * range, 0, 1, 1.0, 0.1)`. Hard hit → shorter attack (10% of base). Soft hit → full attack. 0.1 floor prevents click.
    - **Target 3 — FilterCutoff:** `velModCutoffHz = vel * range * 5000 Hz`. Stored as private member, added to `modulatedCutoff` each sample alongside filterEnv and LFO.
    - **Target 4 — FilterRes:** `velModRes = vel * range * 0.5`. Stored as private member, added inside `jlimit(0,1,...)` clamp alongside LFO res mod.
    - All mods are additive (consistent with LFO target pattern from 6.6).
  - **APVTS params added (PluginProcessor.cpp):**
    - `voiceCount` (int 1-16, def 16), `velocityRange` (float 0-1, step 0.01, def 1.0), `velocityModTarget1` (int 0-4, def 2=AmpAttack), `velocityModTarget2` (int 0-4, def 0=None).
  - **Velocity mod target enum:** 0=None, 1=AmpLevel, 2=AmpAttack, 3=FilterCutoff, 4=FilterResonance.
  - **Default `velocityModTarget1=2` (AmpAttack):** Matches Figma. Gives expressiveness without changing loudness dynamics (users who want loudness control set target to AmpLevel).
  - **✅ HIGH BUG FIXED (2026-03-11):** `SolaceSynthesiser::noteOn()` was a no-op. Fixed by replacing it with `findFreeVoice()` + `findVoiceToSteal()` overrides that restrict the searchable voice pool to `[0, voiceLimit)`. Now inactive voices beyond the cap are never touched by JUCE's allocator. The simplistic `noteOn()` pre-count approach failed because JUCE only triggers stealing when ALL voices are occupied -- a cap of 4 with voices 5-16 free means JUCE uses a free voice and ignores the count.
  - **✅ LOW FIXED (2026-03-11):** `jassert(params.voiceCount != nullptr)` replaced with a clarifying comment explaining it's not functionally required by SolaceVoice (it's read by SolaceSynthesiser in processBlock). jassert kept for pointer validity only.
  - **✅ LOW FIXED (2026-03-11):** Stale signal-flow comment (Phase 6.6 era) rewritten to describe the dual-filter, preFiltL/R, velModCutoffHz/velModRes, and mono fold-down architecture.
  - **⚠️ OPEN DESIGN QUESTION — needs Nabeel (2026-03-11):**
    - **Velocity-to-level semantic:** Current behavior: when AmpLevel is NOT in either target slot, `velocityScale = velocity` (level always scales with MIDI velocity). When AmpLevel IS targeted, `velocityScale = lerp(1.0, velocity, velRange)`. This means velocity always affects level unless AmpLevel is targeted AND velocityRange=0. Codex/Claude Code flag this as inconsistent: the documented intent was that AmpLevel is the routing \"gate\" for level sensitivity. Anshul's preference: keep current behavior (feels intuitive). Nabeel should confirm the exact UX intent before V1 release.
    - **Velocity mod target list mismatch:** Vision Document (Nabeel, line 55-63) lists 8 velocity targets: Osc Pitch, Osc Tuning, Amp Attack, Filter Cutoff, Filter Resonance, Distortion, Master Volume, Osc Mix. Current implementation has only 4 (None, AmpLevel, AmpAttack, FilterCutoff, FilterResonance -- 5 enum values total). **The full list from the vision doc needs to be reconciled with the implementation.** This is a V1.x concern -- adding more targets is backward-compatible (APVTS int range extension). Flagged for Nabeel review.
- [x] **Phase 6.9: Master Distortion** — COMPLETE (2026-03-11)
  - **`SolaceDistortion.h`** — new stateless module, single static inline method.
  - **Formula:** `tanh(k * x) / tanh(k)`, where `k = 1.0f + drive * 9.0f`. At drive=0: k=1, output ≈ linear (tanh(x)/tanh(1) is 1.31x scaling -- noted below). At drive=1: k=10, heavy saturation. tanh(k) normalisation prevents loudness jump.
  - **⚠️ Implementation note:** At drive=0, k=1 → tanh(1)≈0.762, so the formula outputs x/0.762 ≈ 1.31x amplification even at \"clean\". This is by design in the plan (noted in Phase 6.9 spec). The plan says \"mild saturation at drive=0\" which matches. If Nabeel prefers truly transparent passthrough at 0, the alternative is `drive==0 → return x` (bypass).
  - **Applied per channel**, before master volume, after renderNextBlock().
  - **APVTS:** `masterDistortion` (float, 0.0-1.0, step 0.01, default 0.0). Default 0.0 maps to k=1 (mild/near-clean).
- [x] **Phase 6.8b: Velocity Mod Expansion** — COMPLETE (2026-03-12)
  - **Problem resolved:** Nabeel confirmed AmpLevel should be default Slot 1, and when AmpLevel is in no slot, volume must be flat (was incorrectly always following velocity).
  - **3 slots** added (was 2). New `velocityModTarget3` APVTS param (int 0-7, default 0=None). Wire-up added in PluginProcessor.cpp constructor. jassert added in SolaceVoice constructor.
  - **Velocity mod target enum expanded from 0-4 to 0-7:**
    - 0=None, 1=AmpLevel (**default Slot1, was AmpAttack**), 2=AmpAttack, 3=FilterCutoff, 4=FilterResonance
    - 5=Distortion (new), 6=Osc1Pitch (new), 7=OscMix (new)
  - **Flat-when-not-routed fix:** `velocityScale` else branch changed from `velocity` to `1.0f`. If AmpLevel is in no target slot, every note plays at full volume regardless of how hard you press.
  - **Distortion target (5):** `velModDistDrive` (float, note-on snapshot) = `vel * range * 0.7f`. Applied pre-filter via `SolaceDistortion::processSample(preFiltL/R, velModDistDrive)` when > 0. Per-voice saturation -- harder hit = more grit on that specific note only. `SolaceDistortion.h` include added to `SolaceVoice.h`.
  - **Osc1Pitch target (6):** `velPitchCents = vel * range * 100.0f` cents. Applied at note-on via `setVelPitchMultiplier(2^(cents/1200))` on all unison osc1 and osc2 instances. New `velPitchMultiplier` member and `setVelPitchMultiplier()` method added to `SolaceOscillator.h`. Multiplied together with `lfoMultiplier` in `getNextSample()` so LFO vibrato and velocity pitch are independent additive pitch ratios.
  - **OscMix target (7):** `velModOscMixOffset = vel * range * 0.5f`. Applied per-block: `mix = jlimit(0,1, params.oscMix->load() + velModOscMixOffset)`. Harder hit blends further toward Osc2.
  - **APVTS skew factors added** to all time-domain params (brings exponential feel to sliders, matching lfoRate):
    - `ampAttack`, `ampDecay`, `filterEnvAttack`, `filterEnvDecay`: skew=0.4
    - `ampRelease`, `filterEnvRelease`: skew=0.3 (longer range, more skew needed)
    - `lfoRate`: already had skew=0.3 ✅
  - **Organic distortion:** The tanh mapping was refined (2026-03-21) to ensure transparent passthrough at zero drive. This provides the "warmth" required without the volume-jump artifacts. Resolved for V1.
- [ ] Phase 7: Full Figma UI implementation
  - [x] **Phase 7.5: Dropdown Popup + Fader UX + DSP Pitch Bend + Mod Wheel** — COMPLETE (2026-03-12)

    **UI — Dropdown (full rewrite, `UI/components/dropdown.js`):**
    - Replaced click-to-cycle behaviour with a proper floating popup panel.
    - Singleton `_sharedPanel` div appended to `<body>` once, shared across all Dropdown instances. Avoids z-index / overflow-clip issues from per-dropdown panels.
    - Panel opens upward (preferred — all dropdowns sit in the bottom row). Falls back downward if not enough room above trigger.
    - Outside-click dismiss: `pointerup` on `document` (bubble phase), deferred via `requestAnimationFrame` to avoid catching the same event that opened the panel.
    - Options use `pointerup + stopPropagation` to prevent the dismiss handler firing on the same event.
    - Escape key closes without changing selection. One popup open at a time — opening a second closes the first.
    - Full ARIA: `role=button`, `aria-haspopup=listbox`, `aria-expanded`, `role=listbox`, `role=option`, `aria-selected`.
    - CSS: `.dropdown-list` (min-width 200px, max-height 360px, overflow-y auto), `.dropdown-option`, `.dropdown-option--selected`, `.dropdown-option:hover`.

    **UI — Vel Mod Target 3rd Slot:**
    - `UI/index.html`: added `#mount-velocityModTarget3` div.
    - `UI/main.js`: `VEL_MOD_TARGET_OPTIONS` expanded from 5 to 8 entries (0=None, 1=AmpLevel, 2=AmpAttack, 3=FilterCutoff, 4=FilterResonance, 5=Distortion, 6=OscPitch, 7=OscMix — matches DSP Phase 6.8b exactly). 3rd Dropdown mounted. `velocityModTarget1` defaultIndex corrected from 2 (AmpAttack) to 1 (AmpLevel) per Nabeel's Phase 6.8b decision.
    - LFO target list remains PENDING (separate from vel mod — vel mod is now complete).

    **UI — Fader Usability (`UI/components/fader.js`):**
    - **Double-click to reset:** double-clicking the range input resets to `defaultValue` and sends to C++.
    - **Click value to type:** clicking the `.fader-value` span spawns an inline `<input type=text>` pre-filled with the raw number. Enter commits + clamps to [min, max] + sends to C++. Escape cancels. Blur also commits (with `committed` flag to prevent double-fire on Enter→blur).
    - **P1 bug fix (2026-03-12):** Inline editor was seeding with `this._fmt(currentVal)` (display label). For `filterCutoff`, this produces `"20.00k"` — `parseFloat("20.00k")` = 20, clamped to 20 Hz, corrupting the parameter silently. Fixed: seed with `parseFloat(this._input.value).toString()` (raw number like `"20000"`). Safe for all other faders.
    - Ctrl+Drag for fine-tuning: **DEFERRED** — marked PENDING for later.
    - CSS: `.fader-value-edit` inline text input styled to match fader value metrics.

    **UI — Waveform Name Labels (`UI/components/waveform-selector.js`):**
    - New `_nameEl` span added below the icon row showing the current waveform name ("Sine", "Sawtooth", etc.).
    - Updated automatically in `_apply()` in sync with the icon change.
    - CSS: `.waveform-name-label` — small, dimmed, centered, non-interactive (`aria-hidden`).

    **UI — Window Size (`Source/PluginEditor.cpp`):**
    - `setSize(900, 600)` → `setSize(1280, 720)` (720p). WebView gets 1280×640 (subtracting 80px MIDI keyboard strip).
    - Resizable window: **deferred** — requires CSS responsive refactor. Plan separately.

    **DSP — Pitch Bend (`Source/DSP/SolaceOscillator.h`, `SolaceVoice.h`):**
    - `SolaceOscillator`: new `pitchBendMultiplier` member (default 1.0), `setPitchBendMultiplier(double)` method. Applied multiplicatively in `getNextSample()` alongside `lfoMultiplier` and `velPitchMultiplier`. All three pitch sources are fully independent.
    - `SolaceVoice::startNote()`: `int /*currentPitchWheelPosition*/` → `int currentPitchWheelPosition`. Calls `_applyPitchBend(currentPitchWheelPosition)` at note-on end to prime the bend correctly. Uses JUCE's built-in per-channel wheel tracking (not a global atomic — JUCE already handles this correctly via `lastPitchWheelValues[channel]` and the parameter passed to `startNote()`).
    - `SolaceVoice::pitchWheelMoved(int newValue)`: implemented — delegates to `_applyPitchBend(newValue)`.
    - `_applyPitchBend(int wheelValue)` private helper: normalises 0-16383 to [-1, +1], multiplies by `kPitchBendRange=2` semitones, `pow(2, semitones/12)`, applies to all active unison osc1+osc2.
    - Bend range: ±2 semitones (`kPitchBendRange = 2`). MIDI standard default. V2: make user-configurable APVTS param.

    **DSP — Mod Wheel (`Source/PluginProcessor.h`, `SolaceVoice.h`, `PluginProcessor.cpp`):**
    - `SolaceSynthesiser::modWheelValue` (std::atomic<int>, default 0): captures CC#1 in `handleMidiEvent()`.
    - `SolaceVoiceParams::modWheelValue` pointer wired in `PluginProcessor.cpp` constructor.
    - `SolaceVoice::renderNextBlock()`: reads `modWheelValue` per-block, normalises to [0,1], adds to `lfoAmountKnob`, clamped to [0,1]. Classic mod wheel = vibrato depth behaviour. Changes take effect next audio block (~2ms at typical buffer — imperceptible, intentional).
    - `SolaceVoice::controllerMoved()`: implemented as a no-op — mod wheel value already captured by `handleMidiEvent()` before this is called. `(void) controller` suppresses warning.
    - Global mod wheel (all channels share one atomic): acceptable V1 behaviour for a mono-timbral synth. Documented.

    **Build fix (2026-03-12):** `int /*currentPitchWheelPosition*/` in `startNote()` signature was commented out. Removing the `/* */` fixed `error C2065: 'currentPitchWheelPosition': undeclared identifier`.

    **Peer reviews processed (Claude Code + Codex, commit 8146cfe):**
    - ✅ All DSP: thread model sound, ordering correct, ARIA complete, bounds checks in place.
    - ✅ Fixed P1 fader edit bug (formatter seeding issue described above).
    - ✅ Fixed P2 pitch wheel: switched from global atomic to JUCE parameter (simpler + more correct).
    - ℹ️ Doc misnomer: `Osc1Pitch` in C++ comments should be `OscPitch` (both oscs shift). Low priority cleanup — does not affect runtime.

- [ ] GitHub Actions CI + pluginval

---

## 🧪 Testing Strategy (Phase 6)

### Three-Layer Approach

| Layer | Tool | When | Purpose |
|-------|------|------|---------|
| **1. Manual listening** | Standalone plugin | After every sub-phase build | Verify DSP sounds correct — ears are the primary gate |
| **2. C++ unit tests (CTest)** | CTest + headless test executables | After 6.3 | Numerical verification: ADSR stages, waveform range [-1,1], no NaN |
| **3. Plugin validation** | `pluginval` (Tracktion) | Before DAW release / Phase 7 gate | Full host stress test: polyphony, parameter save/load, thread safety |

### Automation Rollout
- **6.1–6.2:** Manual listening only. Fast iteration priority.
- **After 6.3:** Add `test_adsr.cpp`, `test_oscillator.cpp`, `test_filter.cpp` — small headless CTest executables (no audio hardware needed).
- **Phase 7 (pre-release):** `pluginval.exe --validate-in-process "Solace Synth.vst3" --strictness-level 5`
- **Pre-release:** GitHub Actions CI: build + CTest + pluginval headless on every push to `dev-anshul`.

### General Rules (Every Sub-Phase)
1. **Build gate:** `cmake --build build --config Release` — zero new warnings. Never commit a failing build.
2. **Log check:** `%TEMP%\SolaceSynth\info.log` — confirm new APVTS params appear on startup.
3. **No crash:** Launch standalone, open window, play notes.
4. **No silence:** Notes produce audio.

### Velocity Testing Note
- Computer keyboard → JUCE sends fixed velocity (64). Cannot test velocity range this way.
- With MIDI keyboard: soft/hard press sends different velocities.
- JUCE's on-screen MidiKeyboardComponent: click **top of key** = low velocity, **bottom of key** = high velocity (built-in JUCE behaviour).

### Per-Phase Manual Test Checklist

#### Phase 6.1 — Amp ADSR (COMPLETE)
- [x] No crash, 16 voices loaded
- [x] Basic notes produce sound
- [x] Attack fade-in
- [x] Release fade-out
- [x] Tail length: release audible
- [x] Velocity range

#### Phase 6.2 — Waveforms + Osc1 Tuning (COMPLETE)
- [x] Sine, Saw, Square, Triangle each sound distinct
- [x] Tuning offsets (Oct/Trans/Fine) correct
- [x] Pitch correct at 440Hz

#### Phase 6.3 — Filter (COMPLETE)
- [x] Sweep cutoff correctly
- [x] Max resonance → self-resonance
- [x] LP12/LP24/HP12 modes functional

#### Phase 6.4 — Filter Envelope (COMPLETE)
- [x] Pluck/Sweep envelopes functional
- [x] Env depth (positive/negative) working

#### Phase 6.5 — Osc2 + Mix (COMPLETE)
- [x] Both oscillators audible and tunable
- [x] OscMix crossfader working smoothly

#### Phase 6.6 — LFO (COMPLETE)
- [x] All 3 targets modulating in real-time
- [x] Free-running phase diversity verified

#### Phase 6.7 — Unison (COMPLETE)
- [x] Thick supersaw tones verified
- [x] Stereo spread verified

---

## 🌿 Git Workflow & Collaboration

### Branch Strategy
```
main          ← stable only. Never commit directly. Merge via PR when a sub-phase is complete and verified.
dev-anshul    ← Anshul's active working branch (all C++/DSP/bridge work). Primary dev branch.
dev-nabeel    ← Nabeel's branch. UI/ folder only. Never touches Source/.
```

### Rules
- **Anshul:** Works on `dev-anshul`. Merges `dev-nabeel` → `dev-anshul` after reviewing diffs (verify only `UI/` files changed). Merges `dev-anshul` → `main` when a sub-phase is stable.
- **Nabeel:** Works on `dev-nabeel` only. Only edits `UI/` folder. Commits + pushes via Antigravity UI. Never merges branches himself. Never touches `Source/`.
- **PRs to main:** Always use a PR (even solo), for the diff review moment.
- **No force pushes** to `main` or `dev-anshul` ever.
- **Branch protection** on GitHub: `main` and `dev-anshul` require PR, no direct push.

### Tagging / Checkpointing
After each sub-phase is merged to `main`:
```bash
git tag v0.1-phase6.1-adsr
git push origin --tags
```
Tags allow instant rollback to any verified state without hunting for commit hashes.

### Nabeel's Hot-Reload Workflow
For **Debug/dev builds**, UI files can still be served from disk at runtime:
1. Edit files in `UI/` → Save
2. Close + reopen the plugin window (or refresh WebView)
3. Changes visible immediately — no C++ compile needed

For **Release/distribution builds**, the UI is embedded via JUCE BinaryData:
1. Edit files in `UI/`
2. Rebuild the project
3. New standalone / plugin artifacts contain the updated embedded UI

### If Nabeel breaks his branch
```bash
# Reset dev-nabeel to last known good commit (Anshul runs this)
git checkout dev-nabeel
git reset --hard <good-commit-hash>
git push --force-with-lease origin dev-nabeel
```
Since `dev-nabeel` only contains `UI/` changes, this is always safe — no audio engine code is at risk.

---

## 🎨 Design Spec (confirmed from Figma screenshots)

**Reference:** `Screenshots/Interface-2-Faders-Main-New.png`
- Theme: Light/white background, orange accent sliders
- All continuous params: vertical faders
- Discrete/enum params: arrow selectors (`< value >`)
- Target selectors: dropdowns (truncated label + arrow)
- Sections (top row): Osc 1, Osc Mix, Osc 2, Amplifier Envelope, Master
- Sections (bottom row): Filter, Filter Configuration, Low Frequency Oscillator, Voicing
- Logo: "SS" monogram with orange highlight

---

## ❓ Open Questions

1. ~~Final project name?~~ → **RESOLVED: Solace Synth**
2. ~~WebView vs native JUCE?~~ → **RESOLVED: WebView confirmed**
3. ~~JUCE license?~~ → **RESOLVED: Starter (free commercial), project license deferred**
4. ~~JUCE inclusion method?~~ → **RESOLVED: CMake FetchContent**
5. ~~Waveform list?~~ → **RESOLVED: Sine/Saw/Square/Triangle in V1; Noise deferred**
6. ~~Filter HP in V1?~~ → **RESOLVED: LP12/LP24/HP12 via LadderFilter; LP24 is default**
7. ~~Osc Mix: crossfader or two levels?~~ → **RESOLVED: single vertical crossfader (`oscMix`, 0.0=Osc1 only, 1.0=Osc2 only)**
8. ~~LFO scope: per-voice or global?~~ → **RESOLVED: per-voice, free-running** (confirmed by Nabeel)
9. **Does Nabeel know HTML/CSS?** Somewhat — he's practically new to coding. Relies on Antigravity IDE. He owns `UI/` only; never touches `Source/`.
10. **Plugin title:** "Solace Soft Synth" (Figma) or "Solace Synth"? Confirm with designer
11. **Portamento:** Polyphonic glide is complex — firmly V2
12. **Preset system:** APVTS is the foundation. **NEXT FOCUS:** Implementation of `.solace` preset files and browser UI.

---

## 🔗 Key Links

- [Audio Plugin Coder (APC)](https://github.com/Noizefield/audio-plugin-coder)
- [pamplejuce JUCE CMake template](https://github.com/sudara/pamplejuce)
- [synth-plugin-book (MIT)](https://github.com/hollance/synth-plugin-book)
- [awesome-juce curated list](https://github.com/sudara/awesome-juce)
- [Helm - architecture reference](https://github.com/mtytel/helm)
- [Odin2 - modern JUCE reference](https://github.com/TheWaveWarden/odin2)
- [Wavetable by FigBug (BSD-3-Clause)](https://github.com/FigBug/Wavetable)
- [JUCE pricing/licensing](https://juce.com/get-juce/)

---
