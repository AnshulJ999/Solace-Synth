/* ============================================================================
 * Solace Synth — Dropdown Component
 * Phase 7.3
 *
 * Target selector control (LFO targets, velocity mod targets).
 * V1 behaviour: click cycles through options sequentially.
 * V2 / Phase 7.5: replace with a real popup list.
 *
 * Usage:
 *   const LFO_TARGETS = [
 *       { value: 0, label: 'None' },
 *       { value: 1, label: 'Filter - Cutoff' },
 *       ...
 *   ];
 *   new Dropdown('lfoTarget1', LFO_TARGETS, {
 *       ariaLabel:    'LFO Target 1',
 *       defaultIndex: 1,   // 'Filter - Cutoff'
 *   }).mount(document.getElementById('mount-lfoTarget1'));
 *
 * Dependencies:
 *   - SolaceBridge global (bridge.js must load first)
 *   - styles.css classes: .dropdown, .dropdown-label, .dropdown-arrow
 *   - assets/icons/arrow-dropdown.svg
 * ============================================================================ */

class Dropdown {
    /**
     * @param {string} paramId
     * @param {{ value: number, label: string }[]} options
     * @param {object} config
     * @param {string}  [config.ariaLabel]    - ARIA label for the dropdown element
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
     * @returns {Dropdown} this
     */
    mount(container) {
        if (!container) {
            console.warn(`[Dropdown] mount: no container for param "${this.paramId}"`);
            return this;
        }

        const { ariaLabel } = this.config;
        const initialLabel = this.options[this._currentIndex]?.label ?? '';

        const wrapper = document.createElement('div');
        wrapper.className = 'dropdown';
        wrapper.id        = `${this.paramId}-dropdown`;
        wrapper.setAttribute('role', 'button');
        wrapper.setAttribute('tabindex', '0');
        wrapper.setAttribute('aria-label', ariaLabel ?? this.paramId);

        const labelEl = document.createElement('span');
        labelEl.className   = 'dropdown-label';
        labelEl.id          = `${this.paramId}-value`;
        labelEl.textContent = initialLabel;
        wrapper.appendChild(labelEl);
        this._display = labelEl;

        const arrowImg = document.createElement('img');
        arrowImg.className = 'dropdown-arrow';
        arrowImg.src       = 'assets/icons/arrow-dropdown.svg';
        arrowImg.alt       = '';
        arrowImg.setAttribute('aria-hidden', 'true');
        wrapper.appendChild(arrowImg);

        container.appendChild(wrapper);

        const activate = () => {
            if (this._suppressSync) return;
            this._apply(this._currentIndex + 1);
            SolaceBridge.setParameter(this.paramId, this.options[this._currentIndex].value);
        };

        wrapper.addEventListener('click', activate);
        wrapper.addEventListener('keydown', (e) => {
            if (e.key === 'Enter' || e.key === ' ') {
                e.preventDefault();
                activate();
            }
        });

        SolaceBridge.onParameterChanged(this.paramId, (val) => {
            const idx = this.options.findIndex(o => o.value === Math.round(val));
            if (idx >= 0) {
                this._suppressSync = true;
                this._apply(idx);
                this._suppressSync = false;
            }
        });

        // Apply initial display state
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
}
