# Solace Synth — Initialization Plan

## Goal

Set up the Solace Synth repository from empty to a working JUCE 8 + WebView plugin skeleton that compiles, loads in a DAW, and produces a test tone — proving the full pipeline works before building any real features.

---

## Key Decision Explanations

### 1. Git Submodules vs CMake FetchContent — How to Include JUCE

JUCE is a ~100MB C++ framework. Your project needs it to compile. The question is: **how does your repo reference JUCE?**

#### Option A — Git Submodule

A **git submodule** is Git's way of embedding one repo inside another. Think of it as a "link" to a specific commit of the JUCE repo that lives inside your project.

```
Solace-Synth/
└── libs/
    └── JUCE/     ← This is a git submodule pointing to JUCE commit abc123
```

**What happens in practice:**
- You run `git submodule add https://github.com/juce-framework/JUCE libs/JUCE`
- Git creates a pointer (not a copy) — your repo stores *which* JUCE commit to use
- Anyone who clones your repo runs `git submodule update --init` to download JUCE
- Your repo grows by ~100MB because of JUCE's source code

**Pros:**
- ✅ Everyone gets the **exact same JUCE version** — reproducible builds
- ✅ Works offline once cloned — no network needed at build time
- ✅ Industry standard — pamplejuce, Surge XT, Helm, Odin2 all use submodules
- ✅ You can pin to a specific JUCE release (e.g., JUCE 8.0.4) and upgrade deliberately

**Cons:**
- ⚠️ Repo is ~100MB heavier
- ⚠️ `git clone` without `--recursive` forgets the submodule (confusing for beginners)
- ⚠️ Submodule commands (`git submodule update`, `git submodule init`) are a little clunky

#### Option B — CMake FetchContent

**CMake FetchContent** downloads JUCE at build time from the internet. No submodule needed.

```cmake
# In CMakeLists.txt:
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4)
FetchContent_MakeAvailable(JUCE)
```

**What happens in practice:**
- You add 4 lines to CMakeLists.txt — no submodule commands at all
- When you run CMake for the first time, it downloads JUCE into a build cache folder
- Your repo stays tiny (no JUCE source committed)

**Pros:**
- ✅ Repo stays small
- ✅ No submodule commands to learn
- ✅ One less thing to manage in Git

**Cons:**
- ⚠️ Needs internet on first build — can't build offline
- ⚠️ If GitHub changes/removes the JUCE tag, your build breaks
- ⚠️ Each contributor downloads JUCE separately (can be slow)
- ⚠️ Slightly less standard for audio plugins (but works fine)

#### ✅ Recommendation: **FetchContent** (Option B)

**Why:** Given that you are unfamiliar with Git workflows and submodules would add confusion, FetchContent is simpler. It's 4 lines in a file vs. learning submodule commands. The "needs internet" downside doesn't matter since you'll always have internet when building. Pamplejuce actually supports *both* approaches, so this isn't locking us out of anything.

> [!NOTE]
> If you later decide you want submodules (e.g., for offline builds or CI speed), switching is a one-time 5-minute change. Not a permanent decision.

---

### 2. Licensing — Do We HAVE To Use AGPLv3?

**Short answer: No, you don't have to use AGPLv3 for your project.**

Here's the full picture:

JUCE 8 is available under **two** licensing models:

| How you use JUCE | Your project's required license |
|---|---|
| **JUCE's open-source GitHub code** (free, AGPLv3) | Your project *must* use a GPL-compatible license (AGPLv3, GPL-3.0, etc.) if you **distribute** the compiled binary |
| **JUCE's commercial license** (Starter = free for < $50K revenue) | Your project can use **any license you want** — MIT, proprietary, whatever |

**The key insight most people miss:** JUCE's "Starter" commercial license is **free** for individuals/companies under $50K annual revenue. You don't need to pay anything. You just need to agree to their EULA.

So the actual options are:

| Approach | Cost | Your project license | Restriction |
|---|---|---|---|
| Use JUCE AGPLv3 (GitHub) | $0 | Must be AGPL/GPL-compatible | Copyleft — anyone forking you must also open-source |
| Use JUCE Starter (Free commercial) | $0 | Whatever you want (MIT, GPL, proprietary) | Revenue must stay under ~$50K/year |

> [!IMPORTANT]
> **Recommendation:** Use the JUCE **Starter (Free) commercial license** and license your project however you want. You're making a free synth — you'll never hit the revenue cap. This gives you maximum flexibility: you can use MIT, GPL-3.0, or even keep it proprietary. You don't have to decide your own license today.

**For now: Keep the repo private, don't add a LICENSE file yet.** Decide the license when you're ready to go public. Nothing in the development process changes based on this choice.

---

### 3. Using Pamplejuce — What Exactly Happens

Pamplejuce isn't a dependency that stays in your project. It's a **template** — like a cookie cutter. You use it once to stamp out your project structure, then it's done.

**What we take from pamplejuce:**
- The `CMakeLists.txt` structure (how to define a JUCE plugin project with CMake)
- The [.gitignore](file:///G:/GitHub/Solace-Synth/.gitignore) (what build artifacts to exclude)
- The GitHub Actions workflow (auto-builds on push — optional)

**What we customize:**
- Plugin name → "Solace Synth"
- Plugin code → everything
- Plugin formats → VST3 + Standalone
- UI → our WebView approach (pamplejuce uses default JUCE editor)

**What we DON'T take:**
- pamplejuce's test setup (we won't need it initially)
- Its specific file organization (we'll use our own Source/UI split)

Effectively, I'll write a `CMakeLists.txt` **based on** pamplejuce's patterns, customized for Solace Synth. Not a copy-paste, not a fork — just informed by its well-tested patterns.

---

### 4. Claude Code's Suggestions — Unbiased Review

| Claude Code Suggestion | Assessment |
|---|---|
| Two-phase init (scaffolding → JUCE setup) | ✅ **Good** — separating concerns makes sense |
| Folder structure (Source/ + UI/ + libs/) | ✅ **Good** — clean separation aligns with WebView approach |
| "Initialize from pamplejuce template" | ⚠️ **Partially agree** — don't literally clone/merge their repo; write CMakeLists.txt based on their patterns instead. Merging repos is messy. |
| Steps 3→4→5 (Hello World → Oscillator → WebView) | ✅ **Good ordering** — prove DSP pipeline first, then prove UI pipeline |
| Use git submodule for JUCE | ⚠️ **Disagree for this team** — FetchContent is simpler for non-Git-experts. Same result, less Git friction. |
| Need CONTRIBUTING.md upfront | ❌ **Premature** — it's two people. Write it when/if you go public. |
| AGPLv3 as the project license | ⚠️ **Not necessary** — JUCE Starter (free commercial) lets you choose any license |

---

## Proposed Changes

### Phase 0 — Dev Environment (User Action)

Before any files are created, you need:
1. **Visual Studio 2022** — "Desktop development with C++" workload
2. **CMake 3.22+** — from [cmake.org](https://cmake.org/download/), the Windows x64 installer
3. **Git** — you already have this

> [!IMPORTANT]
> Nothing below can happen until VS2022 + CMake are installed. Let me know when you have them.

---

### Phase 1 — Repo Scaffolding

#### [MODIFY] [.gitignore](file:///G:/GitHub/Solace-Synth/.gitignore)
Replace the current thin C++ gitignore with a comprehensive one covering: build/ output, CMake cache files, Visual Studio artifacts (.vs/, .suo, .user), VST3/Standalone output binaries, JUCE generated files, macOS .DS_Store, thumbs.db, IDE project files. Based on pamplejuce's proven [.gitignore](file:///G:/GitHub/Solace-Synth/.gitignore).

#### [MODIFY] [README.md](file:///G:/GitHub/Solace-Synth/README.md)
Proper project README: project name, one-line description, feature list (from V1 scope), current status, build instructions placeholder, license TBD note.

#### [NEW] docs/project-memory.md
The consolidated project memory (cleaned up from the Synth-Project version — no duplicates). This becomes the living context document for all AIs working on this project.

---

### Phase 2 — JUCE Project Setup

#### [NEW] [CMakeLists.txt](file:///G:/GitHub/Solace-Synth/CMakeLists.txt)
The core build file. Uses CMake FetchContent to get JUCE 8, defines the plugin target:
- Plugin name: "Solace Synth"
- Plugin formats: VST3, Standalone
- JUCE modules: `juce_audio_basics`, `juce_audio_devices`, `juce_audio_formats`, `juce_audio_plugin_client`, `juce_audio_processors`, `juce_audio_utils`, `juce_core`, `juce_data_structures`, `juce_dsp`, `juce_events`, `juce_graphics`, `juce_gui_basics`, `juce_gui_extra`
- Links the `Source/` directory

#### [NEW] Source/PluginProcessor.h and Source/PluginProcessor.cpp
The audio engine entry point. Initial version: passes audio through unchanged, accepts MIDI (does nothing with it yet). Defines `AudioProcessorValueTreeState` with a single test parameter (master volume).

#### [NEW] Source/PluginEditor.h and Source/PluginEditor.cpp
The plugin window. Initial version: shows a simple label "Solace Synth — WebView UI coming soon". Will be replaced with WebView in Phase 4.

---

### Phase 3 — Hello World Verification (First Build)

After Phase 2 files are created, you would:
1. Open a terminal in the repo root
2. Run: `cmake -B build -G "Visual Studio 17 2022"`
3. Run: `cmake --build build --config Release`
4. Find the Standalone executable in `build/` and run it
5. **Success = a window opens with the label text. No crashes.**

---

### Phase 4 — WebView Integration (After Hello World Works)

#### [MODIFY] Source/PluginEditor.cpp
Replace the label with JUCE 8's `juce::WebBrowserComponent` loading a local HTML file from `UI/index.html`. Implement the C++↔JS message relay.

#### [NEW] [UI/index.html](file:///G:/GitHub/Solace-Synth/UI/index.html)
Basic HTML page: "Solace Synth" title, a test slider, JS that sends/receives values to/from C++.

#### [NEW] [UI/styles.css](file:///G:/GitHub/Solace-Synth/UI/styles.css)
Minimal styling: background color, font, slider appearance.

#### [NEW] [UI/bridge.js](file:///G:/GitHub/Solace-Synth/UI/bridge.js)
JavaScript side of the C++↔JS communication relay.

---

### Phase 5 — First Sound (Single Oscillator)

#### [NEW] Source/DSP/Oscillator.h and Source/DSP/Oscillator.cpp
A basic sawtooth oscillator class. Monophonic for now.

#### [MODIFY] Source/PluginProcessor.cpp
Wire the oscillator to MIDI note-on. When a MIDI note arrives, the oscillator plays. One voice, one waveform, no envelope yet.

**Success = press a key on a MIDI keyboard (or virtual keyboard) and hear a sound.**

---

## Verification Plan

### Phase 3 Verification — Hello World
```
1. Open terminal in G:\GitHub\Solace-Synth
2. Run: cmake -B build -G "Visual Studio 17 2022"
3. Run: cmake --build build --config Release
4. Expected: Build completes with no errors
5. Run: build\SolaceSynth_artefacts\Release\Standalone\SolaceSynth.exe
6. Expected: A window opens showing "Solace Synth — WebView UI coming soon"
7. Close the window. No crashes.
```

### Phase 4 Verification — WebView
```
1. Rebuild: cmake --build build --config Release
2. Run the Standalone exe
3. Expected: The plugin window shows an HTML page with the "Solace Synth" title and a slider
4. Move the slider in the HTML UI
5. Expected: The C++ console/debug output shows the parameter value changing
```

### Phase 5 Verification — First Sound
```
1. Rebuild: cmake --build build --config Release
2. Run the Standalone exe
3. Click the on-screen keyboard (if present) or connect a MIDI keyboard
4. Play a note
5. Expected: A sawtooth tone plays at the correct pitch
6. Release the note
7. Expected: Sound stops (no envelope yet, so abrupt cutoff is expected)
```

> [!NOTE]
> All verification is manual at this stage. Automated testing (pluginval, unit tests) can be added later once there's actual DSP to test. Running pluginval on a plugin with no audio processing is not meaningful.

---

## Summary of Decisions Needing Your Approval

1. **FetchContent over submodules** for including JUCE — simpler for your team
2. **JUCE Starter (free commercial license)** instead of AGPLv3 — gives you license flexibility
3. **No LICENSE file yet** — decide when going public
4. **Phase ordering:** Scaffolding → JUCE setup → Hello World → WebView → First Sound
5. **No CONTRIBUTING.md yet** — premature for a two-person private repo
6. **Project memory consolidation** — clean up duplicate tail in [synth-project-memory.md](file:///G:/GitHub/Personal-Stuff/Synth-Project/synth-project-memory.md) and copy the clean version into `Solace-Synth/docs/`
