/* ============================================================================
 * Solace Synth — Main UI Logic
 * Phase 7.2 rev 2: Fixed LFO enum ordering, added filterEnvDepth, fixed defaults
 *
 * Responsibilities:
 *   - Bridge initialization
 *   - Bind all parameter controls (faders, arrow selectors, waveform selectors, dropdowns)
 *   - Handle C++ → JS parameterChanged events to keep UI in sync
 *   - Debug panel toggle (Ctrl+Shift+D)
 *   - Status indicator updates
 *
 * Source of truth: C++ APVTS. JS reflects and requests — never owns state.
 *
 * LFO target enum (verified against synth-project-memory.md line 397):
 *   0=None, 1=FilterCutoff, 2=Osc1Pitch, 3=Osc2Pitch,
 *   4=Osc1Level, 5=Osc2Level, 6=AmpLevel, 7=FilterResonance
 *
 * Velocity mod target enum (verified against synth-project-memory.md line 398):
 *   0=None, 1=AmpLevel, 2=AmpAttack, 3=FilterCutoff, 4=FilterResonance
 * ============================================================================ */

(function () {
    "use strict";

    // =========================================================================
    // Constants
    // =========================================================================
    const LOG = { TRACE: 0, DEBUG: 1, INFO: 2, WARN: 3, ERROR: 4 };
    const MAX_LOG_ENTRIES = 100;

    // =========================================================================
    // Waveform icon definitions
    // Sawtooth + Triangle now have proper SVGs (still placeholders until Nabeel
    // provides the Figma originals, but they render correctly).
    // =========================================================================
    const OSC_WAVEFORMS = [
        { value: 0, label: "Sine",     icon: "assets/icons/waveform-sine-icon.svg",     alt: "Sine wave"     },
        { value: 1, label: "Sawtooth", icon: "assets/icons/waveform-sawtooth-icon.svg", alt: "Sawtooth wave" },
        { value: 2, label: "Square",   icon: "assets/icons/waveform-square-icon.svg",   alt: "Square wave"   },
        { value: 3, label: "Triangle", icon: "assets/icons/waveform-triangle-icon.svg", alt: "Triangle wave" },
    ];

    const LFO_WAVEFORMS = [
        { value: 0, label: "Sine",     icon: "assets/icons/waveform-sine-icon.svg",     alt: "Sine wave"       },
        { value: 1, label: "Triangle", icon: "assets/icons/waveform-triangle-icon.svg", alt: "Triangle wave"   },
        { value: 2, label: "Sawtooth", icon: "assets/icons/waveform-sawtooth-icon.svg", alt: "Sawtooth wave"   },
        { value: 3, label: "Square",   icon: "assets/icons/waveform-square-icon.svg",   alt: "Square wave"     },
        { value: 4, label: "S&H",      icon: "assets/icons/waveform-sh-icon.svg",       alt: "Sample and Hold" },
    ];

    // =========================================================================
    // Enum option lists — values match APVTS integer parameters exactly
    // =========================================================================
    const FILTER_TYPES = [
        { value: 0, label: "LP 12 dB" },
        { value: 1, label: "LP 24 dB" },
        { value: 2, label: "HP 12 dB" },
    ];

    // LFO targets — order confirmed: synth-project-memory.md line 397
    const LFO_TARGET_OPTIONS = [
        { value: 0, label: "None"               },
        { value: 1, label: "Filter - Cutoff"    },
        { value: 2, label: "Osc 1 - Pitch"      },
        { value: 3, label: "Osc 2 - Pitch"      },
        { value: 4, label: "Osc 1 - Level"      },
        { value: 5, label: "Osc 2 - Level"      },
        { value: 6, label: "Amp - Level"         },
        { value: 7, label: "Filter - Resonance" },
    ];

    // Velocity mod targets — order confirmed: synth-project-memory.md line 398
    const VEL_MOD_TARGET_OPTIONS = [
        { value: 0, label: "None"               },
        { value: 1, label: "Amp - Level"         },
        { value: 2, label: "Amp - Attack"        },
        { value: 3, label: "Filter - Cutoff"    },
        { value: 4, label: "Filter - Resonance" },
    ];

    // =========================================================================
    // State
    // =========================================================================
    let isUpdatingFromCpp = false;

    // =========================================================================
    // Logging
    // =========================================================================
    const debugLog = document.getElementById("debug-log");

    const log = (level, message) => {
        if (!debugLog) return;

        const labels = { [LOG.TRACE]: "TRACE", [LOG.DEBUG]: "DEBUG", [LOG.INFO]: "INFO", [LOG.WARN]: "WARN", [LOG.ERROR]: "ERROR" };
        const css    = { [LOG.TRACE]: "from-trace", [LOG.DEBUG]: "from-js", [LOG.INFO]: "from-cpp", [LOG.WARN]: "from-warn", [LOG.ERROR]: "error" };

        const entry = document.createElement("div");
        entry.className = `log-entry ${css[level] ?? ""}`;
        const ts = new Date().toLocaleTimeString("en-US", { hour12: false, hour: "2-digit", minute: "2-digit", second: "2-digit", fractionalSecondDigits: 2 });
        entry.textContent = `[${ts}] [${labels[level]}] ${message}`;
        debugLog.appendChild(entry);

        while (debugLog.children.length > MAX_LOG_ENTRIES) {
            debugLog.removeChild(debugLog.firstChild);
        }
        debugLog.scrollTop = debugLog.scrollHeight;

        if (level >= LOG.INFO && SolaceBridge.isJuceAvailable()) {
            SolaceBridge.log(`[${labels[level]}] ${message}`);
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
        if (!statusDot) return;
        statusDot.className = "status-dot";
        if (state === "connected") statusDot.classList.add("status-dot--connected");
        if (state === "error")     statusDot.classList.add("status-dot--error");
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
    // bindFader
    // Connects a <input type="range"> to an APVTS paramId via the bridge.
    // formatFn: optional fn(value) → string for the value display.
    // =========================================================================
    const bindFader = (paramId, formatFn) => {
        const input   = document.getElementById(paramId);
        const display = document.getElementById(`${paramId}-value`);

        if (!input) {
            logWarn(`bindFader: #${paramId} not found in DOM`);
            return;
        }

        const fmt = formatFn ?? ((v) => parseFloat(v).toFixed(2));

        input.addEventListener("input", (e) => {
            if (isUpdatingFromCpp) return;
            const val = parseFloat(e.target.value);
            logTrace(`fader ${paramId} → ${val}`);
            if (display) display.textContent = fmt(val);
            SolaceBridge.setParameter(paramId, val);
        });

        SolaceBridge.onParameterChanged(paramId, (val) => {
            logTrace(`C++ → ${paramId} = ${val}`);
            isUpdatingFromCpp = true;
            input.value = val;
            if (display) display.textContent = fmt(val);
            isUpdatingFromCpp = false;
        });
    };

    // =========================================================================
    // bindWaveformSelector
    // Click cycles through waveforms, updating icon + sending param to C++.
    // =========================================================================
    const bindWaveformSelector = (paramId, iconId, waveforms, defaultIndex = 0) => {
        const iconEl = document.getElementById(iconId);
        if (!iconEl) {
            logWarn(`bindWaveformSelector: #${iconId} not found`);
            return;
        }

        const row = iconEl.closest(".waveform-icon-row");
        let currentIndex = defaultIndex;

        const apply = (index) => {
            currentIndex = ((index % waveforms.length) + waveforms.length) % waveforms.length;
            const wf = waveforms[currentIndex];
            iconEl.src = wf.icon;
            iconEl.alt = wf.alt;
        };

        const handleActivate = () => {
            if (isUpdatingFromCpp) return;
            apply(currentIndex + 1);
            SolaceBridge.setParameter(paramId, waveforms[currentIndex].value);
            logTrace(`${paramId} → ${waveforms[currentIndex].label}`);
        };

        row?.addEventListener("click", handleActivate);
        row?.addEventListener("keydown", (e) => {
            if (e.key === "Enter" || e.key === " ") { e.preventDefault(); handleActivate(); }
        });

        SolaceBridge.onParameterChanged(paramId, (val) => {
            isUpdatingFromCpp = true;
            apply(Math.round(val));
            isUpdatingFromCpp = false;
        });

        apply(defaultIndex);
    };

    // =========================================================================
    // bindArrowSelector
    // Prev/next buttons step through an options array.
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
    // bindDropdown
    // Click cycles through options (full dropdown popup deferred to Phase 7.3).
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

        const handleActivate = () => {
            if (isUpdatingFromCpp) return;
            apply(currentIndex + 1);
            SolaceBridge.setParameter(paramId, options[currentIndex].value);
            logTrace(`${paramId} → ${options[currentIndex].label}`);
        };

        dropdown.addEventListener("click", handleActivate);
        dropdown.addEventListener("keydown", (e) => {
            if (e.key === "Enter" || e.key === " ") { e.preventDefault(); handleActivate(); }
        });

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
    // Formatters
    // =========================================================================
    const fmtFreq    = (v) => v >= 1000 ? `${(v / 1000).toFixed(2)}k` : `${Math.round(v)}`;
    const fmtSemis   = (v) => `${Math.round(v)}`;
    const fmtSeconds = (v) => parseFloat(v).toFixed(3);
    const fmtBipolar = (v) => parseFloat(v).toFixed(2);   // filterEnvDepth shows e.g. "+0.50" or "-0.75"

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
            3   // default index 3 → value 0
        );
        bindArrowSelector(
            "osc1Transpose",
            Array.from({ length: 25 }, (_, i) => ({ value: i - 12, label: String(i - 12) })),
            "osc1-transpose-value", "osc1-transpose-prev", "osc1-transpose-next",
            12  // default index 12 → value 0
        );
        bindFader("osc1Tuning", fmtSemis);

        // --- Osc Mix ---
        bindFader("oscMix");

        // --- Oscillator 2 ---
        bindWaveformSelector("osc2Waveform", "osc2-waveform-icon", OSC_WAVEFORMS, 2);  // default: Square
        bindArrowSelector(
            "osc2Octave",
            [-3,-2,-1,0,1,2,3].map(v => ({ value: v, label: String(v) })),
            "osc2-octave-value", "osc2-octave-prev", "osc2-octave-next",
            4   // default index 4 → value 1
        );
        bindArrowSelector(
            "osc2Transpose",
            Array.from({ length: 25 }, (_, i) => ({ value: i - 12, label: String(i - 12) })),
            "osc2-transpose-value", "osc2-transpose-prev", "osc2-transpose-next",
            12  // default 0
        );
        bindFader("osc2Tuning", fmtSemis);

        // --- Amplifier Envelope ---
        bindFader("ampAttack",  fmtSeconds);
        bindFader("ampDecay",   fmtSeconds);
        bindFader("ampSustain");
        bindFader("ampRelease", fmtSeconds);

        // --- Master ---
        bindFader("masterDistortion");
        bindFader("masterVolume");

        // --- Filter ---
        bindFader("filterCutoff", fmtFreq);
        bindFader("filterResonance");

        // --- Filter Configuration ---
        bindArrowSelector(
            "filterType",
            FILTER_TYPES,
            "filterType-value", "filter-type-prev", "filter-type-next",
            1   // default: LP 24 dB
        );
        bindFader("filterEnvDepth",   fmtBipolar);  // Phase 6.4 bipolar (-1 → +1)
        bindFader("filterEnvAttack",  fmtSeconds);
        bindFader("filterEnvDecay",   fmtSeconds);
        bindFader("filterEnvSustain");
        bindFader("filterEnvRelease", fmtSeconds);

        // --- LFO ---
        bindWaveformSelector("lfoWaveform", "lfo-waveform-icon", LFO_WAVEFORMS, 0);
        bindFader("lfoAmount");
        bindFader("lfoRate", (v) => parseFloat(v).toFixed(2));
        bindDropdown("lfoTarget1", LFO_TARGET_OPTIONS, "lfoTarget1-dropdown", "lfoTarget1-value", 1);  // Filter Cutoff
        bindDropdown("lfoTarget2", LFO_TARGET_OPTIONS, "lfoTarget2-dropdown", "lfoTarget2-value", 5);  // Osc 2 Level
        bindDropdown("lfoTarget3", LFO_TARGET_OPTIONS, "lfoTarget3-dropdown", "lfoTarget3-value", 0);  // None

        // --- Voicing ---
        bindArrowSelector(
            "voiceCount",
            Array.from({ length: 16 }, (_, i) => ({ value: i + 1, label: String(i + 1) })),
            "voiceCount-value", "voice-count-prev", "voice-count-next",
            15  // default index 15 → value 16
        );
        bindArrowSelector(
            "unisonCount",
            Array.from({ length: 8 }, (_, i) => ({ value: i + 1, label: String(i + 1) })),
            "unisonCount-value", "unison-prev", "unison-next",
            0   // FIX: was 2 (showing 3). Now 0 → value 1, matching APVTS default.
        );
        bindFader("velocityRange");
        bindDropdown("velocityModTarget1", VEL_MOD_TARGET_OPTIONS, "velModTarget1-dropdown", "velocityModTarget1-value", 2);  // Amp Attack
        bindDropdown("velocityModTarget2", VEL_MOD_TARGET_OPTIONS, "velModTarget2-dropdown", "velocityModTarget2-value", 0);  // None

        logDebug("All controls bound");
    };

    // =========================================================================
    // Initialize
    // =========================================================================
    const init = async () => {
        logInfo("=== Solace Synth UI — Phase 7.2 (rev 2) ===");

        try {
            const bridgeOk = SolaceBridge.init();
            logInfo(`Bridge: ${bridgeOk ? "JUCE backend detected" : "browser preview mode"}`);

            // Bind controls before signalUiReady so C++ initial sync hits registered listeners
            bindAllControls();

            if (!bridgeOk) {
                setStatus("preview mode", "disconnected");
                return;
            }

            setStatus("connecting...", "disconnected");

            const ready = await SolaceBridge.signalUiReady();
            logInfo(`uiReady: ${ready}`);

            if (ready) {
                setStatus("connected", "connected");
                logInfo("Handshake complete");
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
