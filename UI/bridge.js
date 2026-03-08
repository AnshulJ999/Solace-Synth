/* ============================================================================
 * Solace Synth - Bridge (JS <-> C++)
 *
 * This module handles all communication between the HTML frontend and the
 * JUCE C++ backend. It wraps JUCE's low-level __JUCE__ API into a clean
 * interface that the rest of the UI code can use.
 *
 * Message Protocol:
 *   JS -> C++ (native functions):
 *     - setParameter(paramId, value)  : change a synth parameter
 *     - uiReady()                     : signal that the page has loaded
 *     - log(message)                  : send debug message to C++ console
 *
 *   C++ -> JS (events):
 *     - parameterChanged { paramId, value } : single parameter updated
 *     - syncAllParameters [{ paramId, value }, ...] : bulk parameter sync
 *
 * IMPORTANT: APVTS in C++ is the single source of truth.
 * JS reflects and requests changes, but never "owns" state.
 * ============================================================================
 */

const SolaceBridge = (() => {
    // Store for parameter change callbacks
    const parameterListeners = new Map();

    // State
    let isConnected = false;

    // ========================================================================
    // Check if JUCE backend is available
    // ========================================================================
    function isJuceAvailable() {
        return typeof window.__JUCE__ !== "undefined" &&
               typeof window.__JUCE__.backend !== "undefined";
    }

    // ========================================================================
    // Get a native function from JUCE
    // ========================================================================
    function getNativeFunction(name) {
        if (!isJuceAvailable()) return null;

        // JUCE 8 exposes native functions via __JUCE__.backend
        const fn = window.__JUCE__.backend.getNativeFunction(name);
        return fn || null;
    }

    // ========================================================================
    // JS -> C++: Set a parameter value
    // ========================================================================
    async function setParameter(paramId, value) {
        const fn = getNativeFunction("setParameter");
        if (fn) {
            try {
                const result = await fn(paramId, value);
                return result;
            } catch (e) {
                console.error("[Bridge] setParameter failed:", e);
                return false;
            }
        }
        console.warn("[Bridge] setParameter: JUCE backend not available");
        return false;
    }

    // ========================================================================
    // JS -> C++: Signal that the UI is ready
    // ========================================================================
    async function signalUiReady() {
        const fn = getNativeFunction("uiReady");
        if (fn) {
            try {
                await fn();
                isConnected = true;
                console.log("[Bridge] UI ready signal sent to C++");
                return true;
            } catch (e) {
                console.error("[Bridge] uiReady failed:", e);
                return false;
            }
        }
        console.warn("[Bridge] uiReady: JUCE backend not available");
        return false;
    }

    // ========================================================================
    // JS -> C++: Send a debug log message
    // ========================================================================
    async function log(message) {
        const fn = getNativeFunction("log");
        if (fn) {
            try {
                await fn(String(message));
            } catch (e) {
                // Silently fail — this is just debug logging
            }
        }
    }

    // ========================================================================
    // C++ -> JS: Listen for parameter changes
    // ========================================================================
    function onParameterChanged(paramId, callback) {
        if (!parameterListeners.has(paramId)) {
            parameterListeners.set(paramId, []);
        }
        parameterListeners.get(paramId).push(callback);
    }

    // ========================================================================
    // C++ -> JS: Register event listeners for C++ events
    // ========================================================================
    function setupEventListeners() {
        if (!isJuceAvailable()) return;

        // Listen for single parameter changes
        window.__JUCE__.backend.addEventListener("parameterChanged", (data) => {
            const { paramId, value } = data;
            const listeners = parameterListeners.get(paramId);
            if (listeners) {
                listeners.forEach(cb => cb(value));
            }
        });

        // Listen for bulk parameter sync (sent on UI ready and preset load)
        window.__JUCE__.backend.addEventListener("syncAllParameters", (params) => {
            if (Array.isArray(params)) {
                params.forEach(({ paramId, value }) => {
                    const listeners = parameterListeners.get(paramId);
                    if (listeners) {
                        listeners.forEach(cb => cb(value));
                    }
                });
            }
        });
    }

    // ========================================================================
    // Initialize the bridge
    // ========================================================================
    function init() {
        if (!isJuceAvailable()) {
            console.warn("[Bridge] JUCE backend not available - running in standalone browser mode");
            return false;
        }

        setupEventListeners();
        return true;
    }

    // Public API
    return {
        init,
        setParameter,
        signalUiReady,
        log,
        onParameterChanged,
        isJuceAvailable,
        get isConnected() { return isConnected; }
    };
})();
