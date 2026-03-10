/* ============================================================================
 * Solace Synth — Fader Component
 * Phase 7.3
 *
 * Self-contained vertical slider bound to one APVTS parameter.
 *
 * Usage:
 *   new Fader('ampAttack', {
 *       label:        'Attack',
 *       min:          0.001,
 *       max:          5,
 *       step:         0.001,
 *       defaultValue: 0.01,
 *       formatFn:     v => parseFloat(v).toFixed(3),
 *       sizeClass:    'fader--standard',   // 'fader--big' | 'fader--small'
 *       bottomLabel:  null,                // optional second label below value (Osc Mix)
 *       extraClass:   null,                // optional extra class e.g. 'fader--bipolar'
 *   }).mount(document.getElementById('mount-ampAttack'));
 *
 * Dependencies:
 *   - SolaceBridge global (bridge.js must load first)
 *   - styles.css classes: .fader, .fader--*, .fader-label, .fader-input, .fader-value
 * ============================================================================ */

class Fader {
    /**
     * @param {string} paramId - Exact APVTS parameter ID
     * @param {object} config
     * @param {string}   config.label        - Primary label (shown above fader)
     * @param {number}   config.min
     * @param {number}   config.max
     * @param {number}   config.step
     * @param {number}   config.defaultValue
     * @param {Function} [config.formatFn]   - Value formatter fn(v) → string
     * @param {string}   [config.sizeClass]  - 'fader--standard' | 'fader--big' | 'fader--small'
     * @param {string}   [config.bottomLabel]- Optional second label below value (Osc Mix)
     * @param {string}   [config.extraClass] - Optional additional CSS class
     */
    constructor(paramId, config) {
        this.paramId = paramId;
        this.config  = config;
        this._input         = null;
        this._display       = null;
        this._wrapper       = null;
        this._suppressSync  = false;
        this._fmt = config.formatFn ?? ((v) => parseFloat(v).toFixed(2));
    }

    /**
     * Creates DOM and appends into container. Registers bridge listeners.
     * @param {HTMLElement} container
     * @returns {Fader} this (chainable)
     */
    mount(container) {
        if (!container) {
            console.warn(`[Fader] mount: no container for param "${this.paramId}"`);
            return this;
        }

        const {
            label,
            min,
            max,
            step,
            defaultValue,
            sizeClass   = 'fader--standard',
            bottomLabel = null,
            extraClass  = null,
            // showTicks: true = show ruler tick strip to the right of track (all faders per Figma, Phase 7.4)
            // Set showTicks: false in config to suppress.
            showTicks   = true,
        } = this.config;

        const classes = ['fader', sizeClass, extraClass].filter(Boolean).join(' ');

        const wrapper = document.createElement('div');
        wrapper.className = classes;
        wrapper.id = `${this.paramId}-fader`;
        this._wrapper = wrapper;

        // Top label
        const topLabelEl = document.createElement('span');
        topLabelEl.className = 'fader-label';
        topLabelEl.textContent = label;
        wrapper.appendChild(topLabelEl);

        // Range input
        const input = document.createElement('input');
        input.className  = 'fader-input';
        input.id         = this.paramId;
        input.type       = 'range';
        input.min        = String(min);
        input.max        = String(max);
        input.step       = String(step);
        input.value      = String(defaultValue);
        input.setAttribute('aria-label', label);
        this._input = input;

        // Track area: input + optional ruler, laid side-by-side (flex-row)
        const trackArea = document.createElement('div');
        trackArea.className = 'fader-track-area';
        trackArea.appendChild(input);

        // Tick ruler — narrow strip RIGHT of the track (Figma spec, Phase 7.4)
        if (showTicks) {
            const ruler = document.createElement('div');
            ruler.className = 'fader-ticks-ruler';
            trackArea.appendChild(ruler);
        }

        wrapper.appendChild(trackArea);

        // Value readout
        const displayEl = document.createElement('span');
        displayEl.className = 'fader-value';
        displayEl.id        = `${this.paramId}-value`;
        displayEl.textContent = this._fmt(defaultValue);
        wrapper.appendChild(displayEl);
        this._display = displayEl;

        // Optional bottom label (Osc Mix crossfader)
        if (bottomLabel) {
            const botLabelEl = document.createElement('span');
            botLabelEl.className = 'fader-label';
            botLabelEl.textContent = bottomLabel;
            wrapper.appendChild(botLabelEl);
        }

        container.appendChild(wrapper);

        // User interaction → send to C++
        input.addEventListener('input', (e) => {
            if (this._suppressSync) return;
            const val = parseFloat(e.target.value);
            if (this._display) this._display.textContent = this._fmt(val);
            this._updateFill(val);
            SolaceBridge.setParameter(this.paramId, val);
        });

        // C++ → update display (won't re-trigger the input listener)
        SolaceBridge.onParameterChanged(this.paramId, (val) => {
            this._suppressSync = true;
            if (this._input)   this._input.value = val;
            if (this._display) this._display.textContent = this._fmt(val);
            this._updateFill(val);
            this._suppressSync = false;
        });

        // Initialise fill from defaultValue
        this._updateFill(defaultValue);

        return this;
    }

    /**
     * Programmatic value update (e.g. preset load) without sending to C++.
     * @param {number} v
     */
    setValue(v) {
        if (this._input)   this._input.value = v;
        if (this._display) this._display.textContent = this._fmt(v);
        this._updateFill(v);
    }

    /**
     * Sets --val-percent CSS custom property on the fader wrapper.
     * Value 0% = fully bottom, 100% = fully top.
     *
     * CSS usage (Phase 7.4):
     *   .fader-input::-webkit-slider-runnable-track {
     *       background: linear-gradient(
     *           to top,
     *           var(--color-fill-track) var(--val-percent),
     *           var(--color-slider-track) var(--val-percent)
     *       );
     *   }
     */
    _updateFill(val) {
        if (!this._wrapper) return;
        const { min, max } = this.config;
        const pct = ((parseFloat(val) - min) / (max - min)) * 100;
        this._wrapper.style.setProperty('--val-percent', `${Math.min(100, Math.max(0, pct)).toFixed(2)}%`);
    }
}
