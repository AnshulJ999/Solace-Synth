# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## Project

**Solace Synth** — a free, open-source polyphonic subtractive synthesizer. VST3 + Standalone, built with JUCE 8 (C++) and a WebView-based HTML/CSS/JS UI.

**Team:** Anshul (C++ audio engine, DSP, bridge), friend Nabeel (Figma UI design, HTML/CSS frontend).

**Project memory:** `.agent/synth-project-memory.md` — always read this first. It is the authoritative rolling record of all decisions, bugs fixed, and current status. Keep it updated after significant changes.

**Phase plans:** `.agent/plans/` — detailed sub-phase implementation plans for Phase 6 (DSP) and Phase 7 (UI).

---

## Build

**Requirements:** Visual Studio 2022+ (Desktop C++ workload), CMake 3.25+, Git, WebView2 NuGet package (already installed).

```bash
# Configure (first time or after CMakeLists.txt changes)
cmake -B build

# Build Release
cmake --build build --config Release
```

**Outputs:**
- `build/SolaceSynth_artefacts/Release/Standalone/Solace Synth.exe`
- `build/SolaceSynth_artefacts/Release/VST3/Solace Synth.vst3`

There are no unit tests and no linting step currently. Build must succeed cleanly before committing — that is the only gate.

**UI hot-reload:** `UI/*.html`, `UI/*.css`, `UI/*.js` are served from disk at runtime via `SOLACE_DEV_UI_PATH`. Edit and save → close and reopen the plugin window → changes appear with no C++ rebuild needed.

---

## Architecture

### Two-Layer Design

The plugin is two largely independent layers:

1. **C++ audio engine** (`Source/`) — MIDI handling, voice synthesis, parameter management. Runs in real time on the audio thread.
2. **WebView UI** (`UI/`) — HTML/CSS/JS frontend embedded via JUCE 8's WebBrowserComponent (WebView2 on Windows). Runs in the browser process; communicates with C++ over a message-passing bridge.

These two layers are decoupled. The bridge is parameter-ID-agnostic — adding a new APVTS parameter in C++ automatically makes it available to JS via `setParameter(id, value)` / `parameterChanged(id, value)` without any bridge code changes.

### C++ Side

**`PluginProcessor`** owns:
- `juce::AudioProcessorValueTreeState apvts` — single source of truth for all parameters. All parameter I/O goes through APVTS.
- `juce::Synthesiser synth` — manages N polyphonic voices.
- `juce::MidiKeyboardState keyboardState` — thread-safe; shared with the editor for the on-screen piano.
- `SolaceLogger solaceLogger` — 3-file log cascade in `%TEMP%\SolaceSynth\`.

**`PluginEditor`** owns:
- `juce::WebBrowserComponent webView` — the entire plugin UI is this browser component.
- `juce::MidiKeyboardComponent midiKeyboard` — 80px native piano strip at the bottom of the window.
- The APVTS listener that forwards parameter changes to JS.

**`DSP/SolaceVoice`** — one instance per polyphonic voice. Runs entirely on the audio thread. All DSP modules (oscillators, filters, envelopes, LFO) are value members of `SolaceVoice`, not pointers, to avoid heap allocation on the audio thread.

**`DSP/SolaceSound`** — trivial tag class; tells the synthesiser this voice handles all MIDI notes and channels.

### JS Side

**`bridge.js`** — low-level wrapper around JUCE 8's native event protocol (`__juce__invoke` / `__juce__complete`). Exposes `SolaceBridge.setParameter()`, `SolaceBridge.signalUiReady()`, `SolaceBridge.log()`, and `SolaceBridge.onParameterChanged()`. Do not touch `window.__JUCE__.backend` directly — always go through `SolaceBridge`.

**`main.js`** — high-level UI logic: control binding, initialization sequence, JS logging. This is where each APVTS parameter gets wired to a UI control.

### Bridge Protocol

```
JS user interaction
  → SolaceBridge.setParameter(paramId, value)
  → __juce__invoke event { name: "setParameter", params: [paramId, value] }
  → C++ handleSetParameter() → param->setValueNotifyingHost(normalizedValue)
  → APVTS fires parameterChanged() on audio thread
  → bounced to message thread via callAsync()
  → webView->emitEventIfBrowserIsVisible("parameterChanged", { paramId, value })
  → JS SolaceBridge listener → UI control updates
```

The bridge handshake on startup:
1. JS calls `SolaceBridge.signalUiReady()` once the page loads.
2. C++ receives `handleUiReady()` → calls `sendAllParametersToJS()` to sync all current values.
3. JS receives `syncAllParameters([...])` → updates all controls to match current APVTS state.

### APVTS Parameter System

All plugin parameters must be registered in `PluginProcessor::createParameterLayout()`. The parameter ID string (e.g., `"masterVolume"`, `"ampAttack"`) is the single key used everywhere: in C++ `apvts.getRawParameterValue("id")`, in JS `SolaceBridge.setParameter("id", value)`, and in DAW automation lanes.

For audio-thread reads: use `apvts.getRawParameterValue("id")` which returns `std::atomic<float>*`. Read it with `->load()`. Never call `apvts.getParameter()` on the audio thread.

---

## Thread Safety — Critical Rules

**Audio thread** (`renderNextBlock`, `processBlock`, `parameterChanged`):
- Zero allocations, zero locks, zero disk I/O.
- `parameterChanged()` is called on the audio thread. It must only capture values and bounce to the message thread via `juce::MessageManager::callAsync()`.
- Always use `juce::Component::SafePointer<SolaceSynthEditor>` in `callAsync` lambdas — the editor can be destroyed before the lambda runs.

**Message thread** (everything UI-related):
- All logging (`SolaceLog::*`), all WebView calls, all JUCE component updates.

---

## Logging

Logs written to `%TEMP%\SolaceSynth\`:
- `trace.log` — everything
- `debug.log` — DEBUG and above
- `info.log` — INFO, WARN, ERROR only (cleanest view of lifecycle events)

C++ usage: `SolaceLog::info("msg")`, `SolaceLog::error("msg")`, etc. Never call from the audio thread.

JS usage: `logTrace()`, `logDebug()`, `logInfo()`, `logWarn()`, `logError()` in `main.js`. TRACE/DEBUG stay in the UI debug panel. INFO and above are forwarded to C++ logs via the bridge.

---

## Key Conventions

- **New DSP modules** (oscillators, filters, envelopes) go in `Source/DSP/` as header-only or `.h/.cpp` pairs. They are composed as value members inside `SolaceVoice` — one instance per voice.
- **New APVTS parameters** must be added to `createParameterLayout()` in `PluginProcessor.cpp` before any code references them.
- **Source file globbing:** `CMakeLists.txt` uses `GLOB` to pick up all `*.cpp` and `*.h` in `Source/`. New files in `Source/` are automatically included in the build after reconfiguring CMake (`cmake -B build`). Files in `Source/DSP/` are included via a separate glob.
- **Em dash (—):** Does not render in JUCE's default font. Use plain dashes in any C++ string that appears as JUCE text.
- **Plugin window:** Currently fixed size (Phase 5). Phase 7 will add `setResizable(true, false)` + `setResizeLimits()`. CSS must use relative units (`%`, `fr`) to scale correctly.
- **UI files for release:** Currently served from disk via `SOLACE_DEV_UI_PATH`. Before shipping, must be embedded via `juce_add_binary_data()`.
