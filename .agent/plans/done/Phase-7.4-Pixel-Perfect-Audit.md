# Phase 7 — Pixel-Perfect Audit & Integration Plan
*(Merged with Frontend Implementation Plan)*

This plan was built by Gemini. Take it with a grain of salt and always discuss/approve everything with me. Don't take it over-seriously.

This document serves as one roadmap to bridge the gap between the current Chrome HTML/CSS scaffold and the flawless Figma Interface 3 design, while integrating necessary DSP-level alignments approved in the Vision Document. 

---

## 1. Visual Audit: Chrome Browser vs. Figma Spec

### A. Global Typography & Fonts
*   **The Title:** The Figma design uses `MuseoModerno` (a futuristic, rounded font) for the title. Chrome might be falling back to a standard sans-serif if not loaded. We should ensure `MuseoModerno` is loaded.
*   **The Values:** The Figma design initially used `NATS`, a soft, rounded sans-serif. **However, Nabeel explicitly noted in the Vision Document:** *"We can just use Jura, too many fonts will make it look messy anyways, let’s not take risks with it."*
*   **Action:** We will stick to the **Jura** font for numeric values and headings to avoid visual clutter and align with Nabeel's vision. We only need to ensure `Jura` and `MuseoModerno` (for the title) are properly bundled in `UI/assets/fonts/`.

### B. The Slider Overhaul [CRITICAL]
The vertical faders require a complete rewrite in CSS and HTML structure to match Figma:
*   **Vertical Labels:** In Chrome, labels ("Attack", "Cutoff") are horizontal on top. **In Figma, they are printed vertically** along the left side of the track, reading bottom-to-top.
*   **Track Fill (Active Range):** **Figma tracks fill with a dark grey color** from the bottom up to where the thumb sits. Chrome just has a flat light-grey track.
*   **Scale Tick Marks:** **Figma features ruler tick marks** vertically along the right side of every slider track. Chrome has none.
*   **Osc Mix Anomaly:** The Osc Mix crossfader in Figma has NO numeric value below it. "Osc 2" is written vertically next to the top half, and "Osc 1" next to the bottom half. 

### C. Waveform & Arrow Selectors
*   **Missing Chevrons:** The `<` and `>` arrow icons in the Octave, Transpose, and Voicing selectors are broken/missing in Chrome. 
*   **Waveform Navigation:** **In Figma, the waveform icon is flanked by `<` and `>` arrow buttons** (`< ~~~ >`). Chrome just has the raw image.
*   **Dropdown Arrows:** Dropdowns (LFO Targets) are missing their right-aligned down arrows (`▼`).
*   **Filter Label:** Figma says `Filter Type -` followed by the `< LP 24 dB >` selector. Chrome is missing the hyphen.
*   **Action:** Hand-code pure SVG geometric arrows to match the thin strokes of the Figma design.

### D. Row 3: Performance Controls & Keyboard
*   **Pitch Bend & Mod Wheel:** We must build a boxed panel on the bottom left containing two specialized sliders. These have solid grey thumbs and an empty white rectangular space to indicate their zero-points/range.
*   **The CSS Keyboard:** The Vision Document explicitly notes: *"Stick to JUCE native for now and discuss CSS later."* Therefore, we will deliberately **omit** building the HTML piano keyboard. The space at the bottom right will remain empty to allow the native JUCE `MidiKeyboardComponent` to render through from the C++ window.

---

## 2. DSP & Engine Alignments (Vision Doc Updates)

### A. The "Master Distortion" Slider
*   **Status:** APPROVED. The Vision Document explicitly lists "Distortion" as an LFO modulation target.
*   **Action:** Keep the UI slider. A soft-clipper / waveshaper DSP module must be added to the C++ backend later.

### B. Expanded LFO Targets
*   **Update Required:** The `index.html` and `main.js` currently use an old LFO target list.
*   **Action:** Update the dropdown enums across both HTML and C++ to match the new Vision Document list:
    1. Oscillator pitch (both)
    2. Oscillator tuning
    3. Amplifier Attack
    4. Filter Cutoff
    5. Filter Resonance
    6. Distortion
    7. Master volume
    8. Oscillator mix

### C. Filter Envelope Depth
*   **Action:** Unhide the `filterEnvDepth` slider in `index.html` and arrange it cleanly above the ADSR sliders in the Filter Configuration block.

---

## 3. Step-by-Step Execution Plan

**Step 1: Icon Generation & Typography**
- Hand-code mathematically perfect geometric SVGs for `chevron-back`, `chevron-forward`, `arrow-dropdown`, and `btn-menu` (hamburger icon).
- Source `NATS` (or `Nunito` fallback) and `MuseoModerno` fonts locally.

**Step 2: The Components Rewrite (HTML/CSS)**
- Refactor the `.fader` component in `styles.css` using CSS Grid to support vertical labels, active dynamic background track fills (via JS `--val` updates), and repeating gradient tick marks.
- Rebuild the `Osc Mix` slider layout to use the split vertical labels.

**Step 3: Layout Correction & Wiring**
- Inject `<` `>` arrows into the Waveform selectors.
- Unhide `filterEnvDepth` and arrange it in Filter Config.
- Update `main.js` and `PluginProcessor.cpp` with the 8 new comprehensive LFO targets.

**Step 4: Row 3 Construction**
- Inject `<div id="row-third">` into `index.html`. 
- Build the exact Pitch Bend & Mod Wheel custom fader styles.
- Construct the interlocking piano keys CSS layout for visual completeness.

**Step 5: Master Distortion DSP (C++ Side)**
- Add a soft-clipper / waveshaper DSP module into the C++ audio block during Phase 6/7 intersection.
