# Solace Synth Full Repo Code Review

Date: 2026-03-10
Reviewer: Codex
Scope: Full repo review across DSP, processor/editor, UI bridge/frontend, build wiring, and project-memory accuracy
Mode: Read-only review plus this report file, written at user request

## Executive Summary

The codebase is in a good structural place. The processor/editor split is clean, the DSP has been broken into sensible modules, and the WebView bridge is understandable and mostly well-contained.

The main risks right now are:

1. A process-global logging design that breaks in multi-instance DAW sessions.
2. The WebView UI is still served from the source checkout path, so current binaries are not portable outside the repo/developer machine setup.
3. A UI that still exposes several controls with no backend/APVTS implementation.
4. An LFO implementation that does not fully match the "free-running per-voice" behavior claimed in code comments and project memory.
5. Project memory that now contains incorrect claims about the LFO internals, which can mislead future AI agents and future implementation work.
6. A small but real S&H startup behavior issue.
7. A remaining runtime font dependency that is fragile in plugin/offline environments.

## Findings

### 1. Global logger is unsafe across multiple plugin instances

- Severity: High
- Priority: P1
- Complexity: Medium
- Files:
  - `/G:/GitHub/Solace-Synth/Source/PluginProcessor.cpp:15`
  - `/G:/GitHub/Solace-Synth/Source/PluginProcessor.cpp:82`
  - `/G:/GitHub/Solace-Synth/Source/SolaceLogger.h:121`

Problem:

Logging is process-global, but each plugin instance installs its own `SolaceLogger`, and every instance destructor clears the global logger unconditionally.

In a multi-instance DAW session:

- Opening a second instance steals logging from the first.
- Closing either instance can disable logging for the rest.

Evidence:

- `juce::Logger::setCurrentLogger (solaceLogger.get())` is called in each processor constructor.
- `juce::Logger::setCurrentLogger (nullptr)` is called in each processor destructor.
- `SolaceLog::*` helpers resolve through `juce::Logger::getCurrentLogger()`, so all logging depends on one shared global pointer.

Recommendation:

- Move away from a single global logger for per-instance state.
- Or add explicit shared/global lifetime management and only clear the logger when the last instance shuts down.

### 2. The current WebView UI packaging is still tied to the source checkout path

- Severity: Medium
- Priority: P1
- Complexity: Medium
- Files:
  - `/G:/GitHub/Solace-Synth/CMakeLists.txt:98`
  - `/G:/GitHub/Solace-Synth/Source/PluginEditor.cpp:156`

Problem:

The current build bakes an absolute `SOLACE_DEV_UI_PATH` into the binary and serves the UI from the repo's `UI/` folder at runtime.

That is acceptable for local development, but it means the current plugin/standalone build is not truly self-contained or portable. If the binary is moved to another machine or the source tree is absent, the WebView UI will fail to load.

Evidence:

- `CMakeLists.txt` defines `SOLACE_DEV_UI_PATH="${CMAKE_SOURCE_DIR}/UI"`.
- `PluginEditor.cpp` uses `juce::File uiDir (SOLACE_DEV_UI_PATH);` inside the resource provider.

Recommendation:

- Before treating builds as shareable, embed the UI assets with `juce_add_binary_data()` or an equivalent packaging path.
- Keep the current disk-backed path only as an explicit dev-mode fallback.

### 3. Several UI controls are wired to parameters that do not exist in C++

- Severity: Medium
- Priority: P1
- Complexity: Low to Medium
- Files:
  - `/G:/GitHub/Solace-Synth/UI/main.js:366`
  - `/G:/GitHub/Solace-Synth/UI/main.js:396`
  - `/G:/GitHub/Solace-Synth/UI/main.js:402`
  - `/G:/GitHub/Solace-Synth/UI/main.js:407`
  - `/G:/GitHub/Solace-Synth/UI/main.js:408`
  - `/G:/GitHub/Solace-Synth/UI/main.js:409`
  - `/G:/GitHub/Solace-Synth/Source/PluginEditor.cpp:238`
  - `/G:/GitHub/Solace-Synth/Source/PluginProcessor.cpp:95`

Problem:

The UI binds controls for parameters that are not present in the APVTS layout:

- `masterDistortion`
- `voiceCount`
- `unisonCount`
- `velocityRange`
- `velocityModTarget1`
- `velocityModTarget2`

When users interact with these controls, JS sends updates to C++, but C++ rejects them as unknown parameters.

Evidence:

- `UI/main.js` binds these controls.
- `PluginProcessor.cpp` does not register matching APVTS parameters.
- `PluginEditor.cpp` logs an error and returns `false` when `getParameter(paramId)` fails.

Recommendation:

- Hide/disable these controls until the backend exists.
- Or implement the APVTS parameters and DSP behavior.
- Or mark them clearly as planned/not-yet-active.

### 4. The LFO is not actually free-running across idle voices

- Severity: Medium
- Priority: P1
- Complexity: Medium
- Files:
  - `/G:/GitHub/Solace-Synth/Source/DSP/SolaceVoice.h:395`
  - `/G:/GitHub/Solace-Synth/Source/DSP/SolaceVoice.h:443`
  - `/G:/GitHub/Solace-Synth/Source/DSP/SolaceLFO.h:189`

Problem:

The code and project docs describe the LFO as "free-running from voice allocation", but idle voices do not advance their LFO phase at all. The LFO only advances while a voice is actively rendering samples.

That means a fresh chord can still start with multiple voices phase-aligned at `0.0`, which does not reliably produce the promised per-voice shimmer.

Evidence:

- `SolaceLFO` starts with `currentAngle = 0.0`.
- `SolaceVoice` never advances the LFO outside `renderNextBlock()`.
- `lfo.getNextSample()` is only called inside the active sample loop.

Recommendation:

- If true free-running behavior is required, give each voice a unique initial phase on construction.
- Or advance each voice's LFO independently of note activity.
- Or explicitly rescope the docs/comments to "not key-synced" instead of "free-running from allocation".

### 5. Project memory now contains incorrect facts about the LFO implementation

- Severity: Medium
- Priority: P2
- Complexity: Low
- Files:
  - `/G:/GitHub/Solace-Synth/.agent/synth-project-memory.md:227`
  - `/G:/GitHub/Solace-Synth/.agent/synth-project-memory.md:231`
  - `/G:/GitHub/Solace-Synth/Source/DSP/SolaceLFO.h:55`
  - `/G:/GitHub/Solace-Synth/build/_deps/juce-src/modules/juce_core/maths/juce_Random.cpp:42`

Problem:

Project memory claims:

- each voice has a random initial LFO phase from construction,
- `SolaceLFO` seeds that phase via `getSystemRandom()`,
- default `juce::Random()` uses `seed=1` for all instances unless manually re-seeded.

Those statements do not match the current code or the local JUCE implementation.

Evidence:

- `SolaceLFO()` is a default constructor with no random phase initialization.
- `currentAngle` still starts at `0.0`.
- Local JUCE `Random::Random()` calls `setSeedRandomly()` in this workspace.

Why it matters:

This is not just documentation drift. Future AI agents and future human contributors may make design decisions based on false repo memory, especially around LFO behavior and S&H randomness.

Recommendation:

- Correct `.agent/synth-project-memory.md` so it matches the actual implementation.
- Keep implementation facts in memory files strictly evidence-based.

### 6. Sample-and-hold LFO outputs centre for its entire first cycle

- Severity: Low
- Priority: P2
- Complexity: Low
- Files:
  - `/G:/GitHub/Solace-Synth/Source/DSP/SolaceLFO.h:120`
  - `/G:/GitHub/Solace-Synth/Source/DSP/SolaceLFO.h:197`

Problem:

`sAndHValue` starts at `0.0f` and is only replaced after the first phase wrap. As a result, S&H produces no modulation for its whole first cycle.

At slow rates, that can make S&H feel broken or inert when first selected.

Recommendation:

- Seed the held value immediately when entering S&H mode, on construction, or lazily on first use.

### 7. UI styling still depends on Google Fonts at runtime

- Severity: Low
- Priority: P3
- Complexity: Low
- Files:
  - `/G:/GitHub/Solace-Synth/UI/styles.css:19`

Problem:

The plugin UI still imports fonts from Google at runtime.

In offline/plugin-hosted environments, this is unreliable and can lead to inconsistent rendering or delayed visual fallback.

Recommendation:

- Bundle fonts locally before relying on the current design system as production-ready.

## Clean Checks

These areas looked good on this pass:

- `PluginEditor.cpp` now uses `SafePointer` in the delayed keyboard-focus callback, removing the earlier use-after-free risk.
- `PluginEditor.cpp` correctly bounces APVTS listener work off the audio thread with `MessageManager::callAsync`.
- `resourceRequested()` guards against path traversal with `isAChildOf()` plus existence checks.
- DSP hot paths in `SolaceVoice`, `SolaceOscillator`, `SolaceFilter`, and `SolaceLFO` are allocation-free.
- The oscillator phase-wrap bug from phase 6.2 is fixed in the current tree.
- The earlier filter-state reuse issue is addressed by resetting the filter at note-on.
- APVTS listener callbacks deliver denormalized values in this local JUCE version, so the `parameterChanged -> JS` bridge path is correct for implemented controls.
- Local JUCE `Random` is genuinely randomized per instance in this workspace, so the per-instance RNG object itself is fine.

## Suggested Fix Order

1. Fix logger lifecycle/global ownership.
2. Make UI packaging portable for non-repo/dev-machine runs.
3. Hide or implement the fake/live UI controls.
4. Decide whether the LFO should be truly free-running or just not key-synced, then align code and docs.
5. Correct project memory so future agents are not misled.
6. Fix S&H initial-value behavior.
7. Bundle fonts locally.

## Files Reviewed Completely

- `/G:/GitHub/Solace-Synth/CMakeLists.txt`
- `/G:/GitHub/Solace-Synth/README.md`
- `/G:/GitHub/Solace-Synth/.agent/synth-project-memory.md`
- `/G:/GitHub/Solace-Synth/.agent/plans/Phase 6 - DSP Roadmap.md`
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
- This report focuses on concrete bugs, state/lifecycle issues, cross-layer mismatches, and misleading/incomplete behavior, not style preferences.
