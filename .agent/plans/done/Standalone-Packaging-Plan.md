# Solace Synth — Standalone Packaging Plan
*Written: 2026-03-11*

## Goal

Make the **Standalone** build portable enough to send to Nabeel without requiring your local repo checkout.

Primary target:
- Nabeel can run `Solace Synth.exe` on his Windows machine
- UI loads correctly
- Synth works
- Any remaining prerequisites are clearly documented

---

## Current Blocker

The app is **not portable yet** because the WebView UI is loaded from the compile-time absolute path:

- `SOLACE_DEV_UI_PATH="${CMAKE_SOURCE_DIR}/UI"` in `CMakeLists.txt`
- `PluginEditor.cpp` reads UI files from that absolute disk location

That means the current standalone EXE works on Anshul's machine because the repo exists at the expected path, but it is **not safe to send as-is**.

---

## Recommended Approach

### Best next step: embed the UI into the binary

Use JUCE `juce_add_binary_data()` for the `UI/` files and serve them through the existing `WebBrowserComponent::ResourceProvider`.

This is the right middle ground because it is:
- proper
- fast enough to do now
- robust for both Standalone and later VST3 distribution

### Why this is the right choice

Pros:
- No dependency on repo path
- No need to ship loose HTML/CSS/JS files beside the EXE
- Same resource provider architecture stays intact
- Works cleanly for both Standalone and plugin targets

Cons:
- Requires a small CMake + C++ refactor
- UI changes will require rebuilds once fully embedded

---

## Implementation Plan

### Step 1 — Add a BinaryData target for UI assets

In `CMakeLists.txt`:
- create a `juce_add_binary_data()` target for all required UI files
- include:
  - `UI/index.html`
  - `UI/styles.css`
  - `UI/bridge.js`
  - `UI/main.js`
  - `UI/components/*.js`
  - `UI/assets/icons/*`
  - future fonts if added

Recommended target name:
- `SolaceUIAssets`

Then link that target into `SolaceSynth`.

### Step 2 — Load UI resources from BinaryData

In `PluginEditor.cpp`:
- replace disk-based `SOLACE_DEV_UI_PATH` loading for distribution mode
- map requested resource paths to embedded binary assets
- keep the existing MIME type handling

Recommended structure:
- use embedded assets as the default
- optionally keep a dev-only disk fallback behind a compile definition

### Step 3 — Add an optional dev-mode fallback

To preserve Nabeel/Anshul UI iteration speed during development:

Option A:
- keep `SOLACE_DEV_UI_PATH` support only in Debug/dev builds

Option B:
- add a compile definition like `SOLACE_ENABLE_DEV_UI_FALLBACK=1`
- resource provider tries disk first if enabled, otherwise uses embedded data

Recommended:
- **Debug/dev:** allow disk fallback
- **Release/distribution:** embedded UI only

That gives the team fast UI iteration without breaking distribution.

### Step 4 — Build and test the standalone from outside the repo context

Verification should include:
- EXE launches
- UI loads
- no missing assets
- controls respond
- sound engine works

Best test:
- copy the built standalone artifact to a different folder
- temporarily ensure it cannot see the repo UI path
- launch and confirm it still works

### Step 5 — Prepare a handoff bundle for Nabeel

Minimum demo bundle:
- `Solace Synth.exe`
- short text/markdown note with prerequisites

Optional better bundle:
- zipped folder named like `Solace-Synth-Standalone-Demo-Windows.zip`
- includes a tiny README with:
  - what it is
  - how to run it
  - prerequisite note for WebView2 / VC++ runtime if needed

---

## Runtime Dependencies

### 1. Microsoft Edge WebView2 Runtime

Required because the UI uses JUCE WebView2 on Windows.

Current Microsoft guidance:
- Windows 11 includes the Evergreen WebView2 Runtime
- many Windows 10 machines also already have it
- Microsoft still recommends distributing/checking for it because there are edge cases where it is missing

Practical take:
- **If Nabeel is on Windows 11, he most likely already has it**
- still not guaranteed enough to rely on blindly

### 2. Microsoft Visual C++ Redistributable (v14)

Likely required unless you explicitly switch to static runtime linking.

Current Microsoft guidance:
- apps built with modern MSVC toolsets use the current Visual C++ v14 Redistributable family
- VS 2015/2017/2019/2022/2026 builds are binary-compatible with the latest supported v14 redistributable

Practical take:
- many dev machines already have it
- normal non-dev user machines may or may not
- do **not** assume it is present unless you test on a clean machine

---

## Recommendation For Nabeel Handoff

### Fastest realistic demo handoff

After embedded UI is done:
1. send the standalone EXE (or zip)
2. ask Nabeel to try launching it
3. if it fails, the likely missing prerequisites are:
   - WebView2 Runtime
   - VC++ Redistributable

This is acceptable for a private demo.

### Proper V1 demo handoff

Ship:
- embedded-UI standalone build
- short README with 2 prerequisite links/instructions

This is the best balance of speed and professionalism.

### Full polished distribution later

Later, for public/demo-quality sharing:
- use an installer
- check/install prerequisites automatically
- optionally add app icon/version metadata polish

That is **not required** for the next handoff to Nabeel.

---

## Complexity / Effort Estimate

### Embedded UI packaging

Complexity:
- **Low to Medium**

Why:
- architecture is already close
- resource provider already exists
- only the data source needs to change

Estimated implementation time:
- **about 45–90 minutes** for a careful implementation
- **another 20–40 minutes** for verification and cleanup

So realistically:
- **same session / same evening is very possible**

### Installer-level polish

Complexity:
- **Medium**

Estimated later:
- a separate focused pass

---

## Recommended Order Of Work

### Tonight / next immediate task

1. Embed UI assets via `juce_add_binary_data()`
2. Make Standalone use embedded assets by default
3. Rebuild
4. Verify EXE still works away from the repo path
5. Create a zip for Nabeel

### Tomorrow or next follow-up

1. Verify on a second machine / clean environment
2. Decide whether to add an installer
3. Reconcile LFO target list mismatch
4. Continue Phase 7.5 UI work

---

## Acceptance Criteria

Packaging work is complete when all are true:

- Standalone EXE no longer depends on `G:/GitHub/Solace-Synth/UI`
- UI renders with all assets
- synth produces sound
- app runs from another folder
- Nabeel can launch it with only normal Windows prerequisites

---

## Notes

- This plan is for the **Standalone demo build**, not full plugin distribution yet.
- The same embedded-resource approach will also help later with VST3 portability.
- Do not spend time on a full installer before the embedded UI step is complete.
