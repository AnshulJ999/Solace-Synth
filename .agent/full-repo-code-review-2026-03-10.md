# Solace Synth Full Repo Code Review

Date: 2026-03-10
Reviewer: Codex
Mode: Full-repo review, read-first audit with a written findings report

## Scope

Reviewed the current tracked repo across:

- Build/config: `CMakeLists.txt`, `README.md`
- JUCE app shell: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/SolaceLogger.h`
- DSP: `Source/DSP/SolaceADSR.h`, `Source/DSP/SolaceFilter.h`, `Source/DSP/SolaceLFO.h`, `Source/DSP/SolaceOscillator.h`, `Source/DSP/SolaceSound.h`, `Source/DSP/SolaceVoice.h`
- Frontend: `UI/index.html`, `UI/bridge.js`, `UI/main.js`, `UI/styles.css`

Also checked:

- Recent repo history (`git log --oneline -10`)
- Line-ending status (`git ls-files --eol`)
- Local JUCE `Random` implementation to verify the LFO S&H RNG claim

## Method

- Read full files, not snippets
- Focused on concrete correctness issues, regressions, lifecycle/state bugs, integration mismatches, and deployability risks
- Avoided speculative style-only comments

## Findings

### 1. Global logger ownership breaks in multi-instance host sessions

- Severity: High
- Priority: P1
- Complexity: Medium
- Files:
  - `Source/PluginProcessor.cpp:15`
  - `Source/PluginProcessor.cpp:82`
  - `Source/SolaceLogger.h:121`

Problem:

`juce::Logger::setCurrentLogger()` is process-global, but each plugin instance installs its own `SolaceLogger` in the constructor and clears the global logger unconditionally in the destructor. In a DAW session with multiple plugin instances, the most recently opened instance steals logging from earlier ones, and closing either instance can disable logging for the other remaining instances.

Evidence:

- Constructor installs a per-instance logger globally at `Source/PluginProcessor.cpp:15-16`
- Destructor clears the global logger at `Source/PluginProcessor.cpp:79-82`
- All logging helpers route through `juce::Logger::getCurrentLogger()` at `Source/SolaceLogger.h:121-153`

Why it matters:

This creates cross-instance interference and makes log behavior nondeterministic in real host sessions.

Recommended fix:

Centralize logger ownership so it is not tied 1:1 to a processor instance, or use reference-counted/shared lifetime management before clearing the global logger.

### 2. The built UI is still tied to the source checkout path

- Severity: High
- Priority: P1
- Complexity: Medium
- Files:
  - `CMakeLists.txt:98`
  - `Source/PluginEditor.cpp:156`

Problem:

The WebView resource provider serves files from `SOLACE_DEV_UI_PATH`, which is compiled as an absolute path to the repo's `UI/` folder. That means the built plugin/standalone app is not relocatable and will fail to load its real UI if the source tree is missing, moved, or built on another machine without the same path.

Evidence:

- `SOLACE_DEV_UI_PATH` is defined as `${CMAKE_SOURCE_DIR}/UI` in `CMakeLists.txt:98-100`
- The editor reads directly from that absolute path in `Source/PluginEditor.cpp:158-185`

Why it matters:

The binary currently depends on the development checkout layout. This is okay for local development, but it is a real deployment/runtime risk in the current tree.

Recommended fix:

Move to embedded assets (`juce_add_binary_data`) or another deployable packaging path before treating the build as portable.

### 3. The UI exposes controls for parameters that do not exist in APVTS yet

- Severity: Medium
- Priority: P1
- Complexity: Low
- Files:
  - `UI/main.js:366`
  - `UI/main.js:396`
  - `UI/main.js:402`
  - `UI/main.js:407`
  - `UI/main.js:408`
  - `UI/main.js:409`
  - `UI/index.html:218`
  - `UI/index.html:390`
  - `UI/index.html:403`
  - `UI/index.html:418`
  - `UI/index.html:431`
  - `Source/PluginEditor.cpp:238`
  - `Source/PluginProcessor.cpp:95`

Problem:

The frontend binds and renders controls for `masterDistortion`, `voiceCount`, `unisonCount`, `velocityRange`, `velocityModTarget1`, and `velocityModTarget2`, but those parameters are not registered in the current APVTS layout. Interacting with those controls sends bridge calls that fall into the editor's `"unknown param"` path instead of changing synth state.

Evidence:

- `UI/main.js:366-409` binds those six param IDs
- Matching controls are present in `UI/index.html:218-221`, `UI/index.html:390-434`, and `UI/index.html:418-424`
- The C++ side rejects unknown IDs in `Source/PluginEditor.cpp:238-252`
- The current `createParameterLayout()` in `Source/PluginProcessor.cpp:95-287` does not register those IDs

Why it matters:

The UI currently overstates what the engine actually supports. That creates misleading state, noisy error logs, and a poor test/debug loop.

Recommended fix:

Either hide/disable those controls until the corresponding phases land, or add a frontend capability gate so only real APVTS parameters are interactive.

### 4. The LFO is not actually free-running across idle voices

- Severity: Medium
- Priority: P2
- Complexity: Medium
- Files:
  - `Source/DSP/SolaceVoice.h:395`
  - `Source/DSP/SolaceVoice.h:443`
  - `Source/DSP/SolaceLFO.h:189`

Problem:

The code comments and project memory describe a free-running per-voice LFO that should give chord voices different phases, but the implementation only advances the LFO while a voice is actively rendering. Idle voices sit at angle 0 until first use.

Evidence:

- `SolaceLFO` starts with `currentAngle = 0.0` at `Source/DSP/SolaceLFO.h:189`
- The LFO phase only advances via `lfo.getNextSample()` in `Source/DSP/SolaceVoice.h:443`
- That call only happens inside `renderNextBlock()` for active voices

Why it matters:

On a fresh chord, newly allocated voices will often begin phase-aligned instead of phase-offset, so the promised “organic shimmer” is not reliably achieved.

Recommended fix:

If true free-running behavior is required, give each voice an independently advancing LFO even while idle, or assign a unique initial phase per voice at construction.

### 5. Sample-and-hold starts with a dead first cycle

- Severity: Low
- Priority: P3
- Complexity: Low
- Files:
  - `Source/DSP/SolaceLFO.h:120`
  - `Source/DSP/SolaceLFO.h:197`

Problem:

The sample-and-hold output starts at `0.0` and only receives a random held value after the first phase wrap. At slow LFO rates, that can leave the first full cycle sounding like “no modulation”.

Evidence:

- `sAndHValue` is initialized to `0.0f` at `Source/DSP/SolaceLFO.h:197`
- It is only updated on wrap at `Source/DSP/SolaceLFO.h:120-121`

Why it matters:

This makes the first use of S&H feel broken or delayed compared with typical S&H behavior.

Recommended fix:

Seed the held value immediately when entering S&H mode, at construction, or on first use.

### 6. The UI still depends on remote Google Fonts

- Severity: Low
- Priority: P3
- Complexity: Low
- Files:
  - `UI/styles.css:18`

Problem:

The current UI imports Google Fonts over the network. In plugin environments, network access is often restricted, unavailable, or undesirable.

Evidence:

- `@import url('https://fonts.googleapis.com/...')` in `UI/styles.css:18-20`
- The file itself already notes this should be replaced before release

Why it matters:

The UI degrades gracefully, but visual consistency depends on an external network resource right now.

Recommended fix:

Bundle local font assets and use `@font-face` before release.

## Clean Checks

These areas looked solid in the current tree:

- The oscillator phase-wrap bug from earlier review is fixed; `SolaceOscillator` now fully wraps with a loop
- The filter wrapper no longer relies on a protected JUCE API and uses a valid public processing path
- `filter.reset()` and `filterEnvelope.reset()` are now handled correctly at note start/reuse points
- APVTS parameter pointer population happens before voice construction
- UI resource provider correctly blocks path traversal with `isAChildOf`
- All tracked text files are currently LF in both index and working tree (`git ls-files --eol`)
- JUCE `Random` default construction does reseed per instance, so the “independent RNG per voice” claim is valid

## Testing Gaps

Not performed in this review:

- Fresh build verification by me
- Runtime listening tests
- Host-specific multi-instance verification
- Manual WebView behavior on a machine without repo-local `UI/` assets

## Bottom Line

The codebase is structurally healthy and the DSP architecture is moving in a good direction, but there are still three important repo-level issues before the project is truly robust:

1. global logger ownership is not safe in multi-instance use
2. the UI runtime path is still development-only
3. the frontend is ahead of the backend and currently exposes unsupported controls

The LFO/S&H issues are narrower and easier to fix, but they are still worth addressing before calling the modulation layer fully settled.
