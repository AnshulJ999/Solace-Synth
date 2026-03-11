# Phase 3 — Figma-First `transform: scale()` Refactor

> **Status:** DRAFT — pending design lock + Anshul/Nabeel sign-off  
> **Prerequisite:** Phase 1 (1440×1104 window) + Phase 2 (clamp calibration) must be live and verified first.  
> **Do NOT implement piecemeal.** This is an atomic refactor — do it all in one session.

---

## What Phase 3 Is

Replace the current responsive CSS system (`clamp()`, `vh`, `vw`, flex ratios) with:

1. **Fixed Figma pixels** for every value in the CSS
2. **One uniform `transform: scale()` on the root** for resizability

This is the architecture used by Serum, Vital, Surge, and most professional synth UIs. It guarantees pixel-perfect Figma match at the canonical size and correct proportional scaling at any window size.

---

## Why This Is Better Than the Current Approach

| | Current (clamp/vh/vw) | Phase 3 (fixed + scale) |
|---|---|---|
| Figma match at target size | Approximate | Pixel-perfect |
| Resizability | Proportional but drifts | Uniform — guaranteed proportions |
| Tick ruler alignment | Fights browser geometry | Fixed — no drift possible |
| Typography | Fluid but inexact | Exact Figma values, scaled |
| Maintenance | Every control needs its own clamp | One scale factor controls everything |
| Fader geometry | Depends on vh — unpredictable | Fixed height, tick ruler always correct |

---

## Step-by-Step Plan

### Step 1 — Design lock (Nabeel)

Before any CSS refactor, Nabeel must finalise the Figma design:
- [ ] All section layouts confirmed (no pending restructuring)
- [ ] All component sizes locked (faders, dropdowns, selectors)
- [ ] Waveform icon variants complete (Sawtooth, Triangle)
- [ ] Keyboard row final decision (CSS or JUCE native)
- [ ] Pitch Bend / Mod Wheel UI decision

**Do not start Step 2 until the Figma design is stable.** Refactoring to fixed pixels while the design is still changing means double work.

---

### Step 2 — CSS: Replace all fluid units with Figma fixed pixels

Go through `styles.css` top to bottom and replace every `clamp()`, `vh`, and `vw` with the exact Figma pixel value. Remove all responsive machinery.

#### Design tokens to set (exact Figma values):
```css
:root {
    /* Typography */
    --text-title:   48px;
    --text-heading: 28px;
    --text-tag:     15px;
    --text-value:   20px;

    /* Root layout */
    --root-padding-v: 40px;
    --root-padding-h: 48px;
    --root-gap:       24px;

    /* Card */
    --card-padding-h: 24px;
    --card-padding-v: 24px;
    --card-gap:       24px;

    /* Slider */
    --slider-track-w:      8px;
    --slider-thumb-w:      20px;
    --slider-thumb-h:      24px;
    --slider-thumb-r:      8px;
    --slider-thumb-border: 4px;
}

/* Fader heights — exact Figma track dimensions */
.fader--standard .fader-input { height: 200px; }
.fader--big      .fader-input { height: 280px; }
.fader--small    .fader-input { height: 120px; }
```

Also replace all `flex: 3`, `flex: 1.5` etc. with explicit `width: Xpx` based on Figma frame widths. The Figma frames have exact widths that we should extract from the design tokens file.

#### Remove:
- All `clamp()` functions (replaced by fixed pixels above)
- All `vh`/`vw` usages in layout tokens (replaced by fixed pixels)
- The LFO/Voicing tight-spacing overrides (no longer needed with correct canvas height)
- The `min-width`/`max-width` floor values on sub-sections (replaced by explicit widths)

---

### Step 3 — Add the scale factor in JavaScript

Add this to `main.js` (or a new `resize.js` loaded before components mount):

```javascript
/* ============================================================================
 * Phase 3 — Uniform canvas scaling
 *
 * The .solace-root is designed at exactly 1440×1024 (Figma canvas).
 * When the plugin window is resized, we scale the entire root uniformly
 * so all proportions remain identical to the Figma design.
 *
 * The WebView itself always fills the full window width × (height - keyboardHeight).
 * We don't know keyboardHeight from JS, but since the WebView IS the canvas area,
 * window.innerHeight here = the usable WebView height = our target 1024px normally.
 * ============================================================================ */

const DESIGN_W = 1440;
const DESIGN_H = 1024;

function applyScale() {
    const scaleX = window.innerWidth  / DESIGN_W;
    const scaleY = window.innerHeight / DESIGN_H;
    // Use the smaller axis to guarantee no clipping (letterbox if aspect differs)
    const scale  = Math.min(scaleX, scaleY);

    const root = document.querySelector('.solace-root');
    root.style.transform       = `scale(${scale})`;
    root.style.transformOrigin = 'top left';

    // When scale < 1, the root is visually smaller than its DOM size.
    // We need to explicitly set root width/height so the page doesn't scroll.
    root.style.width  = `${DESIGN_W}px`;
    root.style.height = `${DESIGN_H}px`;
}

// Apply on load and whenever the window resizes (JUCE fires resize events on panel resize)
applyScale();
window.addEventListener('resize', applyScale);
```

> **Note on aspect ratio:** 1440/1024 ≈ 1.406. At 1440×1104 (our default window), the WebView is 1440×1024 exactly, so scale = 1.0 — no scaling applied. If the user drags the window smaller (future resizable mode), the scale factor drops proportionally.

---

### Step 4 — PluginEditor.cpp: Enable resizing

Once the scale system is in place, resizing becomes trivial on the C++ side:

```cpp
// In SolaceSynthEditor constructor (PluginEditor.cpp):
setResizable(true, false);     // user-resizable, not constrainable by host
setResizeLimits(720, 512,      // min: half of Figma canvas (720x512)
                2880, 2048);   // max: 2x Figma canvas (4K screens)
```

The JS resize handler takes care of scaling. No other C++ changes needed.

> **Optional:** Maintain aspect ratio on resize. In `resized()`, could enforce a 1440:1024 aspect ratio. But since `Math.min(scaleX, scaleY)` letterboxes anyway, this is cosmetic.

---

### Step 5 — Remove the Phase 2 clamp overrides

After Phase 3, the Phase 2 clamp-calibration CSS is redundant — everything is now fixed pixels. Remove:
- All `clamp()` in the `:root` block
- All `vh`/`vw` tokens
- LFO/Voicing tight-spacing overrides

---

## What to Verify After Phase 3

- [ ] At 1440×1104 (default): scale = 1.0, UI looks identical to Figma
- [ ] At 720×552 (half size): scale = 0.5, proportions preserved, no layout shift
- [ ] At 2880×2048 (2x size): scale = 2.0, UI fills correctly on large screen
- [ ] Fader tick ruler aligned at all scale factors (it will be, since it's fixed pixels)
- [ ] Dropdown popup still appears correctly at all scales (position: fixed panels need special handling — see note below)

> ⚠️ **Dropdown popup position note:** The floating `.dropdown-list` panel uses `position: fixed` placement calculated from `getBoundingClientRect()`. When the root is scaled via `transform: scale()`, `getBoundingClientRect()` returns visual coordinates (post-scale), so the positioning math in `dropdown.js` should continue to work correctly. **Verify this before marking Phase 3 complete.**

---

## Risk Assessment

| Risk | Severity | Mitigation |
|---|---|---|
| Dropdown popup misaligns at non-1.0 scale | Medium | Test at 0.5x and 2x; fix in dropdown.js if needed |
| Plugin host reports wrong window size | Low | JUCE handles this; hosts see the physical pixel size |
| Font rendering at scale < 1 looks blurry | Low | Browser sub-pixel rendering handles this well |
| Legacy designs drift during Nabeel's ongoing work | Medium | Lock design before starting (Step 1) |

---

## Timeline Estimate

- Step 1 (design lock): Nabeel's decision — not our timeline
- Steps 2-4: ~2-3 hours of focused work in one session
- Step 5 (cleanup): 30 minutes

**Total: One focused session once the design is locked.**

---

## Decision Checklist Before Starting

- [ ] Nabeel has confirmed design is final for V1
- [ ] Phase 1 + Phase 2 are live and verified for at least 1 week
- [ ] Resizable window min/max size range confirmed with Nabeel
- [ ] Dropdown popup scale-correctness tested in browser DevTools first
