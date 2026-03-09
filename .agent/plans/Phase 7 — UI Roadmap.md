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

## Prerequisites

1. **Figma export** — your designer friend needs to export the final design. Ideally as:
   - Full-page screenshots / mockups for visual reference
   - Exported assets (SVGs for icons, PNGs for textures — if any)
   - Color palette, font choices, spacing specs
   - You do NOT need Figma-to-code plugins — we'll hand-write the HTML/CSS/JS

2. **DSP parameters defined** — the UI needs to know what knobs/sliders to show. Each DSP sub-phase adds APVTS parameters, so the UI can always stay current with whatever is implemented.

---

## Sub-Phase 7.1 — Design System Setup

**Before building any UI pages,** establish the CSS foundation.

### Files

| Action | File | What |
|--------|------|------|
| MODIFY | `UI/styles.css` | Complete rewrite: CSS custom properties for colors, fonts, spacing, border-radius. Dark theme base. Import Google Font (Inter or whatever the Figma specifies). Define component primitives: `.slider`, `.knob-container`, `.section`, `.label`. |
| NEW | `UI/assets/` (directory) | Any exported SVGs, background textures, or icons from Figma. |

### What to define:
- Color palette (background, surface, primary accent, secondary, text, muted text)
- Font stack + sizes (heading, label, value readout)
- Spacing scale (4px base grid)
- Component tokens (slider track color, thumb color, active states, section backgrounds)
- Transition/animation defaults

---

## Sub-Phase 7.2 — Layout Scaffold

Build the page structure matching the Figma layout.

### Typical synth UI zones (adjust to actual Figma):

```
┌─────────────────────────────────────────────────┐
│  Header bar (logo, preset name, master volume)  │
├──────────────┬──────────────┬───────────────────┤
│              │              │                   │
│  Oscillator  │   Filter     │   Envelope        │
│   Section    │   Section    │   Section         │
│              │              │                   │
├──────────────┴──────────────┴───────────────────┤
│         LFO / Modulation Section                │
├─────────────────────────────────────────────────┤
│  [JUCE MidiKeyboardComponent — C++ native]      │
└─────────────────────────────────────────────────┘
```

### Files

| Action | File | What |
|--------|------|------|
| MODIFY | `UI/index.html` | Replace placeholder with semantic layout: `<header>`, `<section class="oscillators">`, `<section class="filter">`, `<section class="envelopes">`, `<section class="lfo">`. Each section contains labeled controls. Include `<div id="debug-panel">` (collapsible, hidden by default in production). |
| MODIFY | `UI/styles.css` | Grid/flexbox layout matching Figma. Responsive within the plugin window (min 800×450, typical 900×600). |

---

## Sub-Phase 7.3 — Control Components

Build the reusable UI controls. The Figma will determine whether these are sliders, knobs, or something else.

### Likely controls needed:

| Control | Use case | Bridge interaction |
|---------|----------|-------------------|
| **Horizontal slider** | Attack/Decay/Release, Osc Mix | `SolaceBridge.setParameter(id, value)` |
| **Rotary knob** (if Figma uses them) | Cutoff, Resonance, LFO Rate | Same bridge call |
| **Dropdown / segmented toggle** | Waveform select, Filter type, LFO target | `SolaceBridge.setParameter(id, intValue)` |
| **ADSR visualizer** | Shows envelope shape live | Read-only, driven by param values |
| **Value readout** | Shows current value below/beside control | Updated on `parameterChanged` events from C++ |

### Files

| Action | File | What |
|--------|------|------|
| NEW | `UI/components/slider.js` | Reusable slider component. Takes parameter ID, min, max, step, label. Sends `setParameter` on change. Updates on C++ `parameterChanged`. |
| NEW | `UI/components/knob.js` | (Only if Figma design uses rotary knobs.) Mouse-drag → rotation mapping. |
| NEW | `UI/components/selector.js` | Dropdown or segmented control for waveform/filter type/LFO target. |
| NEW | `UI/components/adsr-viz.js` | Canvas-based ADSR shape visualizer. |
| MODIFY | `UI/main.js` | Import components. Map APVTS parameter IDs to component instances. Handle `parameterChanged` events from C++ to update all controls. |

---

## Sub-Phase 7.4 — Section Implementation

Wire the layout + components together into functional sections.

### Oscillator Section
- Osc1 waveform selector (Sine/Saw/Square/Triangle)
- Osc2 waveform selector
- Osc2 octave (+/- buttons or selector)
- Osc2 detune slider
- Osc mix slider (Osc1 ↔ Osc2)

### Filter Section
- Cutoff knob/slider
- Resonance knob/slider
- Filter type selector (LP/HP/BP)
- Filter env depth slider (bipolar: -1.0 to +1.0)
- Filter ADSR sliders (A/D/S/R)

### Amp Envelope Section
- ADSR sliders (A/D/S/R)
- ADSR visualizer

### LFO Section
- Rate knob/slider
- Shape selector (Sine/Triangle/Saw/Square/S&H)
- Target selector (FilterCutoff/Pitch/Amp)
- Depth slider

### Header
- Master volume slider
- (Future: preset name, prev/next preset buttons)

---

## Sub-Phase 7.5 — Polish & Animations

- Micro-animations on control interaction (hover glow, active press state)
- Smooth transitions when switching sections or tabs (if tabbed layout)
- Loading screen removal (replace "Connecting to engine..." with splash → fade)
- Debug panel: hidden by default, togglable via keyboard shortcut (e.g., Ctrl+D)
- Responsive sizing: ensure the layout works if the plugin window is resized

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

- **UI can start as soon as the Figma export is ready** — even before all DSP is done. Controls can be wired to APVTS parameters as they get added. Controls for parameters that don't exist yet simply won't do anything.
- **DSP doesn't need the UI** — all parameters are testable via APVTS defaults and the placeholder slider. The MIDI keyboard provides input. Logs provide visibility.
- **Integration points:** After each DSP sub-phase, update `UI/main.js` to include the new parameter controls. This is a 5-minute task per sub-phase.

> [!TIP]
> **Hot-reload workflow:** JS/CSS/HTML files are served from disk. Edit them → save → refresh the WebView (close+reopen plugin window) → see changes instantly. No C++ rebuild needed for UI-only changes.

---

## Nice-To-Have (V1.1 / V2)

- [ ] Preset browser panel (save/load, factory presets)
- [ ] Oscilloscope / spectrum analyzer visualization
- [ ] Keyboard shortcuts for common actions
- [ ] Accessibility (screen reader labels, high-contrast mode)
- [ ] Theming (light/dark/custom)
- [ ] Embed UI files via `juce_add_binary_data()` for distribution
