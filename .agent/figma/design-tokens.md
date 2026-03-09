# Solace Synth — Figma Design Tokens
> Extracted from: `Synth Project / Mockup / Interface 3`
> Figma File: `p9vEzTpMbKPC3nh7EK3XSg`
> Node: `118:665`
> Extracted: 2026-03-09

---

## Color Palette

| Token Name | Hex Value | Usage |
|---|---|---|
| `White` | `#FFFFFF` | Background, section fill, button fills |
| `Black` | `#202020` | Primary text, tags, labels, waveform text |
| `Dark` | `#666666` | Secondary text, section headings, slider values |
| `Medium` | `#999999` | Logo (middle S element) |
| `Light` | `#CCCCCC` | Logo (lightest S element) |
| `Lightest` | `#EEEEEE` | Dividers, hover borders, section borders, separator line |
| `Primary` | `#FF9E2F` | Accent — Logo underline stroke, primary interactive color |
| `fill_78XKYP` | `#D9D9D9` | Internal chevron/button fill |

### CSS Custom Properties (ready to use)
```css
:root {
  --color-bg:         #FFFFFF;
  --color-text:       #202020;
  --color-text-dark:  #666666;
  --color-text-mid:   #999999;
  --color-text-light: #CCCCCC;
  --color-border:     #EEEEEE;
  --color-accent:     #FF9E2F;
  --color-divider:    #D9D9D9;
}
```

---

## Typography

| Token Name | Font Family | Weight | Size (px) | Line Height | Letter Spacing | Usage |
|---|---|---|---|---|---|---|
| `Title` | `MuseoModerno` | 600 | 48 | 1.59em | 0.31% | Logo "SS" lettermark, plugin title |
| `Heading` | `Jura` | 700 | 28 | 1.18em | — | Section headers (Oscillator 1, Filter, LFO, etc.) |
| `Tag` | `Jura` | 600 | 15 | 1.6em | 1.0% | Control labels (Attack, Decay, Waveform, Cutoff, etc.) |
| `Contents` | `NATS` | 400 | 20 | 1.2em | 0.75% | Value readouts, dropdown text, selector values |
| `style_VEC66X` | `Jura` | 700 | 20 | 1.18em | — | Preset dropdown text (larger dropdown) |

### Fonts to Import
```html
<!-- Google Fonts — add to <head> -->
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=MuseoModerno:wght@600&family=Jura:wght@600;700&display=swap" rel="stylesheet">
```

> ⚠️ **NATS** — This may be a custom/paid font, not on Google Fonts. Verify if Nabeel has a font file, or identify a substitute.
> ⚠️ **MuseoModerno** — Available on Google Fonts as `MuseoModerno`.

### CSS Typography Variables
```css
:root {
  --font-display: 'MuseoModerno', sans-serif;
  --font-ui: 'Jura', sans-serif;
  --font-value: 'NATS', sans-serif;

  --text-title:    600 48px/1.59 var(--font-display);
  --text-heading:  700 28px/1.18 var(--font-ui);
  --text-tag:      600 15px/1.6  var(--font-ui);
  --text-contents: 400 20px/1.2  var(--font-value);
}
```

---

## Effects

| Token Name | Value | Usage |
|---|---|---|
| `Basic Drop` | `box-shadow: 0px 1px 1px 0px rgba(0,0,0,0.25)` | Section cards (Oscillator, Filter, LFO, Voicing) |

---

## Layout — Root Frame (Interface 3)

- **Canvas size:** 1440 × 1024 px
- **Layout mode:** column
- **Padding:** `40px 48px`
- **Gap between rows:** `32px`
- **Justify content:** center
- **Align items:** stretch

### CSS
```css
.plugin-root {
  width: 1440px;
  height: 1024px;
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: stretch;
  gap: 32px;
  padding: 40px 48px;
  background: var(--color-bg);
}
```

---

## Layout — Top Bar (Frame 30)

- **Mode:** row
- **Justify:** space-between
- **Align items:** flex-end
- **Gap:** 32px
- **Size:** fill horizontal, hug vertical

Contains:
- Left: Logo group + "Solace Soft Synth" title
- Right: Preset Selecter (left button + dropdown + right button) + Menu Button

### Logo Area (Frame 44)
- **Mode:** row, align-items: center, hug × hug
- Logo padding: `0px 24px`, gap: `10px`

### Preset Selecter
- **Border:** 1px `#EEEEEE`, border-radius: `8px`
- **Padding:** `4px`
- **Gap between items:** `8px`
- Dropdown width: **320px**, padding: `12px`, border-radius: `4px`

### Menu Button
- Padding: `16px`, border-radius: `8px`
- Border: 1px `#EEEEEE`

---

## Layout — Section Cards

All major section cards (Oscillator, Filter, LFO, Voicing):
- **Border:** 1px `#EEEEEE`
- **Border-radius:** `8px`
- **Box-shadow:** `0px 1px 1px 0px rgba(0,0,0,0.25)` (Basic Drop)

### Oscillator Block (Frame 33) — Row
- **Mode:** row, padding: `24px`, gap: `24px`, hug × hug

### Filter + LFO Block (Frame 36) — Row
- **Mode:** row, padding: `24px 32px`, gap: `24px`

### Voicing Block (Frame 19)
- **Mode:** column, padding: `24px 32px`, gap: `24px`, hug × hug
- **Stroke:** `Light` (#CCCCCC) — note: lighter border than other sections
- **No drop shadow** on voicing

---

## Layout — Within Sections

### Section Header (inside each card)
```
vertical stack (column), gap: 24px, hug × hug
  ├── Section Title (Heading text style)
  └── Controls row
```

### Vertical Slider Assembly
```
column, gap: 8px, hug × hug
  └── Slot row: row, gap: 8px
        ├── Label (Tag text style, fills: Black)
        └── Slider SVG component
```

### Selector (Octave / Transpose / etc.)
- **Width:** 94px fixed
- **Mode:** column, gap: 8px
- Inner row: `chevron_backward | value text | chevron_forward`
  - Chevrons: 24×24px
  - Value text fixed width: 24px

### Waveform Selector
- **Mode:** column, align-items: center, gap: 12px, hug × hug
- Text label (Tag style) above SVG waveform icon
- SVG icon container: row, center, gap: 8px

### Dropdown
- **Small (LFO/Voicing):** border 1px `#EEEEEE`, radius: `4px`, padding: `8px`, fill-horizontal, gap: `8px`
- **Large (Preset):** width 320px, padding: `12px`, border 1px `#EEEEEE`

---

## Component Inventory

| Component Name | Node ID | Purpose |
|---|---|---|
| `Logo` | `84:236` | SS monogram + underline |
| `Title` | `37:633` | "Solace Soft Synth" text |
| `Preset Selecter` | `120:1532` | Preset nav [< Dropdown >] |
| `Left Button` | `120:1513` (set) | Preset nav left |
| `Right Button` | `120:1526` (set) | Preset nav right |
| `Menu Button` | `120:1208` (set) | Hamburger/menu icon |
| `Dropdown` | `120:1185` (set) | Preset + small dropdowns |
| `Waveform` | `36:504` (set) | Waveform selector (Sine/Square/etc) |
| `Selecter` | `36:388` | Arrow-selector (Octave, Transpose, etc) |
| `Vertical Slide` | `85:736` | Vertical slider container slot |
| `Slider` | `85:624` (set) | The actual slider handle SVG |
| `Horizontal Slide` | `120:1374` (set) | Horizontal sliders (if needed) |
| `Octave of Keys` | `120:1250` | MIDI keyboard octave display |

---

## Section Map — All Controls in Interface 3

### Row 1 Top-Bar
- Logo (SS monogram + title: "Solace Soft Synth")
- Preset Selecter: [< | "Initial Preset" | >]
- Menu Button

### Row 2 — Left Block
**Oscillator 1** | **Osc Mix** | **Oscillator 2** (3 sub-sections, row)

#### Oscillator 1
- Waveform selector (default: Sinewave)
- Octave selector (value: 0)
- Transpose selector (value: 0)
- Tuning vertical slider

#### Osc Mix
- Single vertical slider (Osc 2 ←→ Osc 1 labels)

#### Oscillator 2
- Waveform selector (default: Squarewave)
- Octave selector (value: 1)
- Transpose selector (value: 0)
- Tuning vertical slider

**Amplifier Envelope** | **Master** (two sub-sections)

#### Amplifier Envelope
- Attack slider (value: 0)
- Decay slider (value: High)
- Sustain slider (value: Mid)
- Release slider (value: 0)

#### Master
- Distortion slider (value: 0)
- Level slider (value: Mid)

### Row 2 — Right Block
**Filter** | **Filter Configuration** (two sub-sections)

#### Filter
- Cutoff slider (variant: Big2 — tall position)
- Resonance slider (variant: Big1)

#### Filter Configuration
- Filter Type selector: [< | "LP 24 dB" | >]
- Filter Envelope: Attack, Decay, Sustain, Release sliders

**Low Frequency Osc**

#### LFO
- Waveform selector (default: Sinewave)
- LFO Targets: 3 dropdowns (Targets 1: "Filter - Cut...", Target 2: "Osc 2 - Level", Target 3: "None")
- Amount slider (variant: Big1)
- Rate slider (variant: Big2)

### Row 2 — Rightmost
**Voicing** (lighter border, no shadow)

#### Voicing
- No. of Voices selector (value: 16)
- Unison selector (value: 3)
- Velocity Mod Targets: 2 dropdowns ("Amp - Atta...", "None")
- Velocity slider (variant: Big1)

### Row 3 (Keyboard + Performance Row)

The bottom row is a `flex justify-between items-center self-stretch` container with two sub-panels:

#### Left Sub-Panel — Pitch Bend & Mod Wheel
- Bordered panel: `border-[#eeeeee] rounded-lg p-3`, column layout, `gap: 16px`, `self-stretch` (full height of keyboard row)
- Contains **2 vertical sliders** side by side:
  - **Slider 1** (Pitch Bend): track height `128px`, bg `#cccccc`, thumb fill `#eeeeee`/`#666666`
  - **Slider 2** (Mod Wheel): track height `128px`, bg `#cccccc`, thumb at higher position in design
- Each slider has a `24×20px` white label area to its right

> ⚠️ **Pending decision:** Pitch Bend and Mod Wheel are MIDI performance controllers. Wiring these in the WebView bridge vs. using JUCE native controls needs to be discussed.

#### Right Sub-Panel — MIDI Keyboard

> ⚠️ **OPEN DECISION: CSS keyboard vs. JUCE native keyboard — to be discussed.**

**Option A — CSS-drawn keyboard (as in Figma design):**
- 6 octaves × 192px wide = 1152px total, 64px tall
- White keys: SVG elements with `fill: #eeeeee`, various widths (26px, 27px, 28px, 29px)
- Black keys: `div` elements, `16×40px`, `background: #666666`
- Wrapped in `border-[#eeeeee] rounded-lg p-2`
- Rendered entirely in HTML/CSS; notes sent via `bridge.js`

**Option B — JUCE native `MidiKeyboardComponent` (current implementation):**
- Already in codebase from Phase 5
- Embedded at the bottom of the plugin window below the WebView area
- Handles MIDI input directly; no bridge needed for note on/off
- Appearance is JUCE-styled, not pixel-perfect to Figma design
- Simpler, more robust, handles all edge cases (key highlighting, MIDI input)

**Tradeoffs:** CSS keyboard matches Figma exactly and allows full styling control. JUCE keyboard handles MIDI I/O reliably and is already implemented. Decision pending.

---

## SVG Assets Downloaded

Stored in `UI/assets/icons/`:

| File | Dimensions | Content |
|---|---|---|
| `logo.svg` | 111×76 | SS monogram with orange underline |
| `waveform-sine.svg` | 144×68 | Sine waveform icon |
| `waveform-square.svg` | 144×68 | Square waveform icon |
| `waveform-all-variants.svg` | 460×108 | All waveform variants in one component set |
| `selecter-component.svg` | 94×56 | Arrow selector (< value >) |
| `btn-left.svg` | 56×56 | Left chevron button |
| `btn-right.svg` | 56×56 | Right chevron button |
| `btn-menu.svg` | 64×64 | Menu/hamburger button |
| `slider-vertical.svg` | 60×210 | Vertical slider track + thumb |
| `preset-selecter.svg` | 456×64 | Full preset nav bar |
| `keyboard-octave.svg` | 192×64 | One octave keyboard |

> **Still needs individual export:** Sawtooth, Triangle, and any other waveform variants. Also confirm: `arrow_drop_down`, `chevron_backward`/`chevron_forward` as individual SVGs.

---

## Open Questions / Gaps

1. **NATS font** — Is this a free font? Where is the font file? Check with Nabeel. *(Acknowledged — will be resolved)*
2. **Waveform variants** — Design currently has Sine + Square only. Will Sawtooth and Triangle be added? *(Acknowledged — may be added)*
3. **Slider states** — "Big1", "Big2", "Mid", "High", "0", "Small", "Small1", "Small2" — what are the exact pixel track heights for each variant?
4. **Filter type options** — The selector shows "LP 24 dB". What are all the filter type options? (LP12, LP24, HP12, HP24, BP?)
5. **LFO Target options** — What are all the possible dropdown values? (Full list for JS implementation)
6. **Keyboard: CSS vs JUCE native** — See Row 3 section above. Decision pending.
7. **Pitch Bend / Mod Wheel** — Confirmed present in design (left of keyboard). Are these MIDI performance controls only, or also mapped to synth parameters?
