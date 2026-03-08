# Solace Synth — Project Memory

**Created:** 2026-03-08
**Last Updated:** 2026-03-09 (Phase 4.1 logging pass reviewed - rebuild blocked by running EXE)
**Status:** Active - Phase 4 bridge handshake verified. Slider CSS fix + FileLogger + debug panel added. Close running standalone EXE, rebuild, then retest slider behavior with logs.

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
- Repo status: Phase 4.1 debugging/logging in progress — bridge handshake verified, slider interaction still under investigation
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
| Design iteration speed | Slow | Fast | Slow |
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
- **Log file:** `%TEMP%\SolaceSynth\SolaceSynth.log` (FileLogger, works in both Debug and Release)
- **JS debug panel:** Visible in the UI, shows timestamped bridge messages
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
- [/] **Phase 4: WebView Integration** — in progress (2026-03-09)
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
    9. **JS bridge API mismatch (root cause of "Connecting to engine..." stuck):**
       - bridge.js called `window.__JUCE__.backend.getNativeFunction()` which doesn't exist
       - JUCE 8's low-level backend only exposes `emitEvent`/`addEventListener`
       - `getNativeFunction` is in JUCE's ES module index.js, not on the backend object
       - **Fix:** rewrote bridge.js to use correct `__juce__invoke` event pattern
       - Also added try/catch in main.js init to surface errors in status bar
  - **Production note:** UI files served from disk via `SOLACE_DEV_UI_PATH`. For release, must embed via `juce_add_binary_data()`
  - **Status: bridge handshake verified. Slider CSS fixed (horizontal). Logging added. Needs 1 bug fix then rebuild.**
  - **Latest changes (not yet rebuilt):**
    - Replaced vertical slider (writing-mode CSS hack) with standard horizontal slider (no rebuild needed)
    - Added visible debug log panel in UI (shows timestamped bridge messages, color-coded: blue=JS, green=C++, red=error)
    - Added FileLogger to PluginProcessor (writes to %TEMP%\SolaceSynth\SolaceSynth.log, 512KB cap, works in Release)
    - Added Logger::writeToLog calls to all bridge handlers in PluginEditor.cpp
    - JS main.js has full diagnostic logging at every boundary
  - **⚠️ Bug to fix before next rebuild:** `parameterChanged()` in PluginEditor.cpp calls `Logger::writeToLog()` BEFORE `callAsync` — this is disk I/O on the audio thread (real-time safety violation, causes xruns). Fix: move the log call inside the callAsync lambda body (message thread). 1-line change.
  - **Future concern (pre-release):** FileLogger uses `setCurrentLogger()` globally — if two plugin instances are open simultaneously, second instance overwrites first's logger pointer. Not a concern for solo dev testing.
  - **Latest review note (2026-03-09):**
    - Fresh rebuild reached final link step but failed because `build/SolaceSynth_artefacts/Release/Standalone/Solace Synth.exe` was still open/locked by a running process
    - Current logging is for debugging only; `Logger::writeToLog` is also called from the APVTS listener path, so it should be removed or reduced before real DSP/performance work

### Next Up
- [ ] Rebuild and verify Phase 4 (slider works + logs visible)
- [ ] Phase 5: First Sound (single oscillator responds to MIDI)
- [ ] GitHub Actions CI + pluginval (automated testing)

### Pending — Spec Gaps (need decisions before DSP implementation)
- [ ] **Define full LFO target list** and velocity mod target list
- [ ] **Waveform list confirmed** (suggest: Sine, Saw, Square, Triangle, Noise)
- [ ] **Filter HP decision** — include in V1 or defer to V2?

### Pending — Implementation (after initialization)
- [ ] Prototype: single oscillator → ADSR → filter → standalone output
- [ ] Implement polyphonic voice architecture
- [ ] Implement LFO modulation routing
- [ ] Implement unison engine

---

## ❓ Open Questions

1. ~~Final project name?~~ → **RESOLVED: Solace Synth**
2. ~~WebView vs native JUCE?~~ → **RESOLVED: WebView confirmed**
3. ~~JUCE license?~~ → **RESOLVED: Starter (free commercial), project license deferred**
4. ~~JUCE inclusion method?~~ → **RESOLVED: CMake FetchContent**
5. **Does friend know HTML/CSS?** Critical for WebView collaboration — not yet confirmed
6. **"Computer-friendly UI" — does it imply an onscreen keyboard?**
7. **Filter HP in V1?** Strongly recommend yes — trivial to add with State Variable Filter
8. **Portamento:** Polyphonic glide is complex — keep firmly in V2
9. **Preset system architecture:** Use `AudioProcessorValueTreeState` from day one — preset load/save is almost free if designed in early
10. **Neural DSP / BIAS FX 2 precedent noted:** BIAS FX 2 uses HTML/CSS WebView for its premium UI — validates this approach for commercial-quality results

---

## 🔗 Key Links

- [Audio Plugin Coder (APC)](https://github.com/Noizefield/audio-plugin-coder)
- [pamplejuce JUCE CMake template](https://github.com/sudara/pamplejuce)
- [synth-plugin-book (MIT)](https://github.com/hollance/synth-plugin-book)
- [awesome-juce curated list](https://github.com/sudara/awesome-juce)
- [Helm — architecture reference](https://github.com/mtytel/helm)
- [Odin2 — modern JUCE reference](https://github.com/TheWaveWarden/odin2)
- [Wavetable by FigBug (BSD-3-Clause)](https://github.com/FigBug/Wavetable)
- [JUCE pricing/licensing](https://juce.com/get-juce/)

---
