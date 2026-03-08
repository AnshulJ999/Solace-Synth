/* ============================================================================
 * Solace Synth - Main UI Logic
 *
 * Handles:
 *   - Initializing the bridge
 *   - Binding UI controls to the bridge
 *   - Diagnostic logging with levels (TRACE, DEBUG, INFO, WARN, ERROR)
 *   - Updating the UI when parameters change from C++ (automation/presets)
 *
 * Log levels:
 *   TRACE — every slider move, every event (UI panel only, NOT sent to C++)
 *   DEBUG — bridge calls, parameter changes (UI panel + C++ file)
 *   INFO  — lifecycle events (UI panel + C++ file)
 *   WARN  — suspicious but non-fatal (UI panel + C++ file)
 *   ERROR — something broke (UI panel + C++ file)
 *
 * IMPORTANT: The UI only reflects and requests state changes.
 * C++ APVTS is always the source of truth.
 * ============================================================================
 */

(function () {
    "use strict";

    // ========================================================================
    // UI Element References
    // ========================================================================
    const masterSlider = document.getElementById("masterVolume");
    const masterValueDisplay = document.getElementById("masterVolume-value");
    const statusText = document.getElementById("status-text");
    const debugLog = document.getElementById("debug-log");

    // Track if we're currently receiving a C++ update (to avoid echo loops)
    let isUpdatingFromCpp = false;

    // Max log entries to keep in the UI panel
    const MAX_LOG_ENTRIES = 80;

    // Log levels
    const LOG_LEVEL = {
        TRACE: 0,
        DEBUG: 1,
        INFO:  2,
        WARN:  3,
        ERROR: 4
    };

    // ========================================================================
    // Logging system — visible panel + optional C++ forwarding
    // ========================================================================
    function log(level, message) {
        if (!debugLog) return;

        var levelName = "TRACE";
        var cssClass = "";
        switch (level) {
            case LOG_LEVEL.TRACE: levelName = "TRACE"; cssClass = "from-trace"; break;
            case LOG_LEVEL.DEBUG: levelName = "DEBUG"; cssClass = "from-js";    break;
            case LOG_LEVEL.INFO:  levelName = "INFO";  cssClass = "from-cpp";   break;
            case LOG_LEVEL.WARN:  levelName = "WARN";  cssClass = "from-warn";  break;
            case LOG_LEVEL.ERROR: levelName = "ERROR"; cssClass = "error";      break;
        }

        var entry = document.createElement("div");
        entry.className = "log-entry " + cssClass;
        var timestamp = new Date().toLocaleTimeString("en-US", {
            hour12: false,
            hour: "2-digit",
            minute: "2-digit",
            second: "2-digit",
            fractionalSecondDigits: 3
        });
        entry.textContent = "[" + timestamp + "] [" + levelName + "] " + message;

        debugLog.appendChild(entry);

        // Keep panel at a reasonable size
        while (debugLog.children.length > MAX_LOG_ENTRIES) {
            debugLog.removeChild(debugLog.firstChild);
        }

        // Auto-scroll
        debugLog.scrollTop = debugLog.scrollHeight;

        // Forward INFO+ to C++ logs (TRACE/DEBUG stay in panel only to avoid noise)
        if (level >= LOG_LEVEL.INFO && SolaceBridge.isJuceAvailable()) {
            SolaceBridge.log("[" + levelName + "] " + message);
        }
    }

    // Convenience wrappers
    function logTrace(msg) { log(LOG_LEVEL.TRACE, msg); }
    function logDebug(msg) { log(LOG_LEVEL.DEBUG, msg); }
    function logInfo(msg)  { log(LOG_LEVEL.INFO,  msg); }
    function logWarn(msg)  { log(LOG_LEVEL.WARN,  msg); }
    function logError(msg) { log(LOG_LEVEL.ERROR, msg); }

    // ========================================================================
    // Bind UI controls to the bridge
    // ========================================================================
    function bindControls() {
        if (!masterSlider) {
            logError("masterVolume slider element not found in DOM!");
            return;
        }

        // When user moves the slider -> send to C++
        masterSlider.addEventListener("input", function(e) {
            if (isUpdatingFromCpp) return;

            var value = parseFloat(e.target.value);
            logTrace("Slider input: value=" + value.toFixed(4));
            updateValueDisplay("masterVolume", value);
            SolaceBridge.setParameter("masterVolume", value);
        });

        // Listen for C++ parameter changes -> update slider
        SolaceBridge.onParameterChanged("masterVolume", function(value) {
            logTrace("C++ -> JS: masterVolume = " + value.toFixed(4));
            isUpdatingFromCpp = true;
            masterSlider.value = value;
            updateValueDisplay("masterVolume", value);
            isUpdatingFromCpp = false;
        });

        logDebug("Controls bound to bridge");
    }

    // ========================================================================
    // Update the value display text for a parameter
    // ========================================================================
    function updateValueDisplay(paramId, value) {
        var display = document.getElementById(paramId + "-value");
        if (display) {
            display.textContent = value.toFixed(2);
        }
    }

    // ========================================================================
    // Update status bar
    // ========================================================================
    function setStatus(text) {
        if (statusText) {
            statusText.textContent = text;
        }
    }

    // ========================================================================
    // Initialize - with proper error handling
    // ========================================================================
    async function init() {
        logInfo("=== Solace Synth UI Init ===");

        try {
            var bridgeOk = SolaceBridge.init();
            logInfo("Bridge init: " + (bridgeOk ? "OK (JUCE backend detected)" : "No backend (browser preview)"));

            if (!bridgeOk) {
                setStatus("Running in browser preview mode (no C++ backend)");
                bindControls();
                return;
            }

            // Bind UI controls
            bindControls();

            // Signal to C++ that we're ready (triggers parameter sync)
            logInfo("Sending uiReady signal...");
            var ready = await SolaceBridge.signalUiReady();
            logInfo("uiReady response: " + ready);

            if (ready) {
                setStatus("Connected to Solace Synth engine");
                logInfo("Phase 4 bridge handshake complete!");
            } else {
                setStatus("Bridge connected but uiReady returned false");
                logWarn("uiReady returned false - check C++ logs");
            }
        } catch (error) {
            console.error("[Solace Synth] Init error:", error);
            logError("Init failed: " + error.message);
            setStatus("Error: " + error.message);
        }
    }

    // Start when DOM is loaded
    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", init);
    } else {
        init();
    }
})();
