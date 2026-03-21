#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

// ============================================================================
// SolacePresetManager — handles preset save/load/browse/manage
//
// Owns the preset list (factory + user), tracks the current preset and
// modified state. All file I/O happens on the message thread.
//
// Factory presets are .solace XML files in Assets/Presets/Factory/, embedded
// via juce_add_binary_data() at build time (SolaceFactoryPresetData namespace).
// User presets are .solace XML files in Documents/Solace Synth/Presets/User/.
//
// The .solace format stores only synth parameters — UI/session metadata
// (window size, last preset name) is stripped on write and preserved on load.
// ============================================================================

class SolacePresetManager
{
public:
    explicit SolacePresetManager (juce::AudioProcessorValueTreeState& apvts);

    // --- Preset info ---
    struct PresetInfo
    {
        juce::String name;
        juce::String author;
        bool isFactory = false;
        juce::File file;            // empty for factory presets
        int listIndex = 0;          // position in the unified preset list
    };

    const std::vector<PresetInfo>& getPresetList() const    { return presets; }
    int getCurrentPresetIndex() const                       { return currentIndex; }
    juce::String getCurrentPresetName() const;
    bool getIsModified() const                              { return isModified; }

    // Returns the display name: "Fat Bass" or "Fat Bass *" if modified
    juce::String getDisplayName() const;

    // --- Load ---
    bool loadPreset (int index);
    bool loadPresetByName (const juce::String& name);

    // --- Save ---
    bool saveUserPreset (const juce::String& name);

    // --- Manage ---
    bool renameUserPreset (int index, const juce::String& newName);
    bool deleteUserPreset (int index);

    // --- Navigation ---
    int loadNextPreset();
    int loadPreviousPreset();

    // --- Reset all parameters to their APVTS default values ---
    void resetToDefaults();

    // --- Modified state ---
    void setModified (bool modified);

    // --- Loading guard (prevents isModified during bulk param sets) ---
    bool isCurrentlyLoading() const { return loadingPreset; }

    // --- Lifecycle ---
    void scanPresets();

    // --- Persistence helpers (called by Processor) ---
    // Reads/writes lastPresetName from/to the APVTS ValueTree
    void restoreFromState();
    void saveToState();

    // --- Preset existence check ---
    bool userPresetExists (const juce::String& name) const;

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<PresetInfo> presets;
    int currentIndex = 0;       // default: Default (index 0)
    bool isModified = false;
    bool loadingPreset = false;  // guard: true during loadPreset to suppress isModified

    // --- File I/O ---
    juce::File getUserPresetDirectory() const;
    void ensureUserDirectoryExists() const;
    bool writePresetFile (const juce::File& file, const juce::String& name,
                          const juce::String& author);
    bool readPresetFile (const juce::File& file, std::vector<std::pair<juce::String, float>>& outParams,
                         juce::String& outName, juce::String& outAuthor);

    // Parse a .solace XML string (used for embedded factory presets from BinaryData)
    bool parsePresetXml (const juce::String& xmlText,
                         std::vector<std::pair<juce::String, float>>& outParams,
                         juce::String& outName, juce::String& outAuthor);

    // Apply a set of parameter values (used by both factory and user preset loading)
    void applyParameterValues (const std::vector<std::pair<juce::String, float>>& paramValues);

    // List management
    void rebuildPresetList();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SolacePresetManager)
};
