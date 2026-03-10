# Solace Synth — Project Memory

**Created:** 2026-03-08
**Last Updated:** 2026-03-11 (Phase 6.7 Unison -- code complete. UnisonVoice[8] array, constant-power pan, equal-power level normalisation, stereo output. Awaiting build + listening test.)
**Status:** Active -- Phases 0-5 complete + Phases 6.1-6.7 code complete. Next: build + test 6.7, then Phase 6.8 (Voicing Parameters).

For UI: Up till phase 7.2 was done. UI prototype works, but needs lots of polishing and tweaks and further refinement.

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
- Repo status: **Phases 0-5 complete** — working polyphonic sine synth with WebView UI, bridge, MIDI keyboard
- The "SS" monogram logo from the original mockup works well with Solace Synth initials

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
- **LFO target list:** Full list not defined. Typical targets: Filter Cutoff, Filter Resonance, Osc 1 Pitch, Osc 2 Pitch, Osc 1 Level, Osc 2 Level, Amp Level, Master Level
- **Velocity mod target list:** Same gap — typical targets: Amp Attack, Amp Level, Filter Cutoff
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
  - **Production note:** UI files served from disk via `SOLACE_DEV_UI_PATH`. For release, must embed via `juce_add_binary_data()`

### Phase 6.1 — Amp ADSR (2026-03-09) — CODE WRITTEN, PENDING BUILD VERIFICATION

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

**⚠️ Build verification required:** Run `cmake --build build --config Release` and verify:
1. Build succeeds cleanly (no new warnings).
2. Standalone launches.
3. Notes have musical shape (Attack/Decay/Sustain/Release instead of organ gate).
4. Soft key press → quieter than hard press (velocity sensitivity).
5. After note-off, sound fades over ~0.3s (default release).

### Next Up
- [x] **Phase 5: First Sound** — COMPLETE (2026-03-09)
  - `Source/DSP/SolaceSound.h` — `SynthesiserSound` tag class (all notes/channels)
  - `Source/DSP/SolaceVoice.h` — `SynthesiserVoice` with sine oscillator, velocity scaling, exponential tail-off on release
  - `PluginProcessor.h` — `juce::Synthesiser synth` + `juce::MidiKeyboardState keyboardState` added; `juce_audio_utils` included
  - `PluginProcessor.cpp` — 8 SolaceVoice + 1 SolaceSound in constructor; `prepareToPlay` sets sample rate; `processBlock`: clear → `keyboardState.processNextMidiBuffer()` → `synth.renderNextBlock()` → apply masterVolume
  - `PluginEditor.h/cpp` — `juce::MidiKeyboardComponent midiKeyboard` added as 80px strip at bottom of window; auto-grabs keyboard focus 500ms after launch
  - **Verified:** polyphonic sine tones play correctly. Mouse click on piano strip ✅. Computer keyboard (A/S/D/F/etc.) ✅. Polyphony ✅. Volume slider controls output level ✅.
  - **Architecture note:** MidiKeyboardState lives in Processor (audio thread access). Editor holds MidiKeyboardComponent (UI thread). Cross-thread: documented as safe by JUCE.
- [~] **Phase 6: Core synth shaping** — IN PROGRESS
  - [x] **6.1 Amp ADSR** — code written 2026-03-09, pending build verification
  - [ ] 6.2 Oscillator waveforms + Osc1 tuning
  - [ ] 6.3 Filter (LadderFilter LP24)
  - [ ] 6.4 Filter Envelope
  - [ ] 6.5 Second Oscillator + Osc Mix
  - [x] 6.6 LFO (3 targets, per-voice free-running) — code complete, pending build + listening test
  - [ ] 6.7 Unison
  - [ ] 6.8 Voicing params
  - [ ] 6.9 Master Distortion
- [ ] GitHub Actions CI + pluginval (automated testing)

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

### ⚠️ SVG Icon Status — All Current Icons Are Placeholders
`UI/assets/icons/` was created manually (2026-03-09). **ALL icons are hand-coded SVGs, not exported from Figma.**
The Figma MCP (`figma-download_figma_images` tool) was called previously but did NOT write files to disk — it returns metadata only; actual file creation was never verified. This means:
- `logo.svg` — placeholder SS monogram, not the real Figma logo
- `waveform-sine-icon.svg` — hand-coded path, NOT Figma export
- `waveform-square-icon.svg` — hand-coded path, NOT Figma export
- `waveform-sawtooth-icon.svg` — placeholder, Nabeel needs to provide
- `waveform-triangle-icon.svg` — placeholder, Nabeel needs to provide
- `waveform-sh-icon.svg` — placeholder LFO only
- `chevron-back/forward.svg`, `btn-left/right/menu.svg`, `arrow-dropdown.svg` — simple geometric placeholders

**To get the real icons**, Nabeel or Anshul must manually export from Figma UI and drop in `UI/assets/icons/`.
The Figma MCP cannot write files — it only returns data. AI agents cannot auto-download assets from Figma.

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
- [ ] Phase 6.7: Unison (with level normalization)
- [ ] Phase 6.8: Voicing params (voice count, velocity mod targets)
- [ ] Phase 7: Full Figma UI implementation
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
- [ ] Attack fade-in (needs APVTS param set via UI — deferred until UI bridge connected)
- [ ] Release fade-out (same)
- [ ] Tail length: release audible after DAW transport stop (requires Reaper test)
- [ ] Velocity range (requires MIDI keyboard or bottom-of-key click)

#### Phase 6.2 — Waveforms + Osc1 Tuning
- [ ] Sine, Saw, Square, Triangle each sound distinct
- [ ] `osc1Octave` = +1 → pitch doubles; -1 → pitch halves
- [ ] `osc1Transpose` = +12 → same as +1 octave; +7 → perfect fifth
- [ ] `osc1Tuning` = +100 cents → slightly sharp; -100 → slightly flat
- [ ] Waveform switch mid-note: no click or crash

#### Phase 6.3 — Filter
- [ ] Sweep cutoff 20kHz → 200Hz (LP24) → progressively darker
- [ ] Max resonance → self-resonance at cutoff
- [ ] HP12 mode → bass disappears
- [ ] Multiple simultaneous notes filter independently (no cross-voice artifacts)
- [ ] Cutoff at 20Hz → near silence, no crash, no NaN

#### Phase 6.4 — Filter Envelope
- [ ] Fast A, med D, S=0, depth=+1.0 → classic pluck
- [ ] depth=-1.0 → inverted pluck (filter closes on hit)
- [ ] depth=0.0 → no envelope effect
- [ ] Cutoff stays in [20, 20000 Hz] at all depths (no NaN)

#### Phase 6.5 — Osc2 + Mix
- [ ] `oscMix`=0.0 → Only Osc1 audible
- [ ] `oscMix`=1.0 → Only Osc2 audible
- [ ] `oscMix`=0.5 + `osc2Tuning` offset → beating / chorus thickness

#### Phase 6.6 — LFO
- [ ] Target=FilterCutoff, slow rate → wah sweep
- [ ] Target=Osc1Pitch, small amount → vibrato
- [ ] Target=AmpLevel → tremolo
- [ ] Hold chord → per-voice LFO de-sync produces organic shimmer

#### Phase 6.7 — Unison
- [ ] `unisonCount`=4, `detune`=20 cents → thick supersaw
- [ ] `unisonSpread`=1.0 → wide stereo on headphones
- [ ] Count=1 vs count=8: approximately equal perceived loudness (equal-power normalisation)

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
Nabeel does **not** need to build the project. UI files are served from disk at runtime:
1. Edit files in `UI/` → Save
2. Close + reopen the plugin window (or refresh WebView)
3. Changes visible immediately — no C++ compile needed

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
12. **Preset system:** APVTS is already the foundation — save/load is almost free when ready

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

