/* ============================================================================
 * Solace Synth - Main UI Logic
 *
 * Handles:
 *   - Initializing the bridge
 *   - Binding UI controls to the bridge
 *   - Diagnostic logging (visible in the debug panel)
 *   - Updating the UI when parameters change from C++ (automation/presets)
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

    // Max log entries to keep
    const MAX_LOG_ENTRIES = 50;

    // ========================================================================
    // Debug logging - visible in the UI debug panel
    // ========================================================================
    function logDebug(message, source) {
        if (!debugLog) return;

        const entry = document.createElement("div");
        entry.className = "log-entry " + (source || "");
        const timestamp = new Date().toLocaleTimeString("en-US", {
            hour12: false,
            hour: "2-digit",
            minute: "2-digit",
            second: "2-digit",
            fractionalSecondDigits: 3
        });
        entry.textContent = "[" + timestamp + "] " + message;

        debugLog.appendChild(entry);

        // Keep only the last N entries
        while (debugLog.children.length > MAX_LOG_ENTRIES) {
            debugLog.removeChild(debugLog.firstChild);
        }

        // Auto-scroll to bottom
        debugLog.scrollTop = debugLog.scrollHeight;

        // Also forward to C++ logs via bridge
        if (SolaceBridge.isJuceAvailable()) {
            SolaceBridge.log(message);
        }
    }

    // ========================================================================
    // Bind UI controls to the bridge
    // ========================================================================
    function bindControls() {
        if (!masterSlider) {
            logDebug("ERROR: masterVolume slider not found!", "error");
            return;
        }

        // When user moves the slider -> send to C++
        masterSlider.addEventListener("input", (e) => {
            if (isUpdatingFromCpp) {
                logDebug("BLOCKED: input event while isUpdatingFromCpp=true", "from-js");
                return;
            }

            const value = parseFloat(e.target.value);
            logDebug("JS input event: value=" + value.toFixed(4), "from-js");
            updateValueDisplay("masterVolume", value);
            SolaceBridge.setParameter("masterVolume", value);
        });

        // When user starts dragging the slider
        masterSlider.addEventListener("mousedown", () => {
            logDebug("JS mousedown on slider", "from-js");
        });

        masterSlider.addEventListener("mouseup", () => {
            logDebug("JS mouseup on slider", "from-js");
        });

        // Listen for C++ parameter changes -> update slider
        SolaceBridge.onParameterChanged("masterVolume", (value) => {
            logDebug("C++ -> JS: masterVolume = " + value.toFixed(4), "from-cpp");
            isUpdatingFromCpp = true;
            masterSlider.value = value;
            updateValueDisplay("masterVolume", value);
            isUpdatingFromCpp = false;
        });

        logDebug("Controls bound", "from-js");
    }

    // ========================================================================
    // Update the value display text for a parameter
    // ========================================================================
    function updateValueDisplay(paramId, value) {
        const display = document.getElementById(paramId + "-value");
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
        logDebug("=== Solace Synth UI Init ===", "from-js");

        try {
            const bridgeOk = SolaceBridge.init();
            logDebug("Bridge init: " + (bridgeOk ? "OK" : "NO BACKEND"), "from-js");

            if (!bridgeOk) {
                setStatus("Running in browser preview mode (no C++ backend)");
                bindControls();
                return;
            }

            // Bind UI controls
            bindControls();

            // Signal to C++ that we're ready (triggers parameter sync)
            logDebug("Calling uiReady...", "from-js");
            const ready = await SolaceBridge.signalUiReady();
            logDebug("uiReady result: " + ready, "from-js");

            if (ready) {
                setStatus("Connected to Solace Synth engine");
                logDebug("Bridge handshake complete!", "from-js");
            } else {
                setStatus("Bridge connected but uiReady returned false");
                logDebug("uiReady returned false", "error");
            }
        } catch (error) {
            console.error("[Solace Synth] Init error:", error);
            logDebug("INIT ERROR: " + error.message, "error");
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
