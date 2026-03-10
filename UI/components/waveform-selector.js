/* ============================================================================
 * Solace Synth — WaveformSelector Component
 * Phase 7.3
 *
 * Displays the current waveform as an icon. Supports two interaction modes:
 *   - Click-to-cycle: clicking the icon advances to the next waveform (always on)
 *   - Arrow mode:     optional < > buttons flank the icon (showArrows: true)
 *
 * showArrows should be false for Oscillator 1/2 sections (too tight for arrows)
 * and true for the LFO section (more space, Figma shows flanking arrows).
 *
 * Usage:
 *   const OSC_WAVEFORMS = [
 *       { value: 0, label: 'Sine',     icon: 'assets/icons/waveform-sine-icon.svg',  alt: 'Sine wave' },
 *       { value: 1, label: 'Sawtooth', icon: 'assets/icons/waveform-sawtooth-icon.svg', alt: 'Sawtooth wave' },
 *       ...
 *   ];
 *   new WaveformSelector('osc1Waveform', OSC_WAVEFORMS, {
 *       defaultIndex: 0,
 *       showArrows:   false,
 *   }).mount(document.getElementById('mount-osc1Waveform'));
 *
 * Dependencies:
 *   - SolaceBridge global (bridge.js must load first)
 *   - styles.css classes: .waveform-selector, .waveform-selector-label,
 *                         .waveform-icon-row, .waveform-icon, .arrow-selector-btn
 *   - assets/icons/chevron-back.svg, chevron-forward.svg (if showArrows: true)
 * ============================================================================ */

class WaveformSelector {
    /**
     * @param {string} paramId
     * @param {{ value: number, label: string, icon: string, alt: string }[]} waveforms
     * @param {object} config
     * @param {number}  [config.defaultIndex] - Starting waveform index
     * @param {boolean} [config.showArrows]   - true = flanking < > buttons; false = icon-only
     */
    constructor(paramId, waveforms, config) {
        this.paramId       = paramId;
        this.waveforms     = waveforms;
        this.config        = config;
        this._currentIndex = config.defaultIndex ?? 0;
        this._iconEl       = null;
        this._suppressSync = false;
    }

    /**
     * @param {HTMLElement} container
     * @returns {WaveformSelector} this
     */
    mount(container) {
        if (!container) {
            console.warn(`[WaveformSelector] mount: no container for param "${this.paramId}"`);
            return this;
        }

        const showArrows = this.config.showArrows ?? false;

        const wrapper = document.createElement('div');
        wrapper.className = 'waveform-selector';
        wrapper.id        = `${this.paramId}-waveform-selector`;

        // Label
        const labelEl = document.createElement('span');
        labelEl.className   = 'waveform-selector-label';
        labelEl.textContent = 'Waveform';
        wrapper.appendChild(labelEl);

        // Icon row  (always flex row; arrow buttons added optionally)
        const iconRow = document.createElement('div');
        iconRow.className = 'waveform-icon-row';
        iconRow.id        = `${this.paramId}-waveform-row`;

        let prevBtn = null;
        let nextBtn = null;

        if (showArrows) {
            prevBtn = this._makeBtn('Previous waveform', 'assets/icons/chevron-back.svg');
            iconRow.appendChild(prevBtn);
        }

        // Waveform icon (click-to-cycle always active)
        const iconEl = document.createElement('img');
        iconEl.className = 'waveform-icon';
        iconEl.id        = `${this.paramId}-waveform-icon`;
        iconEl.setAttribute('role', 'button');
        iconEl.setAttribute('tabindex', '0');
        iconEl.setAttribute('aria-label', 'Click to cycle waveform');
        iconRow.appendChild(iconEl);
        this._iconEl = iconEl;

        if (showArrows) {
            nextBtn = this._makeBtn('Next waveform', 'assets/icons/chevron-forward.svg');
            iconRow.appendChild(nextBtn);
        }

        wrapper.appendChild(iconRow);
        container.appendChild(wrapper);

        // Click on icon → advance forward (next)
        iconEl.addEventListener('click', () => {
            if (this._suppressSync) return;
            this._go(this._currentIndex + 1);
        });

        iconEl.addEventListener('keydown', (e) => {
            if (e.key === 'Enter' || e.key === ' ') {
                e.preventDefault();
                if (!this._suppressSync) this._go(this._currentIndex + 1);
            }
        });

        // Arrow buttons (if rendered)
        if (prevBtn) {
            prevBtn.addEventListener('click', () => {
                if (!this._suppressSync) this._go(this._currentIndex - 1);
            });
        }
        if (nextBtn) {
            nextBtn.addEventListener('click', () => {
                if (!this._suppressSync) this._go(this._currentIndex + 1);
            });
        }

        // C++ → update
        SolaceBridge.onParameterChanged(this.paramId, (val) => {
            this._suppressSync = true;
            this._apply(Math.round(val));
            this._suppressSync = false;
        });

        // Apply initial state (display only, no bridge call)
        this._apply(this._currentIndex);

        return this;
    }

    /** Programmatic update — no bridge send. */
    setValue(v) {
        this._apply(Math.round(v));
    }

    // -------------------------------------------------------------------------
    // Private
    // -------------------------------------------------------------------------

    /** Move to index and send to C++. */
    _go(index) {
        this._apply(index);
        SolaceBridge.setParameter(this.paramId, this.waveforms[this._currentIndex].value);
    }

    /** Update display only — does not send to C++. */
    _apply(index) {
        const len = this.waveforms.length;
        this._currentIndex = ((index % len) + len) % len;
        const wf = this.waveforms[this._currentIndex];
        if (this._iconEl) {
            this._iconEl.src = wf.icon;
            this._iconEl.alt = wf.alt;
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
