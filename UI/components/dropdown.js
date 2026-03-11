/* ============================================================================
 * Solace Synth — Dropdown Component
 * Phase 7.5 — Real popup list implementation
 *
 * Replaces the Phase 7.3 click-to-cycle behaviour with a proper floating panel.
 * A singleton panel element is shared across all Dropdown instances and
 * re-populated each time a dropdown is opened. This avoids z-index and
 * overflow-clipping problems that arise when each dropdown owns its own panel.
 *
 * Interaction contract:
 *   - Click trigger (or Enter / Space whilst focused) → open popup above trigger.
 *   - Click an option → select it, send value to C++, close popup.
 *   - Click outside the popup → close popup.
 *   - Escape key → close popup without changing selection.
 *   - Only one popup can be open at a time; opening a second closes the first.
 *
 * Usage:
 *   const LFO_TARGETS = [
 *       { value: 0, label: 'None' },
 *       { value: 1, label: 'Filter - Cutoff' },
 *       ...
 *   ];
 *   new Dropdown('lfoTarget1', LFO_TARGETS, {
 *       ariaLabel:    'LFO Target 1',
 *       defaultIndex: 1,   // starts on 'Filter - Cutoff'
 *   }).mount(document.getElementById('mount-lfoTarget1'));
 *
 * Dependencies:
 *   - SolaceBridge global (bridge.js must load first)
 *   - styles.css: .dropdown, .dropdown-label, .dropdown-arrow,
 *                 .dropdown-list, .dropdown-option, .dropdown-option--selected
 *   - assets/icons/arrow-dropdown.svg
 * ============================================================================ */

// ============================================================================
// Module-level singleton panel — one panel shared by all Dropdown instances.
// Created lazily on first open; never removed from the DOM.
// ============================================================================

/** @type {HTMLElement|null} */
let _sharedPanel = null;

/** @type {Dropdown|null} Currently-open Dropdown instance */
let _activeInstance = null;

/**
 * Lazily create and return the shared popup panel element.
 * The panel is appended to <body> exactly once.
 * @returns {HTMLElement}
 */
function _getSharedPanel () {
    if (_sharedPanel) return _sharedPanel;

    _sharedPanel = document.createElement ('div');
    _sharedPanel.className   = 'dropdown-list';
    _sharedPanel.id          = 'dropdown-list-panel';
    _sharedPanel.setAttribute ('role', 'listbox');
    document.body.appendChild (_sharedPanel);
    return _sharedPanel;
}

/**
 * Close the currently-open popup (if any).
 * Safe to call when no popup is open.
 */
function _closeActive () {
    // Remove global listeners before anything else so they don't fire again.
    document.removeEventListener ('pointerup', _onOutsidePointerUp);
    document.removeEventListener ('keydown',   _onEscapeKey);

    if (_activeInstance) {
        _activeInstance._isOpen = false;
        _activeInstance._trigger && _activeInstance._trigger.setAttribute ('aria-expanded', 'false');
        _activeInstance = null;
    }

    if (_sharedPanel) {
        _sharedPanel.style.display = 'none';
        // innerHTML cleared on next open so DOM references don't linger
    }
}

/**
 * Document-level pointerup handler: closes the popup when the user clicks
 * outside both the panel and the active trigger button.
 * Uses bubble phase (no capture) so option pointerup handlers can call
 * stopPropagation() to prevent this from also firing.
 * @param {PointerEvent} e
 */
function _onOutsidePointerUp (e) {
    if (_sharedPanel && _sharedPanel.contains (e.target)) return;
    if (_activeInstance && _activeInstance._trigger &&
        _activeInstance._trigger.contains (e.target)) return;
    _closeActive();
}

/**
 * Document-level keydown handler: closes on Escape.
 * @param {KeyboardEvent} e
 */
function _onEscapeKey (e) {
    if (e.key === 'Escape') {
        e.preventDefault();
        _closeActive();
    }
}

// ============================================================================
// Dropdown class
// ============================================================================

class Dropdown {
    /**
     * @param {string} paramId - Exact APVTS parameter ID
     * @param {{ value: number, label: string }[]} options
     * @param {object} config
     * @param {string} [config.ariaLabel]    - ARIA label for the trigger element
     * @param {number} [config.defaultIndex] - Initial selected index in options[]
     */
    constructor (paramId, options, config) {
        this.paramId       = paramId;
        this.options       = options;
        this.config        = config;
        this._currentIndex = config.defaultIndex ?? 0;
        this._isOpen       = false;

        /** @type {HTMLElement|null} The visible trigger row (the ".dropdown" div) */
        this._trigger      = null;

        /** @type {HTMLElement|null} The label span inside the trigger */
        this._display      = null;

        this._suppressSync = false;
    }

    /**
     * Build DOM and register all event listeners.
     * @param {HTMLElement} container
     * @returns {Dropdown} this (chainable)
     */
    mount (container) {
        if (!container) {
            console.warn ('[Dropdown] mount: no container for param "' + this.paramId + '"');
            return this;
        }

        const { ariaLabel } = this.config;

        // --- Trigger row (".dropdown") ---
        const wrapper = document.createElement ('div');
        wrapper.className = 'dropdown';
        wrapper.id        = this.paramId + '-dropdown';
        wrapper.setAttribute ('role',          'button');
        wrapper.setAttribute ('tabindex',       '0');
        wrapper.setAttribute ('aria-haspopup', 'listbox');
        wrapper.setAttribute ('aria-expanded',  'false');
        wrapper.setAttribute ('aria-label',     ariaLabel ?? this.paramId);
        this._trigger = wrapper;

        // Label span showing current selection
        const labelEl = document.createElement ('span');
        labelEl.className   = 'dropdown-label';
        labelEl.id          = this.paramId + '-value';
        labelEl.textContent = this.options[this._currentIndex]?.label ?? '';
        wrapper.appendChild (labelEl);
        this._display = labelEl;

        // Chevron icon
        const arrowImg = document.createElement ('img');
        arrowImg.className = 'dropdown-arrow';
        arrowImg.src       = 'assets/icons/arrow-dropdown.svg';
        arrowImg.alt       = '';
        arrowImg.setAttribute ('aria-hidden', 'true');
        wrapper.appendChild (arrowImg);

        container.appendChild (wrapper);

        // --- Trigger interaction ---
        const activate = () => {
            if (this._suppressSync) return;
            if (this._isOpen) {
                _closeActive();
            } else {
                this._openPanel();
            }
        };

        wrapper.addEventListener ('click', activate);
        wrapper.addEventListener ('keydown', (e) => {
            if (e.key === 'Enter' || e.key === ' ') {
                e.preventDefault();
                activate();
            }
        });

        // --- Bridge: C++ → UI sync ---
        SolaceBridge.onParameterChanged (this.paramId, (val) => {
            const idx = this.options.findIndex (o => o.value === Math.round (val));
            if (idx >= 0) {
                this._suppressSync = true;
                this._applyIndex (idx);
                this._suppressSync = false;
            }
        });

        // Apply initial display
        this._applyIndex (this._currentIndex);

        return this;
    }

    /**
     * Programmatic value update from preset load etc. — does not send to C++.
     * @param {number} v  Raw parameter value (not index)
     */
    setValue (v) {
        const idx = this.options.findIndex (o => o.value === Math.round (v));
        if (idx >= 0) this._applyIndex (idx);
    }

    // =========================================================================
    // Private
    // =========================================================================

    /**
     * Open the shared popup panel positioned above (or below) the trigger.
     * Populates the panel with this dropdown's options.
     */
    _openPanel () {
        // Close any other open dropdown first
        if (_activeInstance && _activeInstance !== this) {
            _closeActive();
        }

        const panel = _getSharedPanel();

        // Populate with fresh option elements
        panel.innerHTML = '';
        this.options.forEach ((opt, idx) => {
            const item = document.createElement ('div');
            item.className   = 'dropdown-option' +
                               (idx === this._currentIndex ? ' dropdown-option--selected' : '');
            item.setAttribute ('role',          'option');
            item.setAttribute ('aria-selected', String (idx === this._currentIndex));
            item.textContent = opt.label;

            // Use pointerup so stopPropagation() blocks the document-level
            // bubble handler from also firing on the same event.
            item.addEventListener ('pointerup', (e) => {
                e.stopPropagation();
                this._selectOption (idx);
                _closeActive();
            });

            panel.appendChild (item);
        });

        // --- Position: prefer upward (controls are in bottom row) ---
        // Fall back to downward if there isn't enough room above.
        const rect        = this._trigger.getBoundingClientRect();
        const panelWidth  = Math.max (rect.width, 140);
        const panelHeight = Math.min (this.options.length * 34, 240);  // rough estimate

        panel.style.left  = rect.left + 'px';
        panel.style.width = panelWidth + 'px';

        if (rect.top > panelHeight + 8) {
            // Room above: open upward
            panel.style.bottom = (window.innerHeight - rect.top + 4) + 'px';
            panel.style.top    = '';
        } else {
            // Not enough room above: fall back to downward
            panel.style.top    = (rect.bottom + 4) + 'px';
            panel.style.bottom = '';
        }

        panel.style.display = 'block';

        // Update trigger ARIA state
        this._trigger.setAttribute ('aria-expanded', 'true');

        // Mark ourselves as the active instance
        _activeInstance = this;
        this._isOpen    = true;

        // Register global listeners.
        // Use requestAnimationFrame to avoid catching the same pointerup that
        // opened the panel (the click event fires before the listener is attached
        // at document level in the bubble phase, but rAF guarantees we skip it).
        requestAnimationFrame (() => {
            document.addEventListener ('pointerup', _onOutsidePointerUp);
            document.addEventListener ('keydown',   _onEscapeKey);
        });
    }

    /**
     * Called when the user picks an option from the popup.
     * Updates local state, sends value to C++.
     * @param {number} idx  Index into this.options[]
     */
    _selectOption (idx) {
        if (idx < 0 || idx >= this.options.length) return;
        this._applyIndex (idx);
        SolaceBridge.setParameter (this.paramId, this.options[idx].value);
    }

    /**
     * Update internal index and label display without sending to C++.
     * @param {number} idx
     */
    _applyIndex (idx) {
        if (idx < 0 || idx >= this.options.length) return;
        this._currentIndex = idx;
        if (this._display) {
            this._display.textContent = this.options[idx].label;
        }
    }
}
