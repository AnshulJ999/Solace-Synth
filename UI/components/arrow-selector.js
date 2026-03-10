/* ============================================================================
 * Solace Synth — ArrowSelector Component
 * Phase 7.3
 *
 * Discrete enum / integer parameter control.
 * Renders as:  [Label]
 *              < current-label >
 * Left arrow decrements (wraps), right arrow increments (wraps).
 *
 * Usage:
 *   const OCTAVE_OPTIONS = [-3,-2,-1,0,1,2,3].map(v => ({ value: v, label: String(v) }));
 *   new ArrowSelector('osc1Octave', OCTAVE_OPTIONS, {
 *       label:        'Octave',
 *       ariaLabel:    'Oscillator 1 octave',
 *       defaultIndex: 3,   // index into options array (value 0)
 *   }).mount(document.getElementById('mount-osc1Octave'));
 *
 * Dependencies:
 *   - SolaceBridge global (bridge.js must load first)
 *   - styles.css classes: .arrow-selector, .arrow-selector-label,
 *                         .arrow-selector-row, .arrow-selector-btn,
 *                         .arrow-selector-value
 *   - assets/icons/chevron-back.svg, chevron-forward.svg
 * ============================================================================ */

class ArrowSelector {
    /**
     * @param {string} paramId
     * @param {{ value: number, label: string }[]} options
     * @param {object} config
     * @param {string}  config.label        - Displayed label above the selector
     * @param {string}  [config.ariaLabel]  - ARIA label for the group (defaults to label)
     * @param {number}  [config.defaultIndex] - Starting index in options[]
     */
    constructor(paramId, options, config) {
        this.paramId       = paramId;
        this.options       = options;
        this.config        = config;
        this._currentIndex = config.defaultIndex ?? 0;
        this._display      = null;
        this._suppressSync = false;
    }

    /**
     * @param {HTMLElement} container
     * @returns {ArrowSelector} this
     */
    mount(container) {
        if (!container) {
            console.warn(`[ArrowSelector] mount: no container for param "${this.paramId}"`);
            return this;
        }

        const { label, ariaLabel } = this.config;
        const initialLabel = this.options[this._currentIndex]?.label ?? '';

        const wrapper = document.createElement('div');
        wrapper.className = 'arrow-selector';
        wrapper.id        = `${this.paramId}-selector`;
        wrapper.setAttribute('role', 'group');
        wrapper.setAttribute('aria-label', ariaLabel ?? label);

        // Label
        const labelEl = document.createElement('span');
        labelEl.className   = 'arrow-selector-label';
        labelEl.textContent = label;
        wrapper.appendChild(labelEl);

        // Row: [ < ] [ value ] [ > ]
        const row = document.createElement('div');
        row.className = 'arrow-selector-row';

        const prevBtn = this._makeBtn('Previous ' + label, 'assets/icons/chevron-back.svg');
        const valueEl = document.createElement('span');
        valueEl.className   = 'arrow-selector-value';
        valueEl.id          = `${this.paramId}-value`;
        valueEl.textContent = initialLabel;
        const nextBtn = this._makeBtn('Next ' + label, 'assets/icons/chevron-forward.svg');

        row.appendChild(prevBtn);
        row.appendChild(valueEl);
        row.appendChild(nextBtn);
        wrapper.appendChild(row);

        container.appendChild(wrapper);
        this._display = valueEl;

        // Wire buttons
        prevBtn.addEventListener('click', () => {
            if (this._suppressSync) return;
            this._apply(this._currentIndex - 1);
            SolaceBridge.setParameter(this.paramId, this.options[this._currentIndex].value);
        });

        nextBtn.addEventListener('click', () => {
            if (this._suppressSync) return;
            this._apply(this._currentIndex + 1);
            SolaceBridge.setParameter(this.paramId, this.options[this._currentIndex].value);
        });

        // C++ → update
        SolaceBridge.onParameterChanged(this.paramId, (val) => {
            const idx = this.options.findIndex(o => o.value === Math.round(val));
            if (idx >= 0) {
                this._suppressSync = true;
                this._apply(idx);
                this._suppressSync = false;
            }
        });

        // Apply initial state (display update only, no bridge call)
        this._apply(this._currentIndex);

        return this;
    }

    /** Programmatic update — no bridge send. */
    setValue(v) {
        const idx = this.options.findIndex(o => o.value === Math.round(v));
        if (idx >= 0) this._apply(idx);
    }

    // -------------------------------------------------------------------------
    // Private
    // -------------------------------------------------------------------------

    _apply(index) {
        const len = this.options.length;
        this._currentIndex = ((index % len) + len) % len;
        if (this._display) {
            this._display.textContent = this.options[this._currentIndex].label;
        }
    }

    _makeBtn(ariaLabel, iconSrc) {
        const btn = document.createElement('button');
        btn.className = 'arrow-selector-btn';
        btn.setAttribute('aria-label', ariaLabel);
        btn.type = 'button';
        const img = document.createElement('img');
        img.src  = iconSrc;
        img.alt  = '';
        img.setAttribute('aria-hidden', 'true');
        btn.appendChild(img);
        return btn;
    }
}
