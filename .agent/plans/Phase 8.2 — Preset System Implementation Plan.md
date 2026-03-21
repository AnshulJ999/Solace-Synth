# Phase 8.2 — Preset System Implementation Plan

**Created:** 2026-03-21
**Status:** PLANNING
**Depends on:** APVTS parameter system (complete), bridge protocol (complete)

---

## Overview

Full preset system: save, load, browse, rename, delete, and cycle through presets. Ships with ~10 factory presets. User presets stored on disk. Last-used preset restored on launch.

---

## 1. File Format

**Extension:** `.solace`
**Format:** XML (matches JUCE's native APVTS serialization -- no custom parser needed)

```xml
<?xml version="1.0"?>
<SolacePreset name="Fat Bass" author="Solace" version="1">
  <APVTS>
    <!-- APVTS parameter state only (UI/session metadata stripped) -->
    <!-- Generated from apvts.copyState().createXml() with non-sound properties removed -->
  </APVTS>
</SolacePreset>
```

**Why XML over JSON:** JUCE's ValueTree serializes natively to XML. Using XML means we call `apvts.copyState().createXml()` to save and `ValueTree::fromXml()` to load -- zero custom parsing. JSON would require manual parameter-by-parameter serialization.

**CRITICAL: Sound-only serialization.** The APVTS ValueTree also contains UI/session metadata (`lastEditorWidth`, `lastEditorHeight`, `lastPresetName`). These MUST be stripped before writing to a `.solace` file. On load, only parameter state is restored -- UI properties are preserved from the current session, not overwritten by the preset.

Implementation: `writePresetFile()` copies the state, removes known non-sound properties, then serializes. `loadPreset()` merges only parameter children into the current state, leaving UI properties untouched.

**Metadata fields:**
- `name` -- display name (also used as filename: `Fat Bass.solace`)
- `author` -- "Solace" for factory, user-editable for user presets
- `version` -- format version for future compatibility (start at 1)

---

## 2. Storage Layout

```
Documents/Solace Synth/Presets/
    User/
        My Cool Bass.solace
        Ambient Pad.solace
```

**Factory presets:** Embedded in the binary via `juce_add_binary_data()`. Read-only at runtime. Source files live in `Assets/Presets/Factory/` in the repo for easy editing by Anshul/Nabeel.

**User presets:** Written to `Documents/Solace Synth/Presets/User/`. Created on first save. Users can manually browse/organize this folder.

**Path resolution:**
- Windows: `juce::File::getSpecialLocation(userDocumentsDirectory) / "Solace Synth" / "Presets" / "User"`
- Future Mac: same pattern with `~/Documents/`

---

## 3. C++ Architecture

### New class: `SolacePresetManager` (`Source/SolacePresetManager.h` / `.cpp`)

Owned by `SolaceSynthProcessor`. Holds the preset list, current preset index, and handles all file I/O.

```cpp
class SolacePresetManager
{
public:
    SolacePresetManager (juce::AudioProcessorValueTreeState& apvts);

    // --- Preset list ---
    struct PresetInfo {
        juce::String name;
        juce::String author;
        bool isFactory;
        juce::File file;           // empty for factory (embedded)
        int factoryIndex;          // -1 for user presets
    };

    const std::vector<PresetInfo>& getPresetList() const;
    int getCurrentPresetIndex() const;
    juce::String getCurrentPresetName() const;

    // --- Load / Save ---
    bool loadPreset (int index);                           // by list index
    bool loadPresetByName (const juce::String& name);      // by name lookup
    bool saveUserPreset (const juce::String& name);        // save current state
    bool saveAsUserPreset (const juce::String& name);      // duplicate to new name

    // --- Manage ---
    bool renameUserPreset (int index, const juce::String& newName);
    bool deleteUserPreset (int index);

    // --- Navigation ---
    void loadNextPreset();
    void loadPreviousPreset();

    // --- Init (hardcoded, file-independent) ---
    void resetToDefaults();      // set all params to createParameterLayout() defaults

    // --- Modified state ---
    bool getIsModified() const;
    void setModified (bool modified);

    // --- Lifecycle ---
    void scanPresets();          // rebuild list from factory + user directory
    juce::String getLastUsedPresetName() const;
    void setLastUsedPresetName (const juce::String& name);

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<PresetInfo> presets;         // factory first, then user, alphabetical within
    int currentIndex = -1;                  // -1 = no preset loaded (init state)
    juce::String lastUsedPresetName;
    bool isModified = false;                // true after any param change post-load

    juce::File getUserPresetDirectory() const;
    void ensureUserDirectoryExists() const;

    // Factory presets from BinaryData
    void loadFactoryPresets();

    // User presets from disk
    void loadUserPresets();

    // Serialize/deserialize
    bool writePresetFile (const juce::File& file, const juce::String& name,
                          const juce::String& author);
    bool readPresetFile (const juce::File& file, juce::ValueTree& outState,
                         juce::String& outName, juce::String& outAuthor);
    bool readPresetFromMemory (const char* data, int size, juce::ValueTree& outState,
                               juce::String& outName, juce::String& outAuthor);
};
```

### Integration with SolaceSynthProcessor

```cpp
// PluginProcessor.h -- add:
#include "SolacePresetManager.h"

class SolaceSynthProcessor : public juce::AudioProcessor {
    // ...existing...
    SolacePresetManager& getPresetManager() { return presetManager; }
private:
    SolacePresetManager presetManager;  // constructed with apvts reference
};
```

**State persistence -- last used preset:**
- `getStateInformation()`: `lastPresetName` is already in the ValueTree (written on every preset load). Serialized automatically with `apvts.copyState()`.
- `setStateInformation()`: `replaceState()` restores the full APVTS state including `lastPresetName`. PresetManager reads this property and sets `currentIndex` for display. **Do NOT re-load the `.solace` file** -- the APVTS state already contains the correct parameter values (including any user tweaks made after loading the preset).

**Why not use JUCE's program API (`getNumPrograms`/`setCurrentProgram`):** The program API is designed for DAW-managed preset lists. It doesn't support user presets, categories, or file-based management. Most modern plugins (Serum, Vital, Surge) ignore the program API and implement their own preset system, which is what we're doing.

---

## 4. Bridge Extensions

### New native functions (JS -> C++)

Register in `createWebViewOptions()`:

| Function | Args | Returns | Description |
|----------|------|---------|-------------|
| `getPresetList` | none | `[{name, author, isFactory, index}...]` | Full preset list for dropdown |
| `loadPreset` | `index` (int) | `{success, name}` | Load preset by list index |
| `savePreset` | `name` (string) | `{success}` | Overwrite current user preset |
| `saveAsPreset` | `name` (string) | `{success, index}` | Save as new user preset |
| `renamePreset` | `index, newName` | `{success}` | Rename user preset |
| `deletePreset` | `index` (int) | `{success}` | Delete user preset |
| `nextPreset` | none | `{name, index}` | Cycle forward |
| `prevPreset` | none | `{name, index}` | Cycle backward |

### New events (C++ -> JS)

| Event | Payload | When |
|-------|---------|------|
| `presetListChanged` | `[{name, author, isFactory, index}...]` | After save/delete/rename/scan |
| `currentPresetChanged` | `{name, index}` | After any load/cycle |

### Bridge flow for preset load:
```
User clicks preset in dropdown
  -> JS calls SolaceBridge.loadPreset(index)
  -> C++ PresetManager reads .solace file / BinaryData
  -> C++ strips UI metadata, merges parameter state into current APVTS
  -> C++ explicitly calls sendAllParametersToJS() for bulk sync
  -> C++ emits currentPresetChanged event with new name
  -> C++ resets isModified flag to false
  -> JS updates preset-name label (no asterisk)
```

**Explicit bulk sync after load:** After restoring preset state, the Editor explicitly calls `sendAllParametersToJS()` to push all parameter values to JS in one batch. This is more reliable than depending on 35+ individual `parameterChanged` callbacks bouncing through `callAsync`. The per-parameter listener path still works for ongoing automation/UI changes -- bulk sync is only for preset loads.

---

## 5. JS Side -- Preset UI

### Existing elements (already in index.html):
- `#btn-preset-prev` -- left arrow button
- `#preset-dropdown` -- clickable preset name area
- `#preset-name` -- span showing "Initial Preset"
- `#btn-preset-next` -- right arrow button

### New UI elements needed:

**Management buttons** -- positioned above the preset bar in the top-bar area:

```html
<div class="preset-management">
    <button class="preset-mgmt-btn" id="btn-preset-delete" title="Delete">Delete</button>
    <button class="preset-mgmt-btn" id="btn-preset-rename" title="Rename">Rename</button>
    <button class="preset-mgmt-btn" id="btn-preset-save" title="Save">Save</button>
    <button class="preset-mgmt-btn" id="btn-preset-save-as" title="Save As">Save As</button>
</div>
```

**Hierarchical dropdown** -- opens on clicking `#preset-dropdown`:

```
+---------------------------+
| Factory              >    |  <- hover shows submenu
| User                 >    |
+---------------------------+

Submenu (Factory):           Submenu (User):
+-------------------+        +-------------------+
| Brass             |        | Ambient Pad       |
| Dirty Lead        |        | My Cool Bass      |
| Fat Bass          |        +-------------------+
| Init              |
| Keys              |
| Lead              |
| Pad               |
| Pluck             |
| Sub               |
| Supersaw          |
+-------------------+
```

**Save As modal** -- simple WebView overlay:

```
+--------------------------------------+
|          Save Preset                 |
|                                      |
|  Name: [________________________]    |
|                                      |
|        [Cancel]    [Save]            |
+--------------------------------------+
```

**Rename modal** -- same layout, pre-filled with current name.

**Delete confirmation** -- simple confirm dialog:

```
+--------------------------------------+
|     Delete "My Cool Bass"?           |
|                                      |
|  This cannot be undone.              |
|                                      |
|        [Cancel]    [Delete]          |
+--------------------------------------+
```

### New JS file: `UI/components/preset-browser.js`

Responsibilities:
- Wire click handlers for prev/next/dropdown/save/save-as/rename/delete
- Build and position the hierarchical dropdown menu
- Show/hide modals for save-as, rename, delete confirmation
- Call `SolaceBridge.loadPreset()`, `savePreset()`, etc.
- Listen for `presetListChanged` and `currentPresetChanged` events
- Update `#preset-name` text

### Bridge additions: `UI/bridge.js`

Add wrapper methods:
```js
// New public API methods
loadPreset(index)       { return callNativeFunction("loadPreset", index); }
savePreset(name)        { return callNativeFunction("savePreset", name); }
saveAsPreset(name)      { return callNativeFunction("saveAsPreset", name); }
renamePreset(index, n)  { return callNativeFunction("renamePreset", index, n); }
deletePreset(index)     { return callNativeFunction("deletePreset", index); }
nextPreset()            { return callNativeFunction("nextPreset"); }
prevPreset()            { return callNativeFunction("prevPreset"); }
getPresetList()         { return callNativeFunction("getPresetList"); }
```

Add event listeners in `setupEventListeners()`:
```js
window.__JUCE__.backend.addEventListener("presetListChanged", ...);
window.__JUCE__.backend.addEventListener("currentPresetChanged", ...);
```

---

## 6. Factory Presets

### Source location: `Assets/Presets/Factory/`

Each `.solace` file is created by setting parameters in the plugin, then using a dev tool or code path to export the current APVTS state. Alternatively, we can manually construct the XML from known parameter values.

### Preset list (10 presets):

| # | Name | Character | Key Parameter Settings |
|---|------|-----------|----------------------|
| 1 | **Init** | Clean default | All parameters at their `createParameterLayout()` defaults. This is the "blank canvas" preset. |
| 2 | **Fat Bass** | Thick low end | osc1Waveform=1(Saw), osc2Waveform=1(Saw), osc2Octave=0, filterType=1(LP24), filterCutoff=800, filterResonance=0.2, ampAttack=0.005, ampDecay=0.2, ampSustain=0.7, ampRelease=0.15, unisonCount=4, unisonDetune=15, unisonSpread=0.4 |
| 3 | **Pluck** | Short percussive | osc1Waveform=1(Saw), filterCutoff=5000, filterEnvDepth=0.8, filterEnvAttack=0.001, filterEnvDecay=0.3, filterEnvSustain=0.0, ampAttack=0.001, ampDecay=0.4, ampSustain=0.0, ampRelease=0.1 |
| 4 | **Warm Pad** | Evolving texture | osc1Waveform=3(Triangle), osc2Waveform=0(Sine), oscMix=0.6, ampAttack=0.8, ampDecay=0.5, ampSustain=0.7, ampRelease=1.5, filterCutoff=3000, filterResonance=0.15, lfoTarget1=3(FilterCutoff), lfoRate=0.5, lfoAmount=0.3 |
| 5 | **Lead** | Cutting mono-ish | osc1Waveform=2(Square), filterType=1(LP24), filterCutoff=4000, filterResonance=0.35, ampAttack=0.005, ampDecay=0.1, ampSustain=0.9, ampRelease=0.15, masterDistortion=0.15 |
| 6 | **Supersaw** | Classic trance | osc1Waveform=1(Saw), osc2Waveform=1(Saw), osc2Octave=0, oscMix=0.5, unisonCount=7, unisonDetune=25, unisonSpread=0.8, filterCutoff=8000, filterResonance=0.1, ampAttack=0.01, ampRelease=0.3 |
| 7 | **Keys** | Electric piano | osc1Waveform=0(Sine), osc2Waveform=3(Triangle), osc2Octave=1, oscMix=0.4, ampAttack=0.005, ampDecay=0.6, ampSustain=0.4, ampRelease=0.3, filterCutoff=6000, filterEnvDepth=0.4, filterEnvDecay=0.5 |
| 8 | **Brass** | Synth brass stab | osc1Waveform=1(Saw), osc2Waveform=1(Saw), osc2Octave=0, osc2Transpose=7, oscMix=0.5, ampAttack=0.08, ampSustain=0.8, filterCutoff=2000, filterEnvDepth=0.6, filterEnvAttack=0.06, filterEnvDecay=0.4, filterEnvSustain=0.3, unisonCount=2, unisonDetune=10 |
| 9 | **Sub** | Deep sub bass | osc1Waveform=0(Sine), oscMix=0.0, filterType=1(LP24), filterCutoff=400, ampAttack=0.005, ampDecay=0.1, ampSustain=1.0, ampRelease=0.1 |
| 10 | **Dirty Lead** | Distorted | osc1Waveform=1(Saw), filterCutoff=6000, filterEnvDepth=0.5, filterEnvAttack=0.001, filterEnvDecay=0.2, ampAttack=0.005, ampSustain=0.8, masterDistortion=0.6, unisonCount=2, unisonDetune=8 |

### Embedding in build:

```cmake
# CMakeLists.txt -- add factory presets to binary data
file(GLOB FactoryPresetFiles "${CMAKE_SOURCE_DIR}/Assets/Presets/Factory/*.solace")

juce_add_binary_data(SolacePresetData
    NAMESPACE SolacePresetBinaryData
    HEADER_NAME SolacePresetBinaryData.h
    SOURCES ${FactoryPresetFiles}
)

target_link_libraries(SolaceSynth PRIVATE SolacePresetData)
```

This creates a separate binary data target for presets (keeps UI assets separate for clarity).

---

## 7. Last-Used Preset Persistence

### How it works:

1. **On preset load:** `PresetManager` stores the preset name in APVTS ValueTree as a property: `state.setProperty("lastPresetName", name, nullptr)`

2. **On `getStateInformation()`:** The property is automatically included in the XML state (it's part of the ValueTree)

3. **On `setStateInformation()`:** The property is automatically restored via `replaceState()`. After restoration, `PresetManager` reads it and sets `currentIndex` accordingly.

4. **On standalone launch:** The standalone wrapper calls `setStateInformation()` with saved state from previous session -> last preset name is restored -> but we do NOT re-load the preset file (the APVTS state already has the correct parameter values from the saved state). We only restore the name for display.

5. **On DAW project load:** Same as standalone -- the DAW saves/restores full APVTS state. We just need the name for display.

**Important distinction:** We don't re-load the `.solace` file on state restoration. The APVTS state already contains the correct parameter values. We only need the preset name so the UI shows which preset is active. If the user tweaked parameters after loading a preset, those tweaks are preserved in the APVTS state.

### Modified state indicator:

After loading a preset, if the user changes any parameter, the display shows `"Fat Bass *"` (asterisk appended) to indicate unsaved changes.

**Implementation:**
- `PresetManager` holds a `bool isModified = false` flag
- On preset load: set `isModified = false`
- On any parameter change (via the existing APVTS listener): set `isModified = true`, emit `currentPresetChanged` with updated name + `*`
- On save: set `isModified = false`, emit `currentPresetChanged` with clean name
- The flag is NOT persisted -- on DAW restore, the preset name shows clean (the tweaked state IS the "saved" state from the DAW's perspective)

**Behavior matrix:**

| Action | Display | isModified |
|--------|---------|-----------|
| Load "Fat Bass" | `Fat Bass` | false |
| Tweak cutoff | `Fat Bass *` | true |
| Save | `Fat Bass` | false |
| Save As "My Bass" | `My Bass` | false |
| Load factory, tweak, close DAW, reopen | `Fat Bass` | false (DAW state is authoritative) |

**"Save" on a modified factory preset:** Acts as "Save As" -- opens name dialog pre-filled with factory name. Factory presets are never overwritten.

**"Save" on a modified user preset:** Overwrites the user preset file with current state. Resets `isModified` to false.

---

## 8. Implementation Order

### Step 1: SolacePresetManager class (C++ only, no UI)
- Create `Source/SolacePresetManager.h` and `Source/SolacePresetManager.cpp`
- Implement file read/write, factory preset loading from BinaryData, user directory management
- Add to `SolaceSynthProcessor` as a member
- Wire into `getStateInformation`/`setStateInformation` for last-used preset name
- **Test:** Manually call save/load from C++ code, verify files are created and state is restored

### Step 2: Factory presets
- Create `Assets/Presets/Factory/` directory
- Generate the 10 `.solace` files (can be done programmatically via a helper or manually)
- Add `juce_add_binary_data()` for presets in CMakeLists.txt
- Verify factory presets load correctly from BinaryData
- **Test:** Build, verify factory presets appear in list

### Step 3: Bridge extensions
- Add 8 native functions to `createWebViewOptions()` in PluginEditor.cpp
- Add 2 new C++ -> JS events (`presetListChanged`, `currentPresetChanged`)
- Add wrapper methods to `bridge.js`
- **Test:** Call from browser console, verify responses

### Step 4: JS preset browser UI
- Create `UI/components/preset-browser.js`
- Wire prev/next arrow buttons to `nextPreset()`/`prevPreset()`
- Build hierarchical dropdown (Factory/User categories with submenu)
- Update `#preset-name` on preset change
- Add CSS for dropdown menu and management buttons
- **Test:** Full UI flow -- click dropdown, select preset, cycle with arrows

### Step 5: Save / Save As / Rename / Delete
- Add management buttons to HTML
- Implement modal dialogs (Save As, Rename, Delete confirmation)
- Wire to bridge functions
- Factory presets: Save/Save As creates a user copy. Rename/Delete disabled.
- **Test:** Full CRUD flow for user presets

### Step 6: Modified indicator + polish
- Wire `isModified` flag in PresetManager: set true on any parameter change, false on load/save
- Emit `currentPresetChanged` with `*` suffix when modified
- JS updates `#preset-name` to show/hide asterisk
- Handle edge cases: duplicate names, invalid characters in filenames, empty name, leading/trailing spaces
- Ensure dropdown closes on click-outside, Escape key
- Test with DAW: verify preset name persists across DAW project save/load, modified flag resets correctly

---

## 9. Edge Cases and Decisions

| Scenario | Behavior |
|----------|----------|
| User clicks "Save" on a factory preset | Treated as "Save As" -- opens name dialog (pre-filled with factory name), saves to User/ |
| User clicks "Save" on a modified user preset | Overwrites the user preset file, resets modified indicator |
| User clicks "Delete" on a factory preset | Button disabled / greyed out for factory presets |
| User clicks "Rename" on a factory preset | Button disabled / greyed out for factory presets |
| Duplicate preset name on save (within User) | Prompt: "A preset named X already exists. Overwrite?" |
| User preset has same name as factory preset | Allowed -- they are in separate categories (Factory/User), both visible |
| Invalid filename characters | Strip `< > : " / \ | ? *` from name before saving |
| Leading/trailing spaces in name | Trimmed automatically |
| Empty preset name | Reject, keep modal open with error message |
| Preset file missing on load | Remove from list, show brief error, rescan presets |
| User manually adds .solace files to folder | Picked up on next scan (scan on editor open) |
| Parameter changed after preset load | Show `*` next to name (e.g. `Fat Bass *`). isModified = true. |
| "Init" preset | Always available. Hardcoded in C++ via `resetToDefaults()` -- programmatically sets all parameters to their `createParameterLayout()` defaults. Does NOT depend on a file. Even if all preset files are deleted, Init always works. |

### Name collision rules:
- **Factory vs User:** Separate namespaces. A user preset CAN have the same display name as a factory preset. No shadowing -- both remain visible in their respective dropdown categories.
- **Within User:** Duplicate names prompt "Overwrite?" confirmation. Case-insensitive comparison on Windows (NTFS is case-insensitive).
- **Filename derivation:** Display name = filename (minus `.solace` extension). `Fat Bass` -> `Fat Bass.solace`.
- **Case-only renames:** Allowed -- rename "fat bass" to "Fat Bass" overwrites the same file (Windows is case-insensitive but case-preserving).

---

## 10. Files to Create/Modify

### New files:
- `Source/SolacePresetManager.h` -- class declaration
- `Source/SolacePresetManager.cpp` -- implementation
- `UI/components/preset-browser.js` -- JS preset browsing/management UI
- `Assets/Presets/Factory/*.solace` -- 10 factory preset files

### Modified files:
- `Source/PluginProcessor.h` -- add `SolacePresetManager` member + getter
- `Source/PluginProcessor.cpp` -- construct `PresetManager`, wire into state persistence
- `Source/PluginEditor.cpp` -- add 8 native functions to `createWebViewOptions()`, add 2 events
- `UI/bridge.js` -- add preset wrapper methods and event listeners
- `UI/main.js` -- initialize preset browser, wire to DOM elements
- `UI/index.html` -- add management buttons HTML
- `UI/styles.css` -- add styles for dropdown menu, management buttons, modals
- `CMakeLists.txt` -- add `SolacePresetData` binary data target for factory presets

---

## 11. Risks and Mitigations

| Risk | Mitigation |
|------|-----------|
| `apvts.replaceState()` causes audio glitches | Parameter smoothing in DSP handles abrupt value changes. Test with long-release pads. |
| BinaryData naming collisions (preset names with spaces) | JUCE mangles filenames for BinaryData. Use `originalFilenames[]` to map back, same as UI assets. |
| User deletes all presets including Init | Init is a factory preset (read-only). Always available. |
| Thread safety on preset load | `replaceState()` is called on message thread. Parameter reads on audio thread use `atomic<float>*` -- already safe. |
| Hierarchical dropdown positioning at screen edges | Use `getBoundingClientRect()` to detect overflow and flip direction (same approach as existing dropdown.js) |
