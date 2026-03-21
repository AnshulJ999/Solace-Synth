/* ============================================================================
 * Solace Synth — Main UI Orchestrator
 * Phase 7.3: Component-based architecture
 *
 * Responsibilities:
 *   - Bridge initialization + status indicator
 *   - Instantiate and mount all UI components into their mount-point divs
 *   - Debug panel toggle (Ctrl+Shift+D)
 *   - Application-level logging
 *
 * Components handle all parameter binding internally.
 * Source of truth: C++ APVTS. JS reflects and requests — never owns state.
 *
 * LFO target enum (⚠️ PENDING RECONCILIATION with Vision Document.
 *   Current values match old synth-project-memory spec, NOT Vision Doc answer 5.
 *   Do NOT wire DSP LFO until Anshul confirms the target list.):
 *   0=None, 1=FilterCutoff, 2=Osc1Pitch, 3=Osc2Pitch,
 *   4=Osc1Level, 5=Osc2Level, 6=AmpLevel, 7=FilterResonance
 *
 * Velocity mod target enum (⚠️ same pending status):
 *   0=None, 1=AmpLevel, 2=AmpAttack, 3=FilterCutoff, 4=FilterResonance
 * ============================================================================ */

(function () {
    "use strict";

    // =========================================================================
    // Logging
    // =========================================================================
    const LOG = { TRACE: 0, DEBUG: 1, INFO: 2, WARN: 3, ERROR: 4 };
    const MAX_LOG_ENTRIES = 100;

    const debugLog = document.getElementById("debug-log");

    const log = (level, message) => {
        if (!debugLog) return;

        const labels = { [LOG.TRACE]: "TRACE", [LOG.DEBUG]: "DEBUG", [LOG.INFO]: "INFO", [LOG.WARN]: "WARN", [LOG.ERROR]: "ERROR" };
        const css    = { [LOG.TRACE]: "from-trace", [LOG.DEBUG]: "from-js", [LOG.INFO]: "from-cpp", [LOG.WARN]: "from-warn", [LOG.ERROR]: "error" };

        const entry = document.createElement("div");
        entry.className = `log-entry ${css[level] ?? ""}`;
        const ts = new Date().toLocaleTimeString("en-US", {
            hour12: false, hour: "2-digit", minute: "2-digit",
            second: "2-digit", fractionalSecondDigits: 2,
        });
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

    const logInfo  = (m) => log(LOG.INFO,  m);
    const logWarn  = (m) => log(LOG.WARN,  m);
    const logError = (m) => log(LOG.ERROR, m);
    const logDebug = (m) => log(LOG.DEBUG, m);

    // =========================================================================
    // Status indicator
    // =========================================================================
    const statusDot  = document.getElementById("status-dot");
    const statusText = document.getElementById("status-text");

    const setStatus = (text, state = "disconnected") => {
        if (statusText) statusText.textContent = text;
        if (!statusDot) return;
        statusDot.className = "status-dot";
        if (state === "connected")    statusDot.classList.add("status-dot--connected");
        if (state === "error")        statusDot.classList.add("status-dot--error");
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
    // Option arrays
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

    const FILTER_TYPES = [
        { value: 0, label: "LP 12 dB" },
        { value: 1, label: "LP 24 dB" },
        { value: 2, label: "HP 12 dB" },
    ];

    const OCTAVE_OPTIONS     = [-3,-2,-1,0,1,2,3].map(v => ({ value: v, label: String(v) }));
    const TRANSPOSE_OPTIONS  = Array.from({ length: 25 }, (_, i) => ({ value: i - 12, label: String(i - 12) }));
    const VOICE_COUNT_OPTIONS= Array.from({ length: 16 }, (_, i) => ({ value: i + 1, label: String(i + 1) }));
    const UNISON_OPTIONS     = Array.from({ length: 8  }, (_, i) => ({ value: i + 1, label: String(i + 1) }));

    // ⚠️ PENDING: LFO target enum does not match Vision Document answer 5.
    //    Leaving unchanged until Anshul/Nabeel confirm the final target list.
    //    DSP enum: 0=None,1=FilterCutoff,2=Osc1Pitch,3=Osc2Pitch,
    //              4=Osc1Level,5=Osc2Level,6=AmpLevel,7=FilterResonance
    const LFO_TARGET_OPTIONS = [
        { value: 0, label: "None"              },
        { value: 1, label: "Filter - Cutoff"   },
        { value: 2, label: "Osc 1 - Pitch"     },
        { value: 3, label: "Osc 2 - Pitch"     },
        { value: 4, label: "Osc 1 - Level"     },
        { value: 5, label: "Osc 2 - Level"     },
        { value: 6, label: "Amp - Level"       },
        { value: 7, label: "Filter - Resonance"},
    ];

    // Velocity mod target enum — fully implemented in DSP (Phase 6.8 / 6.8b).
    //   0=None, 1=AmpLevel, 2=AmpAttack, 3=FilterCutoff, 4=FilterResonance,
    //   5=Distortion, 6=Osc1Pitch, 7=OscMix
    const VEL_MOD_TARGET_OPTIONS = [
        { value: 0, label: "None"              },
        { value: 1, label: "Amp - Level"       },
        { value: 2, label: "Amp - Attack"      },
        { value: 3, label: "Filter - Cutoff"   },
        { value: 4, label: "Filter - Resonance"},
        { value: 5, label: "Distortion"        },
        { value: 6, label: "Osc Pitch"     },
        { value: 7, label: "Osc Mix"           },
    ];

    // =========================================================================
    // Helper: get mount element, warn if missing
    // =========================================================================
    const mount = (id) => {
        const el = document.getElementById(id);
        if (!el) logWarn(`mount point #${id} not found`);
        return el;
    };

    // =========================================================================
    // Mount all components
    // Called before signalUiReady so bridge listeners are registered
    // before C++ sends the initial syncAllParameters.
    // =========================================================================
    const mountAllComponents = () => {

        // --- Oscillator 1 ---
        new WaveformSelector("osc1Waveform", OSC_WAVEFORMS, { defaultIndex: 0, showArrows: true })
            .mount(mount("mount-osc1Waveform"));

        new ArrowSelector("osc1Octave", OCTAVE_OPTIONS, { label: "Octave", ariaLabel: "Oscillator 1 octave", defaultIndex: 3 })
            .mount(mount("mount-osc1Octave"));   // defaultIndex 3 → value 0

        new ArrowSelector("osc1Transpose", TRANSPOSE_OPTIONS, { label: "Transpose", ariaLabel: "Oscillator 1 transpose", defaultIndex: 12 })
            .mount(mount("mount-osc1Transpose")); // defaultIndex 12 → value 0

        new Fader("osc1Tuning", {
            label: "Tuning", min: -100, max: 100, step: 1, defaultValue: 0,
            formatFn: v => Math.round(v).toString(), sizeClass: "fader--standard",
        }).mount(mount("mount-osc1Tuning"));

        // --- Osc Mix ---
        new Fader("oscMix", {
            label: "Osc 2", bottomLabel: "Osc 1",
            min: 0, max: 1, step: 0.01, defaultValue: 0.5,
            sizeClass: "fader--standard",
        }).mount(mount("mount-oscMix"));

        // --- Oscillator 2 ---
        new WaveformSelector("osc2Waveform", OSC_WAVEFORMS, { defaultIndex: 2, showArrows: true }) // default: Square
            .mount(mount("mount-osc2Waveform"));

        new ArrowSelector("osc2Octave", OCTAVE_OPTIONS, { label: "Octave", ariaLabel: "Oscillator 2 octave", defaultIndex: 4 })
            .mount(mount("mount-osc2Octave"));   // defaultIndex 4 → value 1

        new ArrowSelector("osc2Transpose", TRANSPOSE_OPTIONS, { label: "Transpose", ariaLabel: "Oscillator 2 transpose", defaultIndex: 12 })
            .mount(mount("mount-osc2Transpose"));

        new Fader("osc2Tuning", {
            label: "Tuning", min: -100, max: 100, step: 1, defaultValue: 0,
            formatFn: v => Math.round(v).toString(), sizeClass: "fader--standard",
        }).mount(mount("mount-osc2Tuning"));

        // --- Amplifier Envelope ---
        const fmtSec = v => parseFloat(v).toFixed(3);
        new Fader("ampAttack",  { label: "Attack",  min: 0.001, max: 5,  step: 0.001, defaultValue: 0.01, formatFn: fmtSec, sizeClass: "fader--standard" }).mount(mount("mount-ampAttack"));
        new Fader("ampDecay",   { label: "Decay",   min: 0.001, max: 5,  step: 0.001, defaultValue: 0.1,  formatFn: fmtSec, sizeClass: "fader--standard" }).mount(mount("mount-ampDecay"));
        new Fader("ampSustain", { label: "Sustain", min: 0,     max: 1,  step: 0.01,  defaultValue: 0.8,  sizeClass: "fader--standard" }).mount(mount("mount-ampSustain"));
        new Fader("ampRelease", { label: "Release", min: 0.001, max: 10, step: 0.001, defaultValue: 0.3,  formatFn: fmtSec, sizeClass: "fader--standard" }).mount(mount("mount-ampRelease"));

        // --- Master ---
        new Fader("masterDistortion", { label: "Distortion", min: 0, max: 1, step: 0.01, defaultValue: 0,   sizeClass: "fader--standard" }).mount(mount("mount-masterDistortion"));
        new Fader("masterVolume",     { label: "Level",      min: 0, max: 1, step: 0.01, defaultValue: 0.8, sizeClass: "fader--standard" }).mount(mount("mount-masterVolume"));

        // --- Filter ---
        const fmtFreq = v => v >= 1000 ? `${(v / 1000).toFixed(2)}k` : `${Math.round(v)}`;
        new Fader("filterCutoff",    { label: "Cutoff",    min: 20,  max: 20000, step: 1,    defaultValue: 20000, formatFn: fmtFreq, sizeClass: "fader--big" }).mount(mount("mount-filterCutoff"));
        new Fader("filterResonance", { label: "Resonance", min: 0,   max: 1,     step: 0.01, defaultValue: 0,     sizeClass: "fader--big" }).mount(mount("mount-filterResonance"));

        // --- Filter Configuration ---
        new ArrowSelector("filterType", FILTER_TYPES, { label: "Filter Type", ariaLabel: "Filter type", defaultIndex: 1 }) // default: LP 24 dB
            .mount(mount("mount-filterType"));

        // filterEnvDepth — mounts into a hidden div; bridge listener registered but invisible.
        // Unhide #mount-filterEnvDepth only when Nabeel adds it to the Figma design.
        new Fader("filterEnvDepth", {
            label: "Env Depth", min: -1, max: 1, step: 0.01, defaultValue: 0,
            sizeClass: "fader--small", extraClass: "fader--bipolar",
        }).mount(mount("mount-filterEnvDepth"));

        new Fader("filterEnvAttack",  { label: "Attack",  min: 0.001, max: 5,  step: 0.001, defaultValue: 0.01, formatFn: fmtSec, sizeClass: "fader--small" }).mount(mount("mount-filterEnvAttack"));
        new Fader("filterEnvDecay",   { label: "Decay",   min: 0.001, max: 5,  step: 0.001, defaultValue: 0.3,  formatFn: fmtSec, sizeClass: "fader--small" }).mount(mount("mount-filterEnvDecay"));
        new Fader("filterEnvSustain", { label: "Sustain", min: 0,     max: 1,  step: 0.01,  defaultValue: 0,    sizeClass: "fader--small" }).mount(mount("mount-filterEnvSustain"));
        new Fader("filterEnvRelease", { label: "Release", min: 0.001, max: 10, step: 0.001, defaultValue: 0.3,  formatFn: fmtSec, sizeClass: "fader--small" }).mount(mount("mount-filterEnvRelease"));

        // --- LFO ---
        // showArrows: true — Figma shows < icon > flanking arrows on LFO waveform selector
        new WaveformSelector("lfoWaveform", LFO_WAVEFORMS, { defaultIndex: 0, showArrows: true })
            .mount(mount("mount-lfoWaveform"));

        new Fader("lfoAmount", { label: "Amount", min: 0, max: 1, step: 0.01, defaultValue: 0, sizeClass: "fader--big" }).mount(mount("mount-lfoAmount"));
        new Fader("lfoRate",   { label: "Rate",   min: 0.01, max: 50, step: 0.01, defaultValue: 1, sizeClass: "fader--big" }).mount(mount("mount-lfoRate"));

        new Dropdown("lfoTarget1", LFO_TARGET_OPTIONS, { ariaLabel: "LFO Target 1", defaultIndex: 1 }) // Filter Cutoff
            .mount(mount("mount-lfoTarget1"));
        new Dropdown("lfoTarget2", LFO_TARGET_OPTIONS, { ariaLabel: "LFO Target 2", defaultIndex: 5 }) // Osc 2 Level
            .mount(mount("mount-lfoTarget2"));
        new Dropdown("lfoTarget3", LFO_TARGET_OPTIONS, { ariaLabel: "LFO Target 3", defaultIndex: 0 }) // None
            .mount(mount("mount-lfoTarget3"));

        // --- Voicing ---
        new ArrowSelector("voiceCount", VOICE_COUNT_OPTIONS, { label: "No. of Voices", ariaLabel: "Number of voices", defaultIndex: 15 }) // default 16
            .mount(mount("mount-voiceCount"));

        new ArrowSelector("unisonCount", UNISON_OPTIONS, { label: "Unison", ariaLabel: "Unison voices", defaultIndex: 0 }) // default 1
            .mount(mount("mount-unisonCount"));

        new Fader("velocityRange", { label: "Velocity", min: 0, max: 1, step: 0.01, defaultValue: 1, sizeClass: "fader--big" })
            .mount(mount("mount-velocityRange"));

        new Dropdown("velocityModTarget1", VEL_MOD_TARGET_OPTIONS, { ariaLabel: "Velocity Mod Target 1", defaultIndex: 1 }) // Amp Level (Nabeel confirmed default, Phase 6.8b)
            .mount(mount("mount-velocityModTarget1"));
        new Dropdown("velocityModTarget2", VEL_MOD_TARGET_OPTIONS, { ariaLabel: "Velocity Mod Target 2", defaultIndex: 0 }) // None
            .mount(mount("mount-velocityModTarget2"));
        new Dropdown("velocityModTarget3", VEL_MOD_TARGET_OPTIONS, { ariaLabel: "Velocity Mod Target 3", defaultIndex: 0 }) // None
            .mount(mount("mount-velocityModTarget3"));

        // unisonDetune + unisonSpread: deliberately NOT mounted — no UI controls in V1 (engine-only by design).
        // C++ APVTS params exist; bridge silently skips them on syncAllParameters (no registered listeners = no-op). This is correct.

        // --- Preset Browser ---
        new PresetBrowser();

        logDebug("All components mounted");
    };

    // =========================================================================
    // Initialize
    // =========================================================================
    const init = async () => {
        logInfo("=== Solace Synth UI — Phase 7.4 ===");

        try {
            const bridgeOk = SolaceBridge.init();
            logInfo(`Bridge: ${bridgeOk ? "JUCE backend detected" : "browser preview mode"}`);

            // Mount components BEFORE signalUiReady so bridge listeners are registered
            // when C++ fires the initial syncAllParameters bulk event.
            mountAllComponents();

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
