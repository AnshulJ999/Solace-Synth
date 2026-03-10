# Dropdown Popup Implementation Plan
*Phase 7.5 — written 2026-03-11*

## What this solves
`Dropdown` component currently click-to-cycles through options. For LFO targets (8 options) and velocity mod targets (5 options), this is bad UX. Needs a real popup list.

## Scope
Only these components need a real dropdown:
- `lfoTarget1`, `lfoTarget2`, `lfoTarget3` (LFO section, 8 options each)
- `velocityModTarget1`, `velocityModTarget2` (Voicing section, 5 options each)

**NOT changing:** `ArrowSelector` components (voiceCount, unisonCount, filterType, octave, transpose — these are fine as < N > style or arrow selectors).

## Implementation — dropdown.js

### DOM Structure
```
.dropdown (trigger row — keeps existing CSS)
  .dropdown-label   ← current value text
  .dropdown-arrow   ← existing chevron SVG

+ [Singleton panel attached to <body>]
.dropdown-list (position:fixed, z-index:9999)
  .dropdown-option × N   ← one per option
  .dropdown-option--selected  ← current value highlighted
```

### Behaviour
1. Click trigger → compute position via `getBoundingClientRect()`, set panel position to open **upward** (these controls are all in the bottom row):
   ```js
   const rect = this._trigger.getBoundingClientRect();
   panel.style.bottom = `${window.innerHeight - rect.top + 4}px`;
   panel.style.left   = `${rect.left}px`;
   panel.style.width  = `${rect.width}px`;
   ```
2. Panel shows all options. Selected option has accent colour.
3. Click option → update value, send to C++, close panel.
4. Click outside panel → close.
5. `Escape` key → close.
6. Only one panel open at a time (shared singleton).

### State changes in Dropdown class
- Add `_isOpen` boolean
- Change click handler: toggle panel instead of cycling
- Add `_openPanel()` / `_closePanel()` / `_createPanel()` methods
- `onParameterChanged` still works the same (updates label text)

### CSS additions (styles.css)
```css
.dropdown-list {
    position: fixed;
    z-index: 9999;
    background: var(--color-bg);
    border: var(--card-border);
    border-radius: var(--card-radius);
    box-shadow: 0 4px 16px rgba(0,0,0,0.12);
    overflow: hidden;
    min-width: 120px;
}

.dropdown-option {
    padding: var(--space-sm) var(--space-md);
    font-family: var(--font-value);
    font-size: var(--text-value);
    color: var(--color-text);
    cursor: pointer;
    border-bottom: 1px solid var(--color-border);
    transition: background 0.08s ease;
}

.dropdown-option:last-child { border-bottom: none; }
.dropdown-option:hover { background: var(--color-border); }
.dropdown-option--selected {
    color: var(--color-accent);
    font-weight: 600;
}
```

## Estimated effort
~80 lines of JS changes to `dropdown.js` + ~30 lines CSS. Medium complexity.

## Notes
- The singleton panel approach avoids z-index and overflow clipping issues
- Always open upward for now (all affected controls are in the bottom row)
- Future: add smart position detection (open down if room, up if not)
- The click-to-cycle fallback is removed once real dropdown is implemented
