# Solace Synth Full Code Review

Date: 2026-03-10
Reviewer: Codex
Scope: Full repo review across DSP, processor/editor, UI bridge/frontend, and build wiring
Mode: Read-mostly review with one new report file added at user request

## Summary

The codebase is structurally promising. The JUCE processor/editor split is clean, the DSP has been modularized sensibly, and the WebView bridge is understandable and mostly well-contained.

The main risks are:

1. A process-global logging design that breaks in multi-instance DAW sessions.
2. An LFO implementation that does not actually deliver the “free-running per-voice” behavior claimed by the plan/comments.
3. A UI layer that still binds several controls to parameters that do not exist in C++, so parts of the interface are misleading and non-functional.
4. A small but real S&H LFO startup bug.
5. A remaining web-font dependency that is fragile in plugin/offline environments.

## Findings

### 1. Global logger is unsafe across multiple plugin instances

- Severity: High
- Priority: P1
- Complexity to fix: Medium
- Files:
  - `/G:/GitHub/Solace-Synth/Source/PluginProcessor.cpp:15`
  - `/G:/GitHub/Solace-Synth/Source/PluginProcessor.cpp:82`
  - `/G:/GitHub/Solace-Synth/Source/SolaceLogger.h:121`

#### Problem

Logging is process-global, but each plugin instance installs its own `SolaceLogger`, and every instance destructor clears the global logger unconditionally.

In a multi-instance DAW session:

- Opening a second instance steals logging from the first.
- Closing either instance can disable logging for the other one.

#### Evidence

- `juce::Logger::setCurrentLogger (solaceLogger.get())` is called in each processor constructor.
- `juce::Logger::setCurrentLogger (nullptr)` is called in each processor destructor.
- `SolaceLog::*` helpers resolve through `juce::Logger::getCurrentLogger()`, so all logging depends on one shared global pointer.

#### Why it matters

This produces confusing or missing logs exactly when debugging multi-instance behavior in a DAW, and it can also create ownership/lifecycle surprises around shutdown.

#### Recommendation

Move away from a single global logger for per-instance state. Options:

- Keep logging fully per-instance and avoid `juce::Logger::setCurrentLogger()` entirely.
- Or add explicit shared/global lifetime management with ref-counting and only clear the logger when the last instance shuts down.

---

### 2. The LFO is not actually free-running across idle voices

- Severity: Medium
- Priority: P1
- Complexity to fix: Medium
- Files:
  - `/G:/GitHub/Solace-Synth/Source/DSP/SolaceVoice.h:395`
  - `/G:/GitHub/Solace-Synth/Source/DSP/SolaceVoice.h:443`
  - `/G:/GitHub/Solace-Synth/Source/DSP/SolaceLFO.h:189`

#### Problem

The code and project memory describe the LFO as “free-running from voice allocation,” but idle voices do not advance their LFO phase at all. The LFO only advances while a voice is actively rendering samples.

That means a fresh chord can still start with multiple voices phase-aligned at `0.0`, which does not reliably produce the promised per-voice shimmer.

#### Evidence

- `SolaceLFO` starts with `currentAngle = 0.0`.
- `SolaceVoice` never advances the LFO outside `renderNextBlock()`.
- `lfo.getNextSample()` is only called inside the active sample loop.

#### Why it matters

This is not just a comment mismatch. It changes audible behavior and invalidates the “free-running per-voice” design claim used to justify the current implementation.

#### Recommendation

If true free-running behavior is required, use one of these patterns:

- Give each voice a unique initial LFO phase on construction.
- Or advance each voice’s LFO independently of note activity.
- Or explicitly re-scope the feature/documentation to “not key-synced” rather than “free-running from allocation.”

---

### 3. Several UI controls are wired to parameters that do not exist in C++

- Severity: Medium
- Priority: P1
- Complexity to fix: Low to Medium
- Files:
  - `/G:/GitHub/Solace-Synth/UI/main.js:366`
  - `/G:/GitHub/Solace-Synth/UI/main.js:396`
  - `/G:/GitHub/Solace-Synth/UI/main.js:402`
  - `/G:/GitHub/Solace-Synth/UI/main.js:407`
  - `/G:/GitHub/Solace-Synth/UI/main.js:408`
  - `/G:/GitHub/Solace-Synth/UI/main.js:409`
  - `/G:/GitHub/Solace-Synth/Source/PluginEditor.cpp:238`
  - `/G:/GitHub/Solace-Synth/Source/PluginProcessor.cpp:95`

#### Problem

The UI binds controls for parameters that are not present in the APVTS layout:

- `masterDistortion`
- `voiceCount`
- `unisonCount`
- `velocityRange`
- `velocityModTarget1`
- `velocityModTarget2`

When users interact with these controls, JS sends updates to C++, but C++ rejects them as unknown parameters.

#### Evidence

- `UI/main.js` binds these controls.
- `PluginProcessor.cpp` does not register matching APVTS parameters.
- `PluginEditor.cpp` logs an error and returns `false` when `getParameter(paramId)` fails.

#### Why it matters

Parts of the UI look interactive and “working” even though they have no backend effect. That makes the current UI an unreliable representation of synth state and will confuse manual testing.

#### Recommendation

Pick one of these paths per control:

- Hide/disable controls until the backend exists.
- Or implement the APVTS parameters and DSP behavior.
- Or add explicit UI affordances that mark those controls as planned/not-yet-active.

---

### 4. Sample-and-hold LFO outputs centre for its entire first cycle

- Severity: Low
- Priority: P2
- Complexity to fix: Low
- Files:
  - `/G:/GitHub/Solace-Synth/Source/DSP/SolaceLFO.h:120`
  - `/G:/GitHub/Solace-Synth/Source/DSP/SolaceLFO.h:197`

#### Problem

`sAndHValue` starts at `0.0f` and is only replaced after the first phase wrap. As a result, S&H produces no modulation for its whole first cycle.

At slow rates, that can make S&H feel broken or inert when first selected.

#### Recommendation

Seed the held value immediately when entering S&H mode, on construction, or lazily on first use.

---

### 5. UI styling still depends on Google Fonts at runtime

- Severity: Low
- Priority: P3
- Complexity to fix: Low
- Files:
  - `/G:/GitHub/Solace-Synth/UI/styles.css:19`

#### Problem

The plugin UI still imports fonts from Google at runtime.

In offline/plugin-hosted environments, this is unreliable and can lead to inconsistent rendering or delayed visual fallback.

#### Recommendation

Bundle fonts locally before relying on the current design system as production-ready.

## Clean Checks

These areas looked good on this pass:

- `PluginEditor.cpp` now uses `SafePointer` in the delayed keyboard-focus callback, which removes the earlier use-after-free risk.
- `PluginEditor.cpp` correctly bounces APVTS listener work off the audio thread with `MessageManager::callAsync`.
- `resourceRequested()` guards against path traversal with `isAChildOf()` plus existence checks.
- DSP hot paths in `SolaceVoice`, `SolaceOscillator`, `SolaceFilter`, and `SolaceLFO` are allocation-free.
- The oscillator phase-wrap bug from phase 6.2 is fixed in the current tree.
- The earlier filter-state reuse issue is addressed by resetting the filter at note-on.
- The JUCE `Random` implementation used for the per-instance LFO RNG is genuinely per-instance/randomized in this local JUCE version; that part is fine.

## Files Reviewed Completely

- `/G:/GitHub/Solace-Synth/CMakeLists.txt`
- `/G:/GitHub/Solace-Synth/README.md`
- `/G:/GitHub/Solace-Synth/Source/PluginProcessor.h`
- `/G:/GitHub/Solace-Synth/Source/PluginProcessor.cpp`
- `/G:/GitHub/Solace-Synth/Source/PluginEditor.h`
- `/G:/GitHub/Solace-Synth/Source/PluginEditor.cpp`
- `/G:/GitHub/Solace-Synth/Source/SolaceLogger.h`
- `/G:/GitHub/Solace-Synth/Source/DSP/SolaceADSR.h`
- `/G:/GitHub/Solace-Synth/Source/DSP/SolaceFilter.h`
- `/G:/GitHub/Solace-Synth/Source/DSP/SolaceLFO.h`
- `/G:/GitHub/Solace-Synth/Source/DSP/SolaceOscillator.h`
- `/G:/GitHub/Solace-Synth/Source/DSP/SolaceSound.h`
- `/G:/GitHub/Solace-Synth/Source/DSP/SolaceVoice.h`
- `/G:/GitHub/Solace-Synth/UI/index.html`
- `/G:/GitHub/Solace-Synth/UI/bridge.js`
- `/G:/GitHub/Solace-Synth/UI/main.js`
- `/G:/GitHub/Solace-Synth/UI/styles.css`
- `/G:/GitHub/Solace-Synth/build/_deps/juce-src/modules/juce_core/maths/juce_Random.h`
- `/G:/GitHub/Solace-Synth/build/_deps/juce-src/modules/juce_core/maths/juce_Random.cpp`

## Review Limits

- I did not run the standalone app, build, or listening tests in this pass.
- This report focuses on concrete bugs, state/lifecycle issues, and misleading/incomplete behavior, not stylistic preferences.
