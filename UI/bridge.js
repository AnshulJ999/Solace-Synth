/* ============================================================================
 * Solace Synth - Bridge (JS <-> C++)
 *
 * This module handles all communication between the HTML frontend and the
 * JUCE C++ backend. It wraps JUCE 8's low-level event-based API into a
 * clean interface that the rest of the UI code can use.
 *
 * JUCE 8 Low-Level API (from check_native_interop.js + index.js):
 *   window.__JUCE__.backend.emitEvent(eventId, payload)  -> send to C++
 *   window.__JUCE__.backend.addEventListener(eventId, fn) -> listen from C++
 *
 * Native functions are called by emitting "__juce__invoke" events.
 * Results come back as "__juce__complete" events with a matching promiseId.
 * This is the exact same pattern JUCE's own index.js module uses internally.
 *
 * Message Protocol:
 *   JS -> C++ (native functions via __juce__invoke):
 *     - setParameter(paramId, value)  : change a synth parameter
 *     - uiReady()                     : signal that the page has loaded
 *     - log(message)                  : send debug message to C++ console
 *
 *   C++ -> JS (events via emitEventIfBrowserIsVisible):
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

    // Promise handling for native function calls (matches JUCE's PromiseHandler)
    let lastPromiseId = 0;
    const pendingPromises = new Map();

    // ========================================================================
    // Check if JUCE backend is available
    // ========================================================================
    function isJuceAvailable() {
        return typeof window.__JUCE__ !== "undefined" &&
               typeof window.__JUCE__.backend !== "undefined" &&
               typeof window.__JUCE__.backend.emitEvent === "function";
    }

    // ========================================================================
    // Call a native C++ function by name (via JUCE's __juce__invoke event)
    //
    // This replicates the pattern from JUCE 8's index.js:
    //   1. Create a promise with a unique ID
    //   2. Emit "__juce__invoke" with the function name, args, and promise ID
    //   3. C++ handles it, calls the NativeFunction, then fires "__juce__complete"
    //   4. Our listener resolves the promise with the result
    // ========================================================================
    function callNativeFunction(name, ...args) {
        if (!isJuceAvailable()) {
            console.warn("[Bridge] JUCE backend not available for: " + name);
            return Promise.resolve(null);
        }

        const promiseId = lastPromiseId++;

        const promise = new Promise((resolve) => {
            pendingPromises.set(promiseId, { resolve: resolve });
        });

        window.__JUCE__.backend.emitEvent("__juce__invoke", {
            name: name,
            params: args,
            resultId: promiseId,
        });

        return promise;
    }

    // ========================================================================
    // JS -> C++: Set a parameter value
    // ========================================================================
    function setParameter(paramId, value) {
        return callNativeFunction("setParameter", paramId, value);
    }

    // ========================================================================
    // JS -> C++: Signal that the UI is ready
    // ========================================================================
    async function signalUiReady() {
        try {
            const result = await callNativeFunction("uiReady");
            if (result) {
                isConnected = true;
                console.log("[Bridge] UI ready signal acknowledged by C++");
            }
            return result;
        } catch (e) {
            console.error("[Bridge] uiReady failed:", e);
            return false;
        }
    }

    // ========================================================================
    // JS -> C++: Send a debug log message
    // ========================================================================
    function log(message) {
        return callNativeFunction("log", String(message));
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

        // Listen for native function completion (matches JUCE's PromiseHandler)
        window.__JUCE__.backend.addEventListener("__juce__complete", (data) => {
            const promiseId = data.promiseId;
            if (pendingPromises.has(promiseId)) {
                pendingPromises.get(promiseId).resolve(data.result);
                pendingPromises.delete(promiseId);
            }
        });

        // Listen for single parameter changes from C++
        window.__JUCE__.backend.addEventListener("parameterChanged", (data) => {
            const listeners = parameterListeners.get(data.paramId);
            if (listeners) {
                listeners.forEach(cb => cb(data.value));
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
        console.log("[Bridge] Initialized with JUCE backend");
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
