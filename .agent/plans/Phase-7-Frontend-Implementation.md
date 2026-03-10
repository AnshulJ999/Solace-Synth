# Solace Synth — Phase 7 UI Implementation Plan

## 1. Visual Audit: Chrome Browser vs. Figma Spec
Based on the visual comparison of the Chrome rendering against the Figma design tokens, several discrepancies currently exist that need to be resolved to achieve pixel-perfection.

### A. Missing / Broken Native Icons
- **Chevrons & Dropdowns:** The `chevron-back.svg`, `chevron-forward.svg`, and `arrow-dropdown.svg` icons are either extremely faint placeholders or rendering improperly. We need to manually write pure CSS geometric arrows or manually code these SVGs to match the Figma `<` and `>` arrow weights.
- **Top Nav Buttons:** The `btn-left.svg`, `btn-right.svg`, and `btn-menu.svg` (hamburger menu) are using placeholder assets.

### B. Typography Discrepancies
- **Value Numbers:** The values (`0.01`, `0.50`, `20k`) are currently falling back to the `Jura` font, which is highly blocky and aggressive. The Figma design specifically uses the **NATS** font for values, which is softer and more rounded. 
- **Action:** We must download a `.woff2` file for NATS (or a visually identical free alternative) and bundle it entirely within `UI/assets/fonts/` to ensure offline DAW compatibility.

### C. Layout & Component Gaps
- **Row 3 (Performance Row):** The entire bottom row containing the **Pitch Bend** and **Mod Wheel** is completely absent from the HTML layout.
- **Filter Envelope Depth:** The slider for `filterEnvDepth` exists in the `index.html` DOM but is `hidden`. It needs a proper layout home in the Filter Configuration block alongside Attack/Decay/Sustain/Release.

---

## 2. DSP & Engine Alignments (Vision Doc Updates)

### A. The "Master Distortion" Slider
- **Conflict:** The `design-tokens.md` flagged the Master "Distortion" slider as a critical gap because it wasn't in the Phase 6 DSP plan.
- **Resolution:** Your latest Vision Document update explicitly lists "Distortion" as an LFO modulation target! This means **Distortion is officially approved** and must be added to the C++ DSP engine. The UI slider should remain.

### B. Expanded LFO Targets
- **Update Required:** The `index.html` and `main.js` currently use an old LFO target list.
- **Action:** Update the dropdown enums across both HTML and C++ to match the new Vision Document list:
  1. Oscillator pitch (both)
  2. Oscillator tuning
  3. Amplifier Attack
  4. Filter Cutoff
  5. Filter Resonance
  6. Distortion
  7. Master volume
  8. Oscillator mix

### C. Keyboard Implementation
- **Action:** Stick to the **JUCE Native Keyboard** for now (as decided in the Vision Doc). We will let the C++ backend draw the MIDI keyboard at the bottom of the plugin window, meaning the HTML only needs to render the Pitch Bend and Mod Wheel sliders on the left side of Row 3.

---

## 3. Step-by-Step Execution Plan

**Step 1: Icon Polish & Layout Tuning**
- Write pure mathematical SVGs for `chevron-back`, `chevron-forward`, `arrow-dropdown`, and the `btn-menu` hamburger icon so we don't need Figma exports.
- Fine-tune slider thumb alignments and track spacing in `styles.css` to precisely match the Figma screenshot padding.

**Step 2: Font Integration**
- Source the `NATS` font or a highly similar open-source rounded sans-serif (like `Nunito` or `Varela Round`).
- Convert to `.woff2`, place in `UI/assets/fonts/`, and update `@font-face` in `styles.css`.

**Step 3: Build Row 3 (Pitch & Mod Wheels)**
- Inject `<div id="row-third">` into `index.html`.
- Add two tall vertical sliders (`fader--big`) for Pitch Bend and Modulation.
- Create new APVTS parameters for these in the C++ backend.

**Step 4: Align LFO Dropdowns & Filter Env Depth**
- Unhide the `filterEnvDepth` slider and arrange it cleanly above the ADSR sliders.
- Update `main.js` and `PluginProcessor.cpp` with the 8 new comprehensive LFO targets.

**Step 5: Master Distortion DSP**
- Sub-phase: Add a soft-clipper / waveshaper DSP module to the C++ audio block, wired to the `masterDistortion` APVTS parameter.
