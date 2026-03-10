# Phase 7 — Pixel-Perfect Audit & Design Guide

This document serves as the definitive roadmap to bridge the gap between the current Chrome HTML/CSS scaffold and the flawless Figma Interface 3 design. 

## 1. Global Typography
The Chrome implementation is suffering from severe font fallbacks, making the UI look rigid and boxy.
*   **The Title:** The Figma design uses a distinct, cursive/rounded geometric font for "Solace Soft Synth" (MuseoModerno). Chrome is falling back to a standard sans-serif.
*   **The Values:** The numeric values (`0`, `0.50`, `-12`) in Chrome are using the `Jura` font, which creates aggressive, squared-off numbers that look terrible. The Figma design uses `NATS`, a soft, highly legible, rounded sans-serif.
*   **ACTION:** We must source `.woff2` files for `MuseoModerno` and `NATS` (or `Nunito` / `Varela Round` as a free duplicate), place them in `UI/assets/fonts/`, and map them in `styles.css`. 

## 2. The Slider Overhaul [CRITICAL]
The vertical faders in the Chrome browser are fundamentally incorrect compared to the design. They require a complete rewrite in CSS and HTML structure.
*   **Vertical Labels:** In Chrome, labels (like "Attack", "Cutoff", "Rate") are printed horizontally above the sliders. **In Figma, they are printed vertically along the left side of the track** reading bottom-to-top (rotated -90 degrees). 
*   **Track Fill (Active Range):** In Chrome, the track is a solid light-grey line. **In Figma, the track fills with a dark grey color** from the bottom up to where the thumb sits (e.g., Cutoff is filled all the way up, Sustain is filled 80%). We will need to use a CSS `linear-gradient` trick driven by JS variable `--val` to paint this fill dynamically.
*   **Scale Tick Marks:** **Figma features ruler tick marks** running vertically along the right side of every slider track (approx 10 ticks per slider). Chrome has none. We will achieve this using a `repeating-linear-gradient` on a pseudo-element (`::after`) next to the input track.
*   **Osc Mix Anomaly:** The Osc Mix crossfader in Figma has NO numeric value below it. Instead, "Osc 2" is written vertically next to the top half of the track, and "Osc 1" vertically next to the bottom half. Chrome currently stacks them horizontally.

## 3. Waveform & Arrow Selectors
*   **Missing Chevrons:** The `<` and `>` arrow icons are entirely missing/broken in Chrome. We need to hand-code these SVGs to match the clean, thin strokes of the Figma arrows so the Octave, Transpose, and Voicing selectors render correctly.
*   **Waveform Navigation:** In Chrome, the Waveform is just an icon. **In Figma, the waveform icon is flanked by `<` and `>` arrow buttons** (`< ~~~ >`). We must inject these buttons into the `.waveform-selector` HTML.

## 4. Dropdowns & Minor Text
*   **Dropdown Arrows:** The LFO Target and Velocity Mod Target dropdowns in Chrome are missing their right-aligned down arrows (`▼`). We need to implement this via CSS or SVG.
*   **Filter Label:** In Figma, the layout says `Filter Type -` and then has the `< LP 24 dB >` selector below it. Chrome is missing the hyphen. 

## 5. Row 3: Performance Controls & Keyboard
The bottom row is completely missing from the HTML file right now. 
*   **Pitch Bend & Mod Wheel:** We must build a boxed panel on the bottom left containing two specialized sliders. In Figma, these sliders are unique: the thumb is a solid grey pill (not a hollow orange ring), and they have an empty white rectangular space to explicitly indicate their zero-points/range.
*   **The CSS Keyboard:** According to the Figma screenshot, there is a beautifully rendered 6-octave piano keyboard at the bottom right. Even if we plan to use the JUCE native keyboard in the C++ backend for *audio*, we should absolutely build the HTML/CSS visual replica of this keyboard to make the UI look complete and premium as a frontend deliverable.

---

## Technical Execution Steps

### Step 1: The Components Rewrite (HTML/CSS)
Refactor `.fader` in `styles.css` to use CSS Grid.
- Column 1: Vertical label (`writing-mode: vertical-rl; transform: rotate(180deg)`).
- Column 2: The range `<input>`, styled with dynamic background gradient fills.
- Column 3: A `.fader-ticks` div using a repeating gradient.
- Row 2: The `.fader-value` text, spanning the width.

### Step 2: Icon Generation
Hand-code the mathematically perfect geometric SVGs for:
1. `chevron-back.svg`
2. `chevron-forward.svg`
3. `arrow-dropdown.svg`
4. `btn-menu.svg` (Hamburger icon)

### Step 3: Layout Correction
Inject the missing `<` `>` arrows into the Waveform selectors. Rebuild the `Osc Mix` slider layout to use the split vertical labels. 

### Step 4: Row 3 Construction
Inject `<div id="row-third">` into `index.html`. Build the Pitch Bend & Mod Wheel custom faders, and construct the CSS interlocking piano keys layout.
