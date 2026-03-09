/* ============================================================================
 * Solace Synth — Main UI Logic
 * Phase 7.2: Layout Scaffold + Control Binding
 *
 * Responsibilities:
 *   - Initialize the bridge
 *   - Bind all fader controls to bridge parameters
 *   - Bind all arrow-selector controls to bridge parameters
 *   - Bind all dropdown controls to bridge parameters
 *   - Handle C++ → JS parameterChanged events to update all displays
 *   - Debug panel (Ctrl+Shift+D toggle)
 *   - Status indicator updates
 *
 * Source of truth: C++ APVTS. The UI only reflects and requests changes.
 *
 * Parameter naming: matches the APVTS IDs in Phase 6 DSP Roadmap.
 * Controls for DSP params not yet implemented have no effect until DSP catches up.
 * ============================================================================ */

(function () {
    "use strict";

    // =========================================================================
    // Log levels
    // =========================================================================
    const LOG = { TRACE: 0, DEBUG: 1, INFO: 2, WARN: 3, ERROR: 4 };
    const MAX_LOG_ENTRIES = 100;

    // =========================================================================
    // Waveform definitions (used by Osc1, Osc2, LFO selectors)
    // =========================================================================
    const OSC_WAVEFORMS = [
        { value: 0, label: "Sine",     icon: "assets/icons/waveform-sine-icon.svg",   alt: "Sinewave"     },
        { value: 1, label: "Sawtooth", icon: "assets/icons/waveform-sine-icon.svg",   alt: "Sawtooth"     }, // placeholder until Nabeel provides icon
        { value: 2, label: "Square",   icon: "assets/icons/waveform-square-icon.svg", alt: "Squarewave"   },
        { value: 3, label: "Triangle", icon: "assets/icons/waveform-sine-icon.svg",   alt: "Triangle"     }, // placeholder until Nabeel provides icon
    ];

    const LFO_WAVEFORMS = [
        { value: 0, label: "Sine",     icon: "assets/icons/waveform-sine-icon.svg",   alt: "Sinewave"   },
        { value: 1, label: "Triangle", icon: "assets/icons/waveform-sine-icon.svg",   alt: "Triangle"   },
        { value: 2, label: "Sawtooth", icon: "assets/icons/waveform-sine-icon.svg",   alt: "Sawtooth"   },
        { value: 3, label: "Square",   icon: "assets/icons/waveform-square-icon.svg", alt: "Squarewave" },
        { value: 4, label: "S&H",      icon: "assets/icons/waveform-sine-icon.svg",   alt: "Sample and Hold" },
    ];

    // =========================================================================
    // Filter type options
    // =========================================================================
    const FILTER_TYPES = [
        { value: 0, label: "LP 12 dB" },
        { value: 1, label: "LP 24 dB" },
        { value: 2, label: "HP 12 dB" },
    ];

    // =========================================================================
    // Dropdown option lists (display label → APVTS int value)
    // =========================================================================
    const LFO_TARGET_OPTIONS = [
        { value: 0, label: "None"               },
        { value: 1, label: "Filter - Cutoff"    },
        { value: 2, label: "Filter - Resonance" },
        { value: 3, label: "Osc 1 - Pitch"      },
        { value: 4, label: "Osc 2 - Pitch"      },
        { value: 5, label: "Osc 1 - Level"      },
        { value: 6, label: "Osc 2 - Level"      },
        { value: 7, label: "Amp - Level"         },
    ];

    const VEL_MOD_TARGET_OPTIONS = [
        { value: 0, label: "None"               },
        { value: 1, label: "Amp - Level"         },
        { value: 2, label: "Amp - Attack"        },
        { value: 3, label: "Filter - Cutoff"    },
        { value: 4, label: "Filter - Resonance" },
    ];

    // =========================================================================
    // State tracking (to avoid C++ echo loops)
    // =========================================================================
    let isUpdatingFromCpp = false;

    // =========================================================================
    // Logging
    // =========================================================================
    const debugLog = document.getElementById("debug-log");

    const log = (level, message) => {
        if (!debugLog) return;

        const levelLabels = { [LOG.TRACE]: "TRACE", [LOG.DEBUG]: "DEBUG", [LOG.INFO]: "INFO", [LOG.WARN]: "WARN", [LOG.ERROR]: "ERROR" };
        const levelCss    = { [LOG.TRACE]: "from-trace", [LOG.DEBUG]: "from-js", [LOG.INFO]: "from-cpp", [LOG.WARN]: "from-warn", [LOG.ERROR]: "error" };

        const entry = document.createElement("div");
        entry.className = `log-entry ${levelCss[level] || ""}`;
        const ts = new Date().toLocaleTimeString("en-US", { hour12: false, hour: "2-digit", minute: "2-digit", second: "2-digit", fractionalSecondDigits: 2 });
        entry.textContent = `[${ts}] [${levelLabels[level]}] ${message}`;
        debugLog.appendChild(entry);

        while (debugLog.children.length > MAX_LOG_ENTRIES) {
            debugLog.removeChild(debugLog.firstChild);
        }
        debugLog.scrollTop = debugLog.scrollHeight;

        if (level >= LOG.INFO && SolaceBridge.isJuceAvailable()) {
            SolaceBridge.log(`[${levelLabels[level]}] ${message}`);
        }
    };

    const logTrace = (m) => log(LOG.TRACE, m);
    const logDebug = (m) => log(LOG.DEBUG, m);
    const logInfo  = (m) => log(LOG.INFO,  m);
    const logWarn  = (m) => log(LOG.WARN,  m);
    const logError = (m) => log(LOG.ERROR, m);

    // =========================================================================
    // Status indicator
    // =========================================================================
    const statusDot  = document.getElementById("status-dot");
    const statusText = document.getElementById("status-text");

    const setStatus = (text, state = "disconnected") => {
        if (statusText) statusText.textContent = text;
        if (statusDot) {
            statusDot.className = "status-dot";
            if (state === "connected") statusDot.classList.add("status-dot--connected");
            if (state === "error")     statusDot.classList.add("status-dot--error");
        }
    };

    // =========================================================================
    // Debug panel: Ctrl+Shift+D toggle
    // =========================================================================
    const debugPanel = document.getElementById("debug-panel");

    document.addEventListener("keydown", (e) => {
        if (e.ctrlKey && e.shiftKey && e.key === "D") {
            e.preventDefault();
            debugPanel?.classList.toggle("debug-panel--visible");
        }
    });

    // =========================================================================
    // Generic fader binding
    // Connects an <input type="range"> to an APVTS param ID via the bridge.
    // formatFn: optional function to format the display value string.
    // =========================================================================
    const bindFader = (paramId, formatFn) => {
        const input   = document.getElementById(paramId);
        const display = document.getElementById(`${paramId}-value`);

        if (!input) {
            logWarn(`bindFader: element #${paramId} not found`);
            return;
        }

        const fmt = formatFn || ((v) => parseFloat(v).toFixed(2));

        // User → C++
        input.addEventListener("input", (e) => {
            if (isUpdatingFromCpp) return;
            const val = parseFloat(e.target.value);
            logTrace(`fader ${paramId} → ${val.toFixed(4)}`);
            if (display) display.textContent = fmt(val);
            SolaceBridge.setParameter(paramId, val);
        });

        // C++ → UI
        SolaceBridge.onParameterChanged(paramId, (val) => {
            logTrace(`C++ → ${paramId} = ${val}`);
            isUpdatingFromCpp = true;
            input.value = val;
            if (display) display.textContent = fmt(val);
            isUpdatingFromCpp = false;
        });
    };

    // =========================================================================
    // Waveform selector binding
    // Clicking the waveform icon cycles through available waveforms.
    // =========================================================================
    const bindWaveformSelector = (paramId, iconId, waveforms, defaultIndex = 0) => {
        const iconEl = document.getElementById(iconId);
        if (!iconEl) {
            logWarn(`bindWaveformSelector: #${iconId} not found`);
            return;
        }

        let currentIndex = defaultIndex;

        const apply = (index) => {
            currentIndex = ((index % waveforms.length) + waveforms.length) % waveforms.length;
            const wf = waveforms[currentIndex];
            iconEl.src = wf.icon;
            iconEl.alt = wf.alt;
            logTrace(`${paramId} waveform → ${wf.label} (${wf.value})`);
        };

        // Click cycles through waveforms
        iconEl.closest(".waveform-icon-row")?.addEventListener("click", () => {
            if (isUpdatingFromCpp) return;
            apply(currentIndex + 1);
            SolaceBridge.setParameter(paramId, waveforms[currentIndex].value);
        });

        iconEl.closest(".waveform-icon-row")?.addEventListener("keydown", (e) => {
            if (e.key === "Enter" || e.key === " ") {
                e.preventDefault();
                if (isUpdatingFromCpp) return;
                apply(currentIndex + 1);
                SolaceBridge.setParameter(paramId, waveforms[currentIndex].value);
            }
        });

        // C++ sync
        SolaceBridge.onParameterChanged(paramId, (val) => {
            isUpdatingFromCpp = true;
            apply(Math.round(val));
            isUpdatingFromCpp = false;
        });

        apply(defaultIndex);
    };

    // =========================================================================
    // Arrow selector binding (< value >)
    // =========================================================================
    const bindArrowSelector = (paramId, options, displayId, prevBtnId, nextBtnId, defaultIndex = 0) => {
        const display = document.getElementById(displayId);
        const prevBtn = document.getElementById(prevBtnId);
        const nextBtn = document.getElementById(nextBtnId);

        if (!display || !prevBtn || !nextBtn) {
            logWarn(`bindArrowSelector: missing elements for ${paramId}`);
            return;
        }

        let currentIndex = defaultIndex;

        const apply = (index) => {
            currentIndex = ((index % options.length) + options.length) % options.length;
            display.textContent = options[currentIndex].label;
        };

        prevBtn.addEventListener("click", () => {
            if (isUpdatingFromCpp) return;
            apply(currentIndex - 1);
            SolaceBridge.setParameter(paramId, options[currentIndex].value);
            logTrace(`${paramId} ← ${options[currentIndex].label}`);
        });

        nextBtn.addEventListener("click", () => {
            if (isUpdatingFromCpp) return;
            apply(currentIndex + 1);
            SolaceBridge.setParameter(paramId, options[currentIndex].value);
            logTrace(`${paramId} → ${options[currentIndex].label}`);
        });

        // C++ sync
        SolaceBridge.onParameterChanged(paramId, (val) => {
            const idx = options.findIndex(o => o.value === Math.round(val));
            if (idx >= 0) {
                isUpdatingFromCpp = true;
                apply(idx);
                isUpdatingFromCpp = false;
            }
        });

        apply(defaultIndex);
    };

    // =========================================================================
    // Dropdown binding
    // Clicking a dropdown cycles through options (full dropdown UI deferred to 7.3).
    // =========================================================================
    const bindDropdown = (paramId, options, dropdownId, displayId, defaultIndex = 0) => {
        const dropdown = document.getElementById(dropdownId);
        const display  = document.getElementById(displayId);

        if (!dropdown || !display) {
            logWarn(`bindDropdown: missing elements for ${paramId}`);
            return;
        }

        let currentIndex = defaultIndex;

        const apply = (index) => {
            currentIndex = ((index % options.length) + options.length) % options.length;
            display.textContent = options[currentIndex].label;
        };

        // Simple cycle-on-click for now (proper dropdown popup in Phase 7.3)
        dropdown.addEventListener("click", () => {
            if (isUpdatingFromCpp) return;
            apply(currentIndex + 1);
            SolaceBridge.setParameter(paramId, options[currentIndex].value);
            logTrace(`${paramId} → ${options[currentIndex].label}`);
        });

        dropdown.addEventListener("keydown", (e) => {
            if (e.key === "Enter" || e.key === " ") {
                e.preventDefault();
                if (isUpdatingFromCpp) return;
                apply(currentIndex + 1);
                SolaceBridge.setParameter(paramId, options[currentIndex].value);
            }
        });

        // C++ sync
        SolaceBridge.onParameterChanged(paramId, (val) => {
            const idx = options.findIndex(o => o.value === Math.round(val));
            if (idx >= 0) {
                isUpdatingFromCpp = true;
                apply(idx);
                isUpdatingFromCpp = false;
            }
        });

        apply(defaultIndex);
    };

    // =========================================================================
    // Frequency display formatter (20 → "20 Hz", 1000 → "1.00 kHz")
    // =========================================================================
    const fmtFreq = (v) => {
        if (v >= 1000) return `${(v / 1000).toFixed(2)}k`;
        return `${Math.round(v)}`;
    };

    // =========================================================================
    // Bind all controls
    // =========================================================================
    const bindAllControls = () => {

        // --- Oscillator 1 ---
        bindWaveformSelector("osc1Waveform", "osc1-waveform-icon", OSC_WAVEFORMS, 0);
        bindArrowSelector(
            "osc1Octave",
            [-3,-2,-1,0,1,2,3].map(v => ({ value: v, label: String(v) })),
            "osc1-octave-value", "osc1-octave-prev", "osc1-octave-next",
            3 // default: 0 (index 3 in the -3..+3 range)
        );
        bindArrowSelector(
            "osc1Transpose",
            Array.from({ length: 25 }, (_, i) => ({ value: i - 12, label: String(i - 12) })),
            "osc1-transpose-value", "osc1-transpose-prev", "osc1-transpose-next",
            12 // default: 0
        );
        bindFader("osc1Tuning", (v) => `${Math.round(v)}`);

        // --- Osc Mix ---
        bindFader("oscMix");

        // --- Oscillator 2 ---
        bindWaveformSelector("osc2Waveform", "osc2-waveform-icon", OSC_WAVEFORMS, 2); // default: Square (index 2)
        bindArrowSelector(
            "osc2Octave",
            [-3,-2,-1,0,1,2,3].map(v => ({ value: v, label: String(v) })),
            "osc2-octave-value", "osc2-octave-prev", "osc2-octave-next",
            4 // default: 1 (index 4 in -3..+3)
        );
        bindArrowSelector(
            "osc2Transpose",
            Array.from({ length: 25 }, (_, i) => ({ value: i - 12, label: String(i - 12) })),
            "osc2-transpose-value", "osc2-transpose-prev", "osc2-transpose-next",
            12 // default: 0
        );
        bindFader("osc2Tuning", (v) => `${Math.round(v)}`);

        // --- Amplifier Envelope ---
        bindFader("ampAttack",  (v) => parseFloat(v).toFixed(3));
        bindFader("ampDecay",   (v) => parseFloat(v).toFixed(3));
        bindFader("ampSustain");
        bindFader("ampRelease", (v) => parseFloat(v).toFixed(3));

        // --- Master ---
        bindFader("masterDistortion");
        bindFader("masterVolume");

        // --- Filter ---
        bindFader("filterCutoff",    fmtFreq);
        bindFader("filterResonance");

        // --- Filter Configuration ---
        bindArrowSelector(
            "filterType",
            FILTER_TYPES,
            "filterType-value", "filter-type-prev", "filter-type-next",
            1 // default: LP 24 dB
        );
        bindFader("filterEnvAttack",  (v) => parseFloat(v).toFixed(3));
        bindFader("filterEnvDecay",   (v) => parseFloat(v).toFixed(3));
        bindFader("filterEnvSustain");
        bindFader("filterEnvRelease", (v) => parseFloat(v).toFixed(3));

        // --- LFO ---
        bindWaveformSelector("lfoWaveform", "lfo-waveform-icon", LFO_WAVEFORMS, 0);
        bindFader("lfoAmount");
        bindFader("lfoRate", (v) => parseFloat(v).toFixed(2));
        bindDropdown("lfoTarget1", LFO_TARGET_OPTIONS, "lfoTarget1-dropdown", "lfoTarget1-value", 1); // default: Filter Cutoff
        bindDropdown("lfoTarget2", LFO_TARGET_OPTIONS, "lfoTarget2-dropdown", "lfoTarget2-value", 6); // default: Osc 2 Level
        bindDropdown("lfoTarget3", LFO_TARGET_OPTIONS, "lfoTarget3-dropdown", "lfoTarget3-value", 0); // default: None

        // --- Voicing ---
        bindArrowSelector(
            "voiceCount",
            Array.from({ length: 16 }, (_, i) => ({ value: i + 1, label: String(i + 1) })),
            "voiceCount-value", "voice-count-prev", "voice-count-next",
            15 // default: 16
        );
        bindArrowSelector(
            "unisonCount",
            Array.from({ length: 8 }, (_, i) => ({ value: i + 1, label: String(i + 1) })),
            "unisonCount-value", "unison-prev", "unison-next",
            2 // default: 3
        );
        bindFader("velocityRange");
        bindDropdown("velocityModTarget1", VEL_MOD_TARGET_OPTIONS, "velModTarget1-dropdown", "velocityModTarget1-value", 2); // default: Amp Attack
        bindDropdown("velocityModTarget2", VEL_MOD_TARGET_OPTIONS, "velModTarget2-dropdown", "velocityModTarget2-value", 0); // default: None

        logDebug("All controls bound");
    };

    // =========================================================================
    // Initialize
    // =========================================================================
    const init = async () => {
        logInfo("=== Solace Synth UI — Phase 7.2 ===");

        try {
            const bridgeOk = SolaceBridge.init();
            logInfo(`Bridge: ${bridgeOk ? "JUCE backend detected" : "browser preview mode (no backend)"}`);

            bindAllControls();

            if (!bridgeOk) {
                setStatus("preview mode", "disconnected");
                return;
            }

            setStatus("connecting...", "disconnected");

            const ready = await SolaceBridge.signalUiReady();
            logInfo(`uiReady response: ${ready}`);

            if (ready) {
                setStatus("connected", "connected");
                logInfo("Bridge handshake complete");
            } else {
                setStatus("bridge error", "error");
                logWarn("uiReady returned false — check C++ logs");
            }

        } catch (err) {
            console.error("[Solace] Init error:", err);
            logError(`Init failed: ${err.message}`);
            setStatus("error", "error");
        }
    };

    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", init);
    } else {
        init();
    }

})();
