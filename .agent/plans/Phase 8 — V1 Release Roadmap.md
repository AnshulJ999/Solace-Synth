# Phase 8 — V1 Release Roadmap

**Created:** 2026-03-21
**Status:** ACTIVE
**Prerequisite:** Merge dev-anshul → main (Phases 6+7 complete)

> This plan covers everything between "working synth" and "shippable V1 product."
> Items are ordered by priority. Each item is self-contained — complete one before starting the next.

---

## 8.1 — Quick Wins (low effort, high polish)

### 8.1a — App Icon (ICO)

**Goal:** Replace the generic JUCE icon on the standalone EXE with the Solace Synth logo.

**Steps:**
1. Convert `UI/assets/icons/SolaceSynthLogo.svg` to a multi-resolution `.ico` file (16x16, 32x32, 48x48, 256x256). Use a tool like RealFaviconGenerator, CloudConvert, or Inkscape (File → Export → Windows Icon).
2. Place the `.ico` in the project root or a new `Assets/` folder.
3. Add to `CMakeLists.txt` in the `juce_add_plugin()` call:
   ```cmake
   ICON_BIG "${CMAKE_SOURCE_DIR}/Assets/SolaceSynthIcon.ico"
   ICON_SMALL "${CMAKE_SOURCE_DIR}/Assets/SolaceSynthIcon.ico"
   ```
4. Rebuild. Verify the EXE and taskbar show the correct icon.

**Effort:** 15 minutes.

### 8.1b — DSP Micro-Optimizations (Jules PR #13 + #7)

**Goal:** Two zero-risk performance improvements.

**Change 1 — `exp2` (from Jules PR #13):**
Replace `std::pow(2.0, x)` with `std::exp2(x)` in:
- `SolaceOscillator.h` — tuning offset calculation
- `SolaceVoice.h` — LFO pitch modulation, pitch bend, velocity pitch
- Anywhere else `pow(2.0, ...)` appears in DSP code

These are mathematically identical but `exp2` is hardware-optimized for base-2.

**Change 2 — masterVolume pointer caching (from Jules PR #7):**
In `PluginProcessor.h`, add a cached pointer:
```cpp
const std::atomic<float>* cachedMasterVolume = nullptr;
```
In the constructor, after APVTS is created:
```cpp
cachedMasterVolume = apvts.getRawParameterValue("masterVolume");
```
In `processBlock()`, replace the string lookup with the cached pointer.

**Effort:** 15 minutes total. Zero risk — same output, just faster.

---

## 8.2 — Preset System

**Goal:** Users can save, load, browse, and manage named presets. Ships with factory presets.

### Architecture

**File format:** `.solace` files = APVTS XML state + metadata header.
```xml
<?xml version="1.0"?>
<SolacePreset name="Fat Bass" author="Anshul" version="1">
  <APVTS>
    <!-- JUCE APVTS XML state dump -->
  </APVTS>
</SolacePreset>
```

JUCE's `apvts.copyState().toXmlString()` gives us the APVTS state. We wrap it with name/author/version metadata.

**Storage locations:**
- Factory presets: embedded via BinaryData (ship with the plugin, read-only)
- User presets: `{user documents}/Solace Synth/Presets/` (or platform-appropriate path)

### C++ Side

**New class: `SolacePresetManager` (Source/SolacePresetManager.h/.cpp)**
- `savePreset(name, apvts)` — serialize APVTS state + metadata to `.solace` XML file
- `loadPreset(file, apvts)` — parse `.solace` file, call `apvts.replaceState()`
- `getFactoryPresets()` — return list from embedded BinaryData
- `getUserPresets()` — scan user preset directory
- `deletePreset(file)` — remove a user preset file
- `getUserPresetDirectory()` — create if needed, return path

**Bridge extension:**
- JS → C++: `savePreset(name)`, `loadPreset(index)`, `deletePreset(index)`, `getPresetList()`
- C++ → JS: `presetListChanged([{name, isFactory, path}...])`, `currentPresetChanged(name)`

### JS Side

**Preset browser panel** — could be:
- A dropdown/overlay in the header bar (where the Menu button placeholder is)
- Shows factory presets (read-only) and user presets
- Save button, rename, delete for user presets
- Current preset name displayed in the header

### Factory Presets (create 8-12)

| Name | Character | Key Settings |
|------|-----------|-------------|
| Init | Clean default | Sine, no filter env, no unison |
| Fat Bass | Thick low end | Saw, LP24 cutoff 800Hz, unison 4, detune 15 |
| Pluck | Short percussive | Saw, filter env fast A/D, sustain 0 |
| Pad | Evolving texture | Triangle+Sine, slow attack, LFO→cutoff |
| Lead | Cutting mono-ish | Square, LP24 cutoff 3kHz, resonance 0.4 |
| Supersaw | Classic trance | Saw, unison 8, detune 25, spread 0.8 |
| Keys | Electric piano feel | Sine+Triangle, medium attack, filter env |
| Brass | Synth brass | Saw, filter env medium, unison 2 |
| Sub | Deep sub bass | Sine only, LP24 low cutoff |
| Dirty Lead | Distorted | Saw, distortion 0.6, filter env fast |

**Effort:** This is the largest item. Estimate 2-3 focused sessions.

---

## 8.3 — Resizable Window (CSS Scale Refactor)

**Goal:** Plugin window can be freely resized while maintaining exact Figma proportions.

**Plan:** Already documented in `.agent/plans/phase3-scale-refactor-plan.md`. Summary:

1. Replace all CSS `clamp()`/`vh`/`vw` with fixed Figma pixel values
2. Add `transform: scale(factor)` on `.solace-root` in JS, computed from `window.innerWidth / 1440`
3. Enable `setResizable(true, false)` + `setResizeLimits(720, 512, 2880, 2048)` in C++
4. Verify dropdown popups work correctly at non-1.0 scale factors

**Prerequisite:** Design should be stable (no major layout changes pending).

**Effort:** One focused session (2-3 hours).

---

## 8.4 — CI/CD (GitHub Actions)

**Goal:** Automated Release builds on GitHub. Push a tag → get downloadable artifacts.

**Reference:** pamplejuce's `.github/workflows/` — battle-tested JUCE CI config.

### What to set up:

1. **Build workflow** (`.github/workflows/build.yml`):
   - Trigger: push to `main`, PR to `main`, manual dispatch
   - Matrix: Windows (MSVC) — start with Windows only
   - Steps: checkout → configure CMake → build Release → upload artifacts
   - Artifacts: `Solace Synth.exe` (standalone) + `Solace Synth.vst3`

2. **Release workflow** (`.github/workflows/release.yml`):
   - Trigger: push a tag like `v1.0.0`
   - Build → create GitHub Release → attach artifacts as downloadable zip

3. **Future additions:**
   - pluginval validation step
   - Unit tests (after Jules PRs #2/#3 are integrated)
   - Mac build (AU format) when ready

**Effort:** 1-2 hours to set up. Reference pamplejuce heavily.

---

## 8.5 — Unit Testing Infrastructure

**Goal:** Automated tests for DSP and bridge logic.

**Based on:** Jules PRs #2 and #3 (Catch2 + CTest).

### What to test:

| Module | Tests |
|--------|-------|
| SolaceOscillator | Output range [-1, +1], frequency accuracy, waveform identity |
| SolaceADSR | Stages (idle→attack→decay→sustain→release→idle), parameter response |
| SolaceFilter | Cutoff clamping [20, 20000], mode switching, no NaN |
| SolaceLFO | Output range [-1, +1], S&H randomness, phase diversity |
| SolaceDistortion | Passthrough at drive=0, saturation at drive=1, no NaN |
| Bridge handler | `setParameter` with valid/invalid IDs, value clamping |

### Steps:
1. Add Catch2 via FetchContent in CMake
2. Create `Tests/` directory with test files
3. Verify Jules' refactors to SolaceLogger and bridge handler are safe
4. Wire into CI workflow

**Effort:** 2-3 hours for initial setup + basic tests.

---

## 8.6 — Remaining UI Polish

These are smaller items that improve the user experience:

| Item | Description | Effort |
|------|-------------|--------|
| Ctrl+drag fine fader | Hold Ctrl while dragging for 10x precision | 30 min |
| filterEnvDepth UI | Decide: show it or keep hidden? Anshul's call | 5 min decision |
| unisonDetune/Spread UI | Add fader controls if desired | 30 min |
| Plugin title | Decide "Solace Soft Synth" vs "Solace Synth" | 1 min decision |
| Figma accuracy | Manual CSS tweaking to match Figma exactly | Varies |
| Debug panel shortcut | Ctrl+Shift+D toggle (already implemented?) | Verify |

---

## 8.7 — Pre-Release Checklist

Before tagging v1.0.0:

- [ ] All factory presets created and tested
- [ ] App icon (ICO) in place
- [ ] Plugin window resizable and scaling correctly
- [ ] `PLUGIN_CODE` changed from `"Ss01"` to a unique value
- [ ] CI builds producing clean artifacts
- [ ] README updated with installation instructions
- [ ] License chosen and applied
- [ ] Test on a clean Windows machine (no dev tools)
- [ ] Verify WebView2 runtime dependency handling
- [ ] Multi-instance logger safety (or document the limitation)
- [ ] Logging strategy decided (keep in release or gate behind flag)

---

## Order of Execution

```
8.1a  App Icon ──────────────────── 15 min   ← START HERE
8.1b  exp2 + masterVol cache ───── 15 min
8.2   Preset System ────────────── 2-3 sessions  ← MAIN WORK
8.3   Resizable Window ─────────── 2-3 hours
8.4   CI/CD (GitHub Actions) ───── 1-2 hours
8.5   Unit Tests ───────────────── 2-3 hours
8.6   UI Polish items ──────────── as needed
8.7   Pre-Release Checklist ────── final pass
```

---

## What's NOT in V1

- VST2 (legally impossible — Steinberg discontinued SDK in 2018)
- AU / CLAP / LV2 (need Mac/Linux builds — V2)
- Noise waveform, PolyBLEP anti-aliasing
- Portamento / Glide
- MIDI CC learn
- Dark theme
- Oscilloscope / spectrum analyzer
- Mod wheel right-click → param mapping
