# Phase 7 — UI Master Plan
*(Merged from: Phase 7 — UI Roadmap.md + Phase-7.4-Pixel-Perfect-Audit.md)*
*(Last updated: 2026-03-11)*

> **For future AI agents: always read this file in full before touching any UI code.
> The two source plans may contradict each other; this document is the authoritative merge.
> Anything marked ⚠️ PENDING requires explicit approval from Anshul before implementing.**

---

## Current State (as of 2026-03-09 rev 2)

| Area | Status |
|------|--------|
| WebView2 + bridge | ✅ Working (`setParameter`, `parameterChanged`, `uiReady`, `log`) |
| CSS design system (`styles.css`) | ✅ Phase 7.1 complete. Figma-exact tokens, responsive clamp() sizes |
| Layout scaffold (`index.html`) | ✅ Phase 7.2 complete. Two-row section grid, all controls present |
| JS control binding (`main.js`) | ✅ Phase 7.2 complete. All params bound; correct LFO enum ordering |
| SVG assets (`UI/assets/icons/`) | ⚠️ All icons are **hand-coded placeholders**. Not Figma exports (see below) |
| Component JS classes | ❌ Phase 7.3 — next step |
| Pixel-perfect Figma fidelity | ❌ Phase 7.4 — after components |

---

## Firm Decisions (do not revisit without Anshul)

| Item | Decision |
|------|----------|
| **Fonts** | `MuseoModerno` (title only) + `Jura` (all other text). NATS dropped — Nabeel's call: "too many fonts will make it look messy." |
| **Controls** | Vertical faders for all continuous params. Arrow selectors (`< value >`) for discrete/enum. Dropdowns for target selectors. No rotary knobs. |
| **Keyboard** | JUCE native `MidiKeyboardComponent` for V1. CSS keyboard is a future exploration only — do not build in V1. |
| **filterEnvDepth** | Hidden in HTML (`hidden` attr on `#filter-env-depth-fader`). **NOT in Figma design.** Do not unhide without Nabeel explicitly adding it to the design. |
| **unisonDetune / unisonSpread** | No UI controls. Engine-only for V1. Pending design decision. |
| **Waveform selector** | Both click-to-cycle AND explicit `< >` arrow buttons. Gemini's 7.4 plan correctly identified that Figma shows the waveform with flanking arrows. Implement both behaviours. |
| **Plugin title** | "Solace Soft Synth" — confirmed from Figma text node |

---

## SVG Asset Status ⚠️

All icons in `UI/assets/icons/` are hand-coded geometric SVGs. None are exported from Figma.

| Icon | Status |
|------|--------|
| `logo.svg` | Placeholder SS monogram — not the real Figma logo |
| `waveform-sine-icon.svg` | Hand-coded orange curve — close but not Figma export |
| `waveform-square-icon.svg` | Hand-coded — close |
| `waveform-sawtooth-icon.svg` | Placeholder — **Nabeel must provide** |
| `waveform-triangle-icon.svg` | Placeholder — **Nabeel must provide** |
| `waveform-sh-icon.svg` | LFO S&H placeholder |
| Chevrons, menu, arrows | Simple geometric shapes — acceptable for now |

**Figma MCP cannot write files to disk.** Real icons must be manually exported from Figma and dropped in `UI/assets/icons/` by Nabeel or Anshul.

---

## Pending Design Confirmations ⚠️

These require Nabeel or Anshul confirmation before implementing:

| Item | Question | Source |
|------|----------|--------|
| **Fader vertical labels** | Gemini (7.4) says Figma shows labels printed vertically alongside the track, reading bottom-to-top. Confirm before rewriting label layout. | 7.4 audit |
| **Track fill from bottom** | Gemini says Figma tracks fill dark grey from the bottom up to the thumb position. Confirm before rewriting track CSS. | 7.4 audit |
| **Tick marks** | Gemini says Figma has ruler tick marks along the track. Confirm before adding. | 7.4 audit |
| **LFO + velocity mod target list** | Vision Document (Nabeel's answer 5) gives a DIFFERENT list from what is currently in `main.js`. See section below. Must be resolved before DSP is wired. | Vision Doc |
| **Pitch Bend / Mod Wheel** | Panel confirmed in Figma left of keyboard. MIDI pitch-bend wiring TBD. Mod Wheel: Nabeel wants it configurable by right-click → map to any param. | Vision Doc Q7 |
| **Osc Mix Figma anomaly** | Gemini says Figma Osc Mix has NO numeric value below it — "Osc 2" vertical text top half, "Osc 1" vertical text bottom half. Confirm before removing the value display. | 7.4 audit |

---

## LFO / Velocity Mod Target List — MISMATCH ⚠️

**Current code** (`main.js`, confirmed against synth-project-memory line 397):
```
None(0), FilterCutoff(1), Osc1Pitch(2), Osc2Pitch(3), Osc1Level(4),
Osc2Level(5), AmpLevel(6), FilterResonance(7)
```

**Vision Document** (Nabeel's answer 5 — authoritative):
```
1. Oscillator pitch (both)
2. Oscillator tuning
3. Amplifier Attack
4. Filter Cutoff
5. Filter Resonance
6. Distortion
7. Master volume
8. Oscillator mix
```
Vision Document also says: "we can have the same list for velocity modulation targets."

**Action required:** Before any further DSP LFO wiring, align `main.js` enum, `synth-project-memory.md`, and eventually `PluginProcessor.cpp` with the Vision Document list. This is a deliberate pending task — do not implement until Anshul confirms timing.

---

## Phase 7.3 — Component Modularization ← NEXT STEP

**Goal:** Break the monolithic `index.html` + `main.js` into proper JS component classes so each control type is owned by a reusable module. The HTML becomes a thin declarative shell.

### Target file structure

```
UI/
├── index.html              ← thin: mount points only, no inline control markup
├── styles.css              ← unchanged: design system
├── bridge.js               ← unchanged
├── main.js                 ← thin: init + component instantiation only
└── components/
    ├── fader.js            ← class Fader(paramId, config)
    ├── arrow-selector.js   ← class ArrowSelector(paramId, options, config)
    ├── waveform-selector.js← class WaveformSelector(paramId, waveforms)
    └── dropdown.js         ← class Dropdown(paramId, options, config)
```

### Component contract

Each component class must:
1. Accept a `paramId` matching the APVTS ID exactly
2. Create its own DOM and append to a provided container
3. Register its own `SolaceBridge.onParameterChanged` listener for C++ → UI sync
4. Send `SolaceBridge.setParameter(paramId, value)` on user interaction
5. Expose a `setValue(v)` method for external sync (e.g., preset load)
6. Be self-contained — no global state, no jQuery, no framework

### `fader.js`
- Config: `{ label, min, max, step, defaultValue, formatFn, sizeClass }`
- `sizeClass`: `"fader--standard"`, `"fader--big"`, or `"fader--small"`
- Once Nabeel confirms: add vertical label and dynamic track fill

### `arrow-selector.js`
- Config: `{ label, options: [{value, label}], defaultIndex }`
- Renders `< label/value >` with chevron buttons
- Wraps cyclically (arrow left at index 0 goes to last, and vice versa)

### `waveform-selector.js`
- Config: `{ waveforms: [{value, label, icon, alt}], defaultIndex }`
- Renders icon with flanking `< >` arrows (both arrow nav AND click-to-cycle on icon click)
- On waveform change: swaps `<img src>` and sends parameter

### `dropdown.js`
- Config: `{ label, options: [{value, label}], defaultIndex }`
- V1: click cycles through options (same as current behaviour)
- V2 (Phase 7.5): opens a real popup list above the dropdown

---

## Phase 7.4 — Pixel-Perfect Polish

After components are in place, do a structured Figma gap pass. Only implement items that have been confirmed (see Pending table above).

### Confirmed gaps (implement in 7.4)

1. **Waveform selector arrows** — add flanking `< >` arrows to waveform selector display
2. **Filter label** — Gemini says Figma reads "Filter Type -" then `< LP 24 dB >`. Add the hyphen separator.
3. **Logo** — swap placeholder SVG for real Figma export once Nabeel provides it
4. **Sawtooth + Triangle icons** — swap when Nabeel provides

### Pending confirmation (do NOT implement until confirmed)

1. **Vertical fader labels** — confirm with Nabeel
2. **Track fill from bottom** — confirm with Nabeel
3. **Tick marks** — confirm with Nabeel
4. **Osc Mix: no value readout, vertical text** — confirm with Nabeel

### Row 3 — Keyboard strip (partial)

- The JUCE native `MidiKeyboardComponent` renders in C++ at the bottom. CSS does not need a keyboard.
- Build the **Pitch Bend + Mod Wheel panel** (HTML/CSS, left of keyboard space) once Nabeel confirms the layout spec. Mod Wheel will eventually support right-click → param mapping (V2 scope).

---

## Phase 7.5 — Polish & Accessibility

After the pixel-perfect pass:

- Micro-animations: hover glow on fader thumb, smooth arrow-selector transitions
- Loading state: fade-in UI on bridge connect
- Keyboard shortcut `Ctrl+Shift+D` for debug panel — already implemented
- Verify layout at min window size (900×540px) and scales cleanly
- Embed fonts locally (`UI/assets/fonts/`) before release — remove Google Fonts `@import`
- Embed UI files via `juce_add_binary_data()` instead of disk path (required before distribution)

---

## Phase 7 — Open Decisions Tracker

| Item | Status | Owner |
|------|--------|-------|
| Sawtooth + Triangle waveform icons | ⏳ Pending Nabeel | Nabeel |
| Real logo SVG | ⏳ Pending Nabeel | Nabeel |
| Fader vertical labels (Figma confirmed?) | ⏳ Pending confirmation | Nabeel |
| Track fill from bottom (Figma confirmed?) | ⏳ Pending confirmation | Nabeel |
| Tick marks (Figma confirmed?) | ⏳ Pending confirmation | Nabeel |
| LFO target list update (Vision Doc vs. current code) | ⏳ Pending Anshul timing | Anshul |
| Pitch Bend + Mod Wheel layout spec | ⏳ Pending Nabeel | Nabeel |
| Osc Mix vertical text / no value | ⏳ Pending confirmation | Nabeel |
| Menu button contents | ⏳ Pending design | Nabeel |
| filterEnvDepth | Hidden. Not in Figma. Keep hidden. | — |
| unisonDetune / unisonSpread | No UI. Engine-only V1. | — |
| NATS font | Dropped. Use Jura. | ✅ Resolved |
| CSS keyboard | Not in V1. Future exploration. | — |

---

## What the Original Plans Cover (for reference)

- **`Phase 7 — UI Roadmap.md`** — the structural plan: layout spec, component contracts, bridge extension notes, resizability implementation. Authoritative for phase structure and component architecture.
- **`Phase-7.4-Pixel-Perfect-Audit.md`** — written by Gemini after reviewing Figma screenshots vs. the prototype. Contains useful visual gap observations (fader labels, track fill, waveform arrows, tick marks) but also incorrect instructions (unhide filterEnvDepth; wrong LFO target list). Use its observations, ignore its instructions where they contradict Anshul's confirmed decisions.
