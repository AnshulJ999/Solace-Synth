# Phase 8 — V1 Release Roadmap

**Created:** 2026-03-21
**Status:** ACTIVE
**Prerequisite:** Merge dev-anshul → main (Phases 6+7 complete)

> This plan covers everything between "working synth" and "shippable V1 product."
> Items are ordered by priority. Each item is self-contained — complete one before starting the next.

---

## 8.1 — Quick Wins (low effort, high polish)

### 8.1a — App Icon (ICO) — ✅ DONE (prior session)

ICON_BIG/ICON_SMALL wired to `Assets/icon.png` in CMakeLists.txt.

### 8.1b — DSP Micro-Optimizations (Jules PR #13 + #7)

**Goal:** Two zero-risk performance improvements.

**Change 1 — `exp2` (from Jules PR #13):** ✅ DONE (already in codebase)
All `std::pow(2.0, x)` replaced with `std::exp2(x)` in SolaceOscillator.h and SolaceVoice.h.

**Change 2 — masterVolume pointer caching (from Jules PR #7):** ✅ DONE (2026-03-21)
All three processBlock parameter lookups cached: `cachedMasterVolume`, `cachedMasterDistortion`, `cachedVoiceCount`.

**Status:** Both changes complete. No remaining effort.

---

## 8.2 — Preset System — ✅ DONE (2026-03-21)

**Goal:** Users can save, load, browse, and manage named presets. Ships with factory presets.

### Implementation (actual)

**File format:** `.solace` XML files with flat parameter list:
```xml
<?xml version="1.0"?>
<SolacePreset name="Fat Bass" author="Solace" version="1">
  <Param id="osc1Waveform" value="1"/>
  <Param id="filterCutoff" value="800"/>
  <!-- ... all 36 APVTS params ... -->
</SolacePreset>
```

**Storage locations:**
- Factory presets: `.solace` files in `Assets/Presets/Factory/`, embedded via `juce_add_binary_data()` (`SolaceFactoryPresetData` namespace). Editable XML — rebuild embeds changes.
- User presets: `Documents/Solace Synth/Presets/User/` (created on first save)

### C++ Side

**`SolacePresetManager` (Source/SolacePresetManager.h/.cpp):**
- `saveUserPreset(name)`, `loadPreset(index)`, `loadPresetByName(name)`
- `renameUserPreset(index, newName)`, `deleteUserPreset(index)`
- `loadNextPreset()`, `loadPreviousPreset()`, `resetToDefaults()`
- `scanPresets()` / `rebuildPresetList()` — iterates BinaryData + user directory
- `parsePresetXml()` — parses `.solace` XML from string (for embedded factory presets)
- `saveToState()` / `restoreFromState()` — persists last preset name + isFactory in ValueTree

**Bridge:** 8 JS→C++ functions + 4 C++→JS events (see `bridge.js` header comment).

### JS Side

**`UI/components/preset-browser.js`:**
- Hierarchical dropdown: Default (standalone) → Factory → User
- Save / Save As / Rename / Delete with modal dialogs
- Modified indicator (`*` suffix), prev/next navigation
- Management buttons disabled for factory presets

### Factory Presets (10 shipped)

Default, Fat Bass, Pluck, Warm Pad, Lead, Supersaw, Keys, Brass, Sub, Dirty Lead.

### Bug fixes applied (13 total)
Rename file safety, isModified race condition (deferred callAsync), Default naming, delete fallback, modal input restore, error feedback, name disambiguation, nav return values, logger init order, dead code removal, bridge header update, modal Enter-listener accumulation fix, resetToDefaults ordering (parse before modify).

---

## 8.3 — Resizable Window — ✅ DONE (2026-03-21)

Implemented differently than originally planned. Instead of `transform: scale()` + fixed pixels, kept the existing `clamp()` CSS which adapts to viewport changes naturally.

**What was done:**
- C++: `setResizable(true, true)` + `setResizeLimits(640, 360, 2560, 1440)` in PluginEditor
- Window size persistence: saved to APVTS ValueTree via `resized()` with `constructionComplete` guard, restored in editor constructor
- Default: 1280×720 (matches original)

**Why not transform:scale():** Replacing clamp() values with Figma pixel values + scale produced a worse-looking UI (truncated text, wrong proportions). The clamp() approach works well and required minimal changes.

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

- [x] All factory presets created and tested — 10 factory presets shipped (2026-03-21)
- [x] App icon (ICO) in place — DONE (8.1a)
- [x] Plugin window resizable and scaling correctly — DONE (clamp CSS + setResizable)
- [x] `PLUGIN_CODE` changed to `"Slce"` — DONE
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
8.1a  App Icon ──────────────────── ✅ DONE
8.1b  exp2 + masterVol cache ───── ✅ DONE
8.2   Preset System ────────────── ✅ DONE
8.3   Resizable Window ─────────── ✅ DONE
8.4   CI/CD (GitHub Actions) ───── 1-2 hours
8.5   Unit Tests ───────────────── 2-3 hours
8.6   UI Polish items ──────────── ongoing (fader heights, logo, alignment done)
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
