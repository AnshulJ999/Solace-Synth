/* ============================================================================
 * Solace Synth - Main UI Logic
 *
 * This file handles:
 *   - Initializing the bridge
 *   - Binding UI controls to the bridge
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

    // Track if we're currently receiving a C++ update (to avoid echo loops)
    let isUpdatingFromCpp = false;

    // ========================================================================
    // Bind UI controls to the bridge
    // ========================================================================
    function bindControls() {
        if (!masterSlider) return;

        // When user moves the slider -> send to C++
        masterSlider.addEventListener("input", (e) => {
            if (isUpdatingFromCpp) return;

            const value = parseFloat(e.target.value);
            updateValueDisplay("masterVolume", value);
            SolaceBridge.setParameter("masterVolume", value);
        });

        // Listen for C++ parameter changes -> update slider
        SolaceBridge.onParameterChanged("masterVolume", (value) => {
            isUpdatingFromCpp = true;
            masterSlider.value = value;
            updateValueDisplay("masterVolume", value);
            isUpdatingFromCpp = false;
        });
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
    // Initialize
    // ========================================================================
    async function init() {
        // Initialize the bridge
        const bridgeOk = SolaceBridge.init();

        if (!bridgeOk) {
            setStatus("Running in browser preview mode (no C++ backend)");
            bindControls(); // Still bind for standalone testing
            return;
        }

        // Bind UI controls to parameters
        bindControls();

        // Signal to C++ that we're ready (triggers parameter sync)
        const ready = await SolaceBridge.signalUiReady();

        if (ready) {
            setStatus("Connected to Solace Synth engine");
            SolaceBridge.log("UI initialization complete");
        } else {
            setStatus("Failed to connect to engine");
        }
    }

    // Start when DOM is loaded
    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", init);
    } else {
        init();
    }
})();
