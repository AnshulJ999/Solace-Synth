# Phase 7 — UI Roadmap

The WebView-based user interface. Transforms the placeholder slider into a production-quality synth UI built from Figma designs.

---

## Current State

| Area | Status |
|------|--------|
| WebView2 integration | ✅ Working |
| C++↔JS bridge | ✅ Working (`setParameter`, `parameterChanged`, `uiReady`, `log`) |
| Placeholder UI | ✅ masterVolume slider + debug panel |
| Figma design | 🔄 Pending export from designer friend |
| MIDI keyboard strip | ✅ JUCE native `MidiKeyboardComponent` at bottom |

---

## Design Reference

**Source of truth: Figma design (friend's file)**

The accepted design is the **second iteration** (slider/fader-based) — the first iteration (knob-based) was rejected. Key visual spec confirmed from screenshots:

- **Theme:** Light / white background, orange accent sliders — clean, minimal, modern
- **Typography:** Geometric sans-serif (confirm exact font from Figma)
- **Controls:** Vertical sliders (faders) for continuous parameters; arrow selectors (`< value >`) for discrete/enum parameters; dropdowns for target selectors
- **Logo:** "SS" monogram with orange highlight

### Confirmed Layout Sections (from screenshot)

**Top row (left to right):**
1. Oscillator 1 — Waveform selector, Octave selector, Transpose selector, Tuning fader
2. Osc Mix — one vertical crossfader (`oscMix`): 0.0 = Osc1 only, 1.0 = Osc2 only
3. Oscillator 2 — Waveform selector, Octave selector, Transpose selector, Tuning fader
4. Amplifier Envelope — Attack, Decay, Sustain, Release faders
5. Master — **Distortion fader** (masterDistortion, added Phase 6.9) + Level fader

**Bottom row (left to right):**
1. Filter — Cutoff fader, Resonance fader
2. Filter Configuration — Filter Type selector, Attack, Decay, Sustain, Release faders
3. Low Frequency Oscillator — Waveform selector, Rate fader, Amount fader, 3 LFO Target dropdowns
4. Voicing — No. of Voices selector, Unison selector, Velocity Range fader, 2 Velocity Mod Target dropdowns

**Bottom row (keyboard strip):**
5. Pitch Bend + Mod Wheel sliders (left panel)
6. MIDI Keyboard — **OPEN DECISION: CSS-drawn vs. JUCE native** (see `design-tokens.md` Row 3 section)

---

## Prerequisites

1. **Figma design** — ✅ **Extracted via Figma MCP (2026-03-09).** Full design tokens, layout, typography, and SVG assets stored in `.agent/figma/design-tokens.md` and `UI/assets/icons/`. Remaining gaps documented in Open Questions below.

2. **DSP parameters defined** — Phase 6 adds APVTS parameters as it progresses. UI can bind to any existing parameter via the bridge. Controls for parameters not yet implemented will simply have no effect until the DSP catches up.

---

## Sub-Phase 7.1 — Design System Setup

**Before building any UI pages,** establish the CSS foundation.

### Files

| Action | File | What |
|--------|------|------|
| MODIFY | `UI/styles.css` | Complete rewrite: CSS custom properties for all design tokens. Light base theme (white/near-white background, orange accent). Import correct font from Google Fonts. Define all component primitives. |
| NEW | `UI/assets/` | Directory for exported SVG icons (waveform shapes, SS logo, arrow chevrons). |

### CSS Custom Properties to Define

```css
:root {
  /* Colors — light theme (source from Figma exact hex values) */
  --color-bg: #ffffff;               /* main background */
  --color-surface: #f5f5f5;          /* section card background */
  --color-accent: #f5a623;           /* orange accent (approximate — confirm in Figma) */
  --color-accent-hover: #e09416;     /* slightly darker on hover */
  --color-text-primary: #1a1a1a;     /* main labels */
  --color-text-muted: #888888;       /* secondary labels, tick marks */
  --color-border: #e0e0e0;           /* section card borders */
  --color-slider-track: #d0d0d0;     /* fader track color */
  --color-slider-thumb: var(--color-accent); /* orange fader thumb */

  /* Typography */
  --font-family: 'Inter', sans-serif;  /* confirm from Figma */
  --font-size-label: 12px;
  --font-size-value: 11px;
  --font-size-section-header: 14px;

  /* Spacing (4px base grid) */
  --space-xs: 4px;
  --space-sm: 8px;
  --space-md: 16px;
  --space-lg: 24px;

  /* Components */
  --section-border-radius: 8px;      /* confirm from Figma */
  --section-padding: 16px;
  --slider-track-width: 4px;
  --slider-thumb-size: 18px;
}
```

> [!NOTE]
> All hex values above are approximate from the screenshot. The designer friend must provide exact values from Figma Inspect. Replace before finalizing Phase 7.1.

---

## Sub-Phase 7.2 — Layout Scaffold

Build the page structure matching the confirmed Figma layout.

### Layout (matches the Figma screenshot):

```
┌──────────────────────────────────────────────────────────────────────┐
│  [SS logo]  Solace Soft Synth                                        │
├──────────┬──────────┬──────────┬──────────────────────────┬─────────┤
│          │          │          │                          │         │
│  Osc 1   │ Osc Mix  │  Osc 2   │   Amplifier Envelope     │ Master  │
│          │          │          │                          │         │
├──────────┴──────────┴──────────┴──────────────────────────┴─────────┤
│          │                     │                          │         │
│  Filter  │  Filter Config      │  Low Freq Oscillator     │ Voicing │
│          │                     │                          │         │
├──────────┴─────────────────────┴──────────────────────────┴─────────┤
│  [JUCE MidiKeyboardComponent — C++ native, 80px strip]              │
└──────────────────────────────────────────────────────────────────────┘
```

### Files

| Action | File | What |
|--------|------|------|
| MODIFY | `UI/index.html` | Replace placeholder with two-row section grid matching the Figma. Top row: `<div class="top-row">` containing cards for Osc1, OscMix, Osc2, AmpEnv, Master. Bottom row: `<div class="bottom-row">` containing cards for Filter, FilterConfig, LFO, Voicing. Header bar at top. Debug panel `<div id="debug-panel">` at bottom, hidden by default. |
| MODIFY | `UI/styles.css` | CSS grid/flexbox layout. Use relative units (`%`, `fr`) so the layout scales when the plugin window is resized (see note on resizability below). Minimum plugin size: 900×540px. |

### Plugin Window Resizability

> [!NOTE]
> **Resizable window is the target.** JUCE supports `setResizable(true, false)` in the editor constructor to allow free resizing. Combined with `setResizeLimits(minW, minH, maxW, maxH)`, this keeps the window within sensible bounds.
>
> For the WebView to resize correctly, the HTML/CSS layout must use relative units (`%`, `vw`, `vh`, `fr`) rather than fixed pixels for major layout elements. Fader heights and section widths should scale with the container. Font sizes can use `clamp()` or fixed px (sliders already resize naturally).
>
> **If fully fluid CSS proves difficult:** implement 3 fixed-size presets (e.g., 75%, 100%, 150% scale) via a "UI Scale" button in the plugin, each calling `setSize(baseW * scale, baseH * scale)`. Presets are simpler to CSS and guarantee correct proportions. Decide which approach to use after testing the CSS responsiveness in 7.2.

---

## Sub-Phase 7.3 — Control Components

Build the reusable UI controls matching the Figma design.

### Control Types in the Design

| Control | Use case | Component file |
|---------|----------|---------------|
| **Vertical fader (slider)** | All continuous params (Attack, Cutoff, Level, etc.) | `UI/components/fader.js` |
| **Arrow selector** (`< value >`) | Discrete enum params: Waveform, Octave, Transpose, Filter Type, LFO Waveform, No. of Voices, Unison | `UI/components/arrow-selector.js` |
| **Dropdown** | Target selectors: LFO Target 1/2/3, Velocity Mod Target 1/2 | `UI/components/dropdown.js` |

> [!NOTE]
> The design uses **no rotary knobs** — the first iteration (knob-based) was rejected. All continuous parameters use vertical faders. There is no `knob.js` component needed.

### `fader.js`

Vertical slider for continuous parameters.
- Input: `paramId`, `min`, `max`, `defaultValue`, `label`, `skew` (optional, for frequency params)
- On user interaction: calls `SolaceBridge.setParameter(paramId, value)`
- On C++ `parameterChanged` event: updates display
- Visual: orange thumb on light grey track, label above, value readout below

### `arrow-selector.js`

`< value >` control for integer/enum parameters.
- Input: `paramId`, `values[]` (array of `{value, label}` objects), `defaultIndex`
- Left arrow: decrement (wraps). Right arrow: increment (wraps).
- On change: calls `SolaceBridge.setParameter(paramId, intValue)`
- On `parameterChanged`: updates displayed label
- Visual: label text centered between `<` and `>` chevrons (use SVG chevron icons from assets)

**Used for:** Waveform (Sine/Saw/Square/Triangle), Octave (-3 to +3), Transpose (-12 to +12), Filter Type (LP12/LP24/HP12), LFO Waveform, No. of Voices (1–16), Unison (1–8).

### `dropdown.js`

Dropdown for LFO target and velocity mod target selectors.
- Input: `paramId`, `options[]` (array of `{value, label}` objects), `defaultIndex`
- On change: calls `SolaceBridge.setParameter(paramId, intValue)`
- On `parameterChanged`: updates selected label
- Visual: truncated label with dropdown arrow (matches the "Filter - Cut..." style in the Figma)

**Used for:** `lfoTarget1/2/3`, `velocityModTarget1/2`.

### Files

| Action | File | What |
|--------|------|------|
| NEW | `UI/components/fader.js` | Vertical fader component |
| NEW | `UI/components/arrow-selector.js` | `< value >` discrete selector component |
| NEW | `UI/components/dropdown.js` | Target dropdown component |
| NEW | `UI/components/adsr-viz.js` | Canvas-based ADSR envelope shape visualizer (optional, nice-to-have for V1) |
| MODIFY | `UI/main.js` | Import components. Map all APVTS parameter IDs to component instances. Handle `parameterChanged` events from C++ to update all controls. |

---

## Sub-Phase 7.4 — Section Implementation

Wire the layout + components together into functional sections. Implement in top-row-first order.

### Header Bar
- SS monogram logo (SVG)
- "Solace Soft Synth" text (or just "Solace Synth" — confirm with designer)
- No controls in header for V1 (preset system is V2)

### Oscillator 1 Section
- `osc1Waveform` — arrow-selector (Sine / Saw / Square / Triangle)
- `osc1Octave` — arrow-selector (-3 to +3)
- `osc1Transpose` — arrow-selector (-12 to +12)
- `osc1Tuning` — fader (-100 to +100 cents)

### Osc Mix Section
- `oscMix` — single vertical crossfader (0.0–1.0). 0.0 = Osc1 only, 1.0 = Osc2 only, 0.5 = equal blend.

### Oscillator 2 Section
- `osc2Waveform` — arrow-selector (Sine / Saw / Square / Triangle)
- `osc2Octave` — arrow-selector (-3 to +3, default display: 1)
- `osc2Transpose` — arrow-selector (-12 to +12)
- `osc2Tuning` — fader (-100 to +100 cents)

### Amplifier Envelope Section
- `ampAttack` — fader
- `ampDecay` — fader
- `ampSustain` — fader
- `ampRelease` — fader
- (Optional: ADSR visualizer showing envelope shape live)

### Master Section
- `masterVolume` — fader (already implemented in placeholder UI)

### Filter Section
- `filterCutoff` — fader (20–20000 Hz, skewed logarithmic)
- `filterResonance` — fader (0.0–1.0)

### Filter Configuration Section
- `filterType` — arrow-selector (LP 12 dB / LP 24 dB / HP 12 dB), default: LP 24 dB
- `filterEnvAttack` — fader
- `filterEnvDecay` — fader
- `filterEnvSustain` — fader
- `filterEnvRelease` — fader

> [!NOTE]
> `filterEnvDepth` (bipolar, -1.0 to +1.0) is a DSP parameter but not clearly visible in the current Figma screenshot. Confirm with designer whether it should appear in Filter Configuration or Filter section. Add it wherever it fits visually.

### Low Frequency Oscillator Section
- `lfoWaveform` — arrow-selector (Sine / Triangle / Saw / Square / S&H)
- `lfoRate` — fader (0.01–50 Hz)
- `lfoAmount` — fader (0.0–1.0, labeled "Amount")
- `lfoTarget1` — dropdown (None / Filter Cutoff / Osc 1 Pitch / Osc 2 Pitch / Osc 1 Level / Osc 2 Level / Amp Level / Filter Resonance)
- `lfoTarget2` — dropdown (same options)
- `lfoTarget3` — dropdown (same options)

### Voicing Section
- `voiceCount` — arrow-selector (1–16, default 16), labeled "No. of Voices"
- `unisonCount` — arrow-selector (1–8, default 1), labeled "Unison"
- `velocityRange` — fader (0.0–1.0), labeled "Velocity Range"
- `velocityModTarget1` — dropdown (None / Amp Level / Amp Attack / Filter Cutoff / Filter Resonance)
- `velocityModTarget2` — dropdown (same options)

> [!NOTE]
> **`unisonDetune` and `unisonSpread`** are DSP parameters (added in Phase 6.7) but are not visible in the current Figma screenshot. They may have been omitted intentionally (the Unison arrow selector sets voice count only) or may need to be added. Confirm with designer. For V1, these params exist in the APVTS and are accessible via bridge — they just need UI controls added if desired.

---

## Sub-Phase 7.5 — Polish & Accessibility

- Micro-animations on fader interaction (hover state, active press glow on thumb)
- Smooth transition for arrow-selector value changes
- Loading state: replace "Connecting to engine..." status text with a subtle loading indicator → fade to full UI on bridge connect
- Debug panel: hidden by default, togglable with keyboard shortcut `Ctrl+Shift+D` (avoids conflict with DAW shortcuts)
- Ensure layout works at minimum window size (900×540px) and scales up cleanly

### Resizable Window — JUCE Side Implementation

When Phase 7 starts, add to `PluginEditor.cpp`:

```cpp
// In SolaceSynthEditor constructor:
setResizable(true, false);  // true = resizable, false = no aspect ratio lock
setResizeLimits(800, 480, 1800, 1080);  // min and max sizes
```

CSS must use relative units for major layout sections. The `MidiKeyboardComponent` at the bottom is a JUCE native component — it resizes automatically with the window.

> [!NOTE]
> Test resizing in both standalone and in a DAW host. Some DAW hosts restrict plugin window resizing (Ableton Live 10 and earlier do not support resizable plugin windows). The resize handle only appears in hosts that support it. In non-resizable hosts, the plugin simply renders at its initial size — this is expected and not a bug.

---

## Bridge Extension Needs

The current bridge supports:

| Direction | Function | Status |
|-----------|----------|--------|
| JS → C++ | `setParameter(id, value)` | ✅ Working |
| JS → C++ | `uiReady()` | ✅ Working |
| JS → C++ | `log(level, message)` | ✅ Working |
| C++ → JS | `parameterChanged(id, value)` | ✅ Working |
| C++ → JS | `syncAllParameters()` | ✅ Working |

**No bridge changes needed for Phase 7.** All new DSP parameters use the same `setParameter` / `parameterChanged` pattern. The bridge is parameter-ID-agnostic — it just forwards whatever ID you give it. As long as the APVTS parameter exists in C++, the bridge works automatically.

---

## Timing & Coordination with DSP

The UI and DSP tracks are **weakly coupled**:

- **UI can start as soon as Figma export is ready** — even before all DSP is done. Controls wired to parameters that don't exist yet simply won't do anything.
- **DSP doesn't need the UI** — all parameters are testable via APVTS defaults and the MIDI keyboard. Logs provide visibility.
- **Integration points:** After each DSP sub-phase, add the new parameter controls to `UI/main.js`. This is a ~10 minute task per sub-phase.

> [!TIP]
> **Hot-reload workflow:** JS/CSS/HTML files are served from disk (`SOLACE_DEV_UI_PATH`). Edit → save → close and reopen the plugin window → see changes instantly. No C++ rebuild needed for UI-only changes.

---

## Open Questions (confirm with designer)

| Item | Question | Status |
|------|----------|--------|
| `filterEnvDepth` | Where does the bipolar depth control appear in the UI — Filter section or Filter Config? | ⏳ Pending |
| `unisonDetune` / `unisonSpread` | Are these visible in the UI or handled silently by the engine? | ⏳ **Pending decision (Anshul)** |
| Plugin title | Confirmed as **"Solace Soft Synth"** (from Figma text node `I118:668;36:320`) | ✅ Resolved |
| Font | Confirmed: **MuseoModerno** (title), **Jura** (headings/tags), **NATS** (values). NATS file needed from Nabeel. | ⏳ Pending (font file) |
| Color values | ✅ Resolved — all exact hex values extracted. See `.agent/figma/design-tokens.md`. |
| Layout/spacing | ✅ Resolved — all padding, gap, border-radius values extracted from Figma MCP. |
| Waveform icons | Sine + Square confirmed. Sawtooth + Triangle icons **must be created by Nabeel** before Phase 7.3. | ⏳ Pending (designer) |
| Menu Button | Present in top-right of design. Contents/behaviour TBD — likely preset management or settings panel. | ⏳ Pending (design decision) |
| MIDI Keyboard | CSS-drawn (Figma design) vs. JUCE native (current impl). Options documented in `design-tokens.md`. | ⏳ **Pending decision (Anshul)** |
| Pitch Bend / Mod Wheel | In design as CSS sliders (left of keyboard). MIDI wiring approach TBD. | ⏳ Pending decision |
| `masterDistortion` | ✅ Confirmed — Distortion slider is in Master section (Figma Interface 3). DSP added as Phase 6.9. UI control needed in Phase 7. |

---

## Nice-To-Have (V1.1 / V2)

- [ ] ADSR envelope shape visualizer (canvas, live)
- [ ] Preset browser panel (save/load, factory presets)
- [ ] Oscilloscope / spectrum analyzer visualization
- [ ] Debug panel keyboard shortcut `Ctrl+Shift+D`
- [ ] Dark theme option
- [ ] Theming (custom color palettes)
- [ ] Embed UI files via `juce_add_binary_data()` instead of disk path (required for distribution)
- [ ] Accessibility (ARIA labels for screen readers, high-contrast mode)
