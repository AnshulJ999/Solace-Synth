#include "SolacePresetManager.h"
#include "SolaceLogger.h"

// ============================================================================
// Constructor
// ============================================================================
SolacePresetManager::SolacePresetManager (juce::AudioProcessorValueTreeState& a)
    : apvts (a)
{
    scanPresets();
}

// ============================================================================
// Factory preset definitions
//
// Each entry lists only the parameters that differ from createParameterLayout()
// defaults. Unlisted parameters keep their default values.
// ============================================================================
std::vector<SolacePresetManager::FactoryPresetDef> SolacePresetManager::getFactoryPresetDefs()
{
    return {
        // 0: Init — all defaults (empty override list)
        { "Init", "Solace", {} },

        // 1: Fat Bass — thick low end
        { "Fat Bass", "Solace", {
            { "osc1Waveform", 1.0f },   // Saw
            { "osc2Waveform", 1.0f },   // Saw
            { "osc2Octave", 0.0f },
            { "oscMix", 0.5f },
            { "filterType", 1.0f },     // LP24
            { "filterCutoff", 800.0f },
            { "filterResonance", 0.2f },
            { "ampAttack", 0.005f },
            { "ampDecay", 0.2f },
            { "ampSustain", 0.7f },
            { "ampRelease", 0.15f },
            { "unisonCount", 4.0f },
            { "unisonDetune", 15.0f },
            { "unisonSpread", 0.4f },
        }},

        // 2: Pluck — short percussive
        { "Pluck", "Solace", {
            { "osc1Waveform", 1.0f },   // Saw
            { "filterCutoff", 5000.0f },
            { "filterEnvDepth", 0.8f },
            { "filterEnvAttack", 0.001f },
            { "filterEnvDecay", 0.3f },
            { "filterEnvSustain", 0.0f },
            { "ampAttack", 0.001f },
            { "ampDecay", 0.4f },
            { "ampSustain", 0.0f },
            { "ampRelease", 0.1f },
        }},

        // 3: Warm Pad — evolving texture
        { "Warm Pad", "Solace", {
            { "osc1Waveform", 3.0f },   // Triangle
            { "osc2Waveform", 0.0f },   // Sine
            { "oscMix", 0.6f },
            { "ampAttack", 0.8f },
            { "ampDecay", 0.5f },
            { "ampSustain", 0.7f },
            { "ampRelease", 1.5f },
            { "filterCutoff", 3000.0f },
            { "filterResonance", 0.15f },
            { "lfoTarget1", 1.0f },     // FilterCutoff
            { "lfoRate", 0.5f },
            { "lfoAmount", 0.3f },
        }},

        // 4: Lead — cutting mono-ish
        { "Lead", "Solace", {
            { "osc1Waveform", 2.0f },   // Square
            { "filterType", 1.0f },     // LP24
            { "filterCutoff", 4000.0f },
            { "filterResonance", 0.35f },
            { "ampAttack", 0.005f },
            { "ampDecay", 0.1f },
            { "ampSustain", 0.9f },
            { "ampRelease", 0.15f },
            { "masterDistortion", 0.15f },
        }},

        // 5: Supersaw — classic trance
        { "Supersaw", "Solace", {
            { "osc1Waveform", 1.0f },   // Saw
            { "osc2Waveform", 1.0f },   // Saw
            { "osc2Octave", 0.0f },
            { "oscMix", 0.5f },
            { "unisonCount", 7.0f },
            { "unisonDetune", 25.0f },
            { "unisonSpread", 0.8f },
            { "filterCutoff", 8000.0f },
            { "filterResonance", 0.1f },
            { "ampAttack", 0.01f },
            { "ampRelease", 0.3f },
        }},

        // 6: Keys — electric piano
        { "Keys", "Solace", {
            { "osc1Waveform", 0.0f },   // Sine
            { "osc2Waveform", 3.0f },   // Triangle
            { "osc2Octave", 1.0f },
            { "oscMix", 0.4f },
            { "ampAttack", 0.005f },
            { "ampDecay", 0.6f },
            { "ampSustain", 0.4f },
            { "ampRelease", 0.3f },
            { "filterCutoff", 6000.0f },
            { "filterEnvDepth", 0.4f },
            { "filterEnvDecay", 0.5f },
        }},

        // 7: Brass — synth brass stab
        { "Brass", "Solace", {
            { "osc1Waveform", 1.0f },   // Saw
            { "osc2Waveform", 1.0f },   // Saw
            { "osc2Octave", 0.0f },
            { "osc2Transpose", 7.0f },
            { "oscMix", 0.5f },
            { "ampAttack", 0.08f },
            { "ampSustain", 0.8f },
            { "filterCutoff", 2000.0f },
            { "filterEnvDepth", 0.6f },
            { "filterEnvAttack", 0.06f },
            { "filterEnvDecay", 0.4f },
            { "filterEnvSustain", 0.3f },
            { "unisonCount", 2.0f },
            { "unisonDetune", 10.0f },
        }},

        // 8: Sub — deep sub bass
        { "Sub", "Solace", {
            { "osc1Waveform", 0.0f },   // Sine
            { "oscMix", 0.0f },      // Osc1 only
            { "filterType", 1.0f },     // LP24
            { "filterCutoff", 400.0f },
            { "ampAttack", 0.005f },
            { "ampDecay", 0.1f },
            { "ampSustain", 1.0f },
            { "ampRelease", 0.1f },
        }},

        // 9: Dirty Lead — distorted
        { "Dirty Lead", "Solace", {
            { "osc1Waveform", 1.0f },   // Saw
            { "filterCutoff", 6000.0f },
            { "filterEnvDepth", 0.5f },
            { "filterEnvAttack", 0.001f },
            { "filterEnvDecay", 0.2f },
            { "ampAttack", 0.005f },
            { "ampSustain", 0.8f },
            { "masterDistortion", 0.6f },
            { "unisonCount", 2.0f },
            { "unisonDetune", 8.0f },
        }},
    };
}

// ============================================================================
// Non-sound properties — stripped from preset files
// ============================================================================
bool SolacePresetManager::isNonSoundProperty (const juce::String& name)
{
    return name == "lastEditorWidth"
        || name == "lastEditorHeight"
        || name == "lastPresetName";
}

// ============================================================================
// Scan / rebuild preset list
// ============================================================================
void SolacePresetManager::scanPresets()
{
    rebuildPresetList();
}

void SolacePresetManager::rebuildPresetList()
{
    presets.clear();

    // --- Factory presets (sorted alphabetically by name) ---
    auto factoryDefs = getFactoryPresetDefs();

    // Sort factory presets alphabetically
    std::sort (factoryDefs.begin(), factoryDefs.end(),
        [] (const FactoryPresetDef& a, const FactoryPresetDef& b) {
            return juce::String (a.name).compareIgnoreCase (juce::String (b.name)) < 0;
        });

    for (const auto& def : factoryDefs)
    {
        PresetInfo info;
        info.name = def.name;
        info.author = def.author;
        info.isFactory = true;
        info.listIndex = static_cast<int> (presets.size());
        presets.push_back (info);
    }

    // --- User presets (from Documents folder, sorted alphabetically) ---
    auto userDir = getUserPresetDirectory();
    if (userDir.isDirectory())
    {
        auto files = userDir.findChildFiles (juce::File::findFiles, false, "*.solace");
        files.sort();  // alphabetical by filename

        for (const auto& file : files)
        {
            // Quick parse: read just the name and author from the XML header
            juce::String name, author;
            std::vector<std::pair<juce::String, float>> dummyParams;

            if (readPresetFile (file, dummyParams, name, author))
            {
                PresetInfo info;
                info.name = name;
                info.author = author;
                info.isFactory = false;
                info.file = file;
                info.listIndex = static_cast<int> (presets.size());
                presets.push_back (info);
            }
        }
    }

    SolaceLog::info ("PresetManager: scanned " + juce::String (presets.size()) + " presets ("
                     + juce::String (std::count_if (presets.begin(), presets.end(),
                         [] (const PresetInfo& p) { return p.isFactory; }))
                     + " factory, "
                     + juce::String (std::count_if (presets.begin(), presets.end(),
                         [] (const PresetInfo& p) { return !p.isFactory; }))
                     + " user)");
}

// ============================================================================
// Current preset name / display name
// ============================================================================
juce::String SolacePresetManager::getCurrentPresetName() const
{
    if (currentIndex >= 0 && currentIndex < static_cast<int> (presets.size()))
        return presets[static_cast<size_t> (currentIndex)].name;
    return "Init";
}

juce::String SolacePresetManager::getDisplayName() const
{
    auto name = getCurrentPresetName();
    if (isModified)
        name += " *";
    return name;
}

// ============================================================================
// Load preset by index
// ============================================================================
bool SolacePresetManager::loadPreset (int index)
{
    if (index < 0 || index >= static_cast<int> (presets.size()))
    {
        SolaceLog::error ("PresetManager: loadPreset index out of range: " + juce::String (index));
        return false;
    }

    const auto& preset = presets[static_cast<size_t> (index)];

    // Set loading guard — suppresses isModified during bulk parameter sets
    loadingPreset = true;

    if (preset.isFactory)
    {
        // Factory preset: reset to defaults first, then apply overrides
        resetToDefaults();

        // Find the matching factory def and apply its parameter overrides
        auto factoryDefs = getFactoryPresetDefs();
        for (const auto& def : factoryDefs)
        {
            if (juce::String (def.name) == preset.name)
            {
                applyParameterValues (def.params);
                break;
            }
        }
    }
    else
    {
        // User preset: read from .solace file
        std::vector<std::pair<juce::String, float>> paramValues;
        juce::String name, author;

        if (! readPresetFile (preset.file, paramValues, name, author))
        {
            SolaceLog::error ("PresetManager: failed to read preset file: " + preset.file.getFullPathName());
            loadingPreset = false;
            return false;
        }

        // Reset to defaults first, then apply file values
        // This ensures any params missing from the file get their defaults
        resetToDefaults();
        applyParameterValues (paramValues);
    }

    currentIndex = index;
    isModified = false;
    loadingPreset = false;

    // Save current preset name to ValueTree for persistence
    saveToState();

    SolaceLog::info ("PresetManager: loaded preset \"" + preset.name + "\" (index " + juce::String (index) + ")");
    return true;
}

bool SolacePresetManager::loadPresetByName (const juce::String& name)
{
    for (int i = 0; i < static_cast<int> (presets.size()); ++i)
    {
        if (presets[static_cast<size_t> (i)].name.equalsIgnoreCase (name))
            return loadPreset (i);
    }

    SolaceLog::warn ("PresetManager: preset not found by name: " + name);
    return false;
}

// ============================================================================
// Reset to defaults (Init)
// ============================================================================
void SolacePresetManager::resetToDefaults()
{
    for (auto* param : apvts.processor.getParameters())
    {
        if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*> (param))
            rangedParam->setValueNotifyingHost (rangedParam->getDefaultValue());
    }
}

// ============================================================================
// Apply parameter values
// ============================================================================
void SolacePresetManager::applyParameterValues (
    const std::vector<std::pair<juce::String, float>>& paramValues)
{
    for (const auto& [paramId, value] : paramValues)
    {
        auto* param = apvts.getParameter (paramId);
        if (param != nullptr)
        {
            param->setValueNotifyingHost (param->convertTo0to1 (value));
        }
        else
        {
            SolaceLog::warn ("PresetManager: unknown parameter in preset: " + paramId);
        }
    }
}

// ============================================================================
// Navigation
// ============================================================================
int SolacePresetManager::loadNextPreset()
{
    if (presets.empty()) return -1;
    int next = (currentIndex + 1) % static_cast<int> (presets.size());
    loadPreset (next);
    return next;
}

int SolacePresetManager::loadPreviousPreset()
{
    if (presets.empty()) return -1;
    int prev = currentIndex - 1;
    if (prev < 0) prev = static_cast<int> (presets.size()) - 1;
    loadPreset (prev);
    return prev;
}

// ============================================================================
// Save user preset
// ============================================================================
bool SolacePresetManager::saveUserPreset (const juce::String& name)
{
    if (name.trim().isEmpty())
    {
        SolaceLog::error ("PresetManager: cannot save preset with empty name");
        return false;
    }

    ensureUserDirectoryExists();

    // Sanitize filename: strip invalid characters
    auto sanitized = name.trim();
    sanitized = sanitized.removeCharacters ("<>:\"/\\|?*");

    auto file = getUserPresetDirectory().getChildFile (sanitized + ".solace");

    if (! writePresetFile (file, sanitized, "User"))
    {
        SolaceLog::error ("PresetManager: failed to write preset: " + file.getFullPathName());
        return false;
    }

    SolaceLog::info ("PresetManager: saved preset \"" + sanitized + "\" to " + file.getFullPathName());

    // Rescan and find the new preset
    rebuildPresetList();

    // Set current to the newly saved preset
    for (int i = 0; i < static_cast<int> (presets.size()); ++i)
    {
        if (! presets[static_cast<size_t> (i)].isFactory
            && presets[static_cast<size_t> (i)].name == sanitized)
        {
            currentIndex = i;
            break;
        }
    }

    isModified = false;
    saveToState();
    return true;
}

// ============================================================================
// Rename user preset
// ============================================================================
bool SolacePresetManager::renameUserPreset (int index, const juce::String& newName)
{
    if (index < 0 || index >= static_cast<int> (presets.size()))
        return false;

    auto& preset = presets[static_cast<size_t> (index)];
    if (preset.isFactory)
    {
        SolaceLog::error ("PresetManager: cannot rename factory preset");
        return false;
    }

    auto sanitized = newName.trim().removeCharacters ("<>:\"/\\|?*");
    if (sanitized.isEmpty())
        return false;

    auto newFile = getUserPresetDirectory().getChildFile (sanitized + ".solace");

    // Read current file, rewrite with new name
    std::vector<std::pair<juce::String, float>> paramValues;
    juce::String oldName, author;
    if (! readPresetFile (preset.file, paramValues, oldName, author))
        return false;

    // Write new file with new name
    // We need to write the current params from the old file, not from APVTS
    // So we write the old file's content to the new file with updated name
    if (preset.file != newFile)
    {
        // Delete old file first if renaming (not just case change)
        preset.file.deleteFile();
    }

    // Write preset using current APVTS state... actually we want to preserve
    // the file's original param values. But since we just loaded them into
    // paramValues, we can reconstruct. However, the simplest approach for rename:
    // just rename the file and update the XML name attribute.
    if (! preset.file.moveFileTo (newFile))
    {
        // If move failed (e.g., case-only rename on Windows), try copy+delete
        if (! preset.file.copyFileTo (newFile))
            return false;
        preset.file.deleteFile();
    }

    // Update the name inside the XML
    auto xml = juce::XmlDocument::parse (newFile);
    if (xml != nullptr && xml->hasTagName ("SolacePreset"))
    {
        xml->setAttribute ("name", sanitized);
        xml->writeTo (newFile);
    }

    SolaceLog::info ("PresetManager: renamed \"" + oldName + "\" to \"" + sanitized + "\"");

    rebuildPresetList();

    // Update currentIndex to point to the renamed preset
    for (int i = 0; i < static_cast<int> (presets.size()); ++i)
    {
        if (! presets[static_cast<size_t> (i)].isFactory
            && presets[static_cast<size_t> (i)].name == sanitized)
        {
            currentIndex = i;
            break;
        }
    }

    saveToState();
    return true;
}

// ============================================================================
// Delete user preset
// ============================================================================
bool SolacePresetManager::deleteUserPreset (int index)
{
    if (index < 0 || index >= static_cast<int> (presets.size()))
        return false;

    const auto& preset = presets[static_cast<size_t> (index)];
    if (preset.isFactory)
    {
        SolaceLog::error ("PresetManager: cannot delete factory preset");
        return false;
    }

    if (! preset.file.deleteFile())
    {
        SolaceLog::error ("PresetManager: failed to delete: " + preset.file.getFullPathName());
        return false;
    }

    SolaceLog::info ("PresetManager: deleted preset \"" + preset.name + "\"");

    rebuildPresetList();

    // If we deleted the current preset, fall back to Init
    if (currentIndex >= static_cast<int> (presets.size()))
        currentIndex = 0;

    saveToState();
    return true;
}

// ============================================================================
// Modified state
// ============================================================================
void SolacePresetManager::setModified (bool modified)
{
    isModified = modified;
}

// ============================================================================
// Persistence — save/restore lastPresetName to/from APVTS ValueTree
// ============================================================================
void SolacePresetManager::saveToState()
{
    apvts.state.setProperty ("lastPresetName", getCurrentPresetName(), nullptr);
}

void SolacePresetManager::restoreFromState()
{
    auto savedName = apvts.state.getProperty ("lastPresetName", "Init").toString();

    // Find the preset by name and set currentIndex (do NOT reload the file —
    // the APVTS state already has the correct parameter values)
    for (int i = 0; i < static_cast<int> (presets.size()); ++i)
    {
        if (presets[static_cast<size_t> (i)].name.equalsIgnoreCase (savedName))
        {
            currentIndex = i;
            isModified = false;
            SolaceLog::info ("PresetManager: restored current preset to \"" + savedName + "\" (index " + juce::String (i) + ")");
            return;
        }
    }

    // Preset not found — default to Init
    currentIndex = 0;
    isModified = false;
    SolaceLog::warn ("PresetManager: saved preset \"" + savedName + "\" not found, defaulting to Init");
}

// ============================================================================
// User preset existence check
// ============================================================================
bool SolacePresetManager::userPresetExists (const juce::String& name) const
{
    auto sanitized = name.trim().removeCharacters ("<>:\"/\\|?*");
    auto file = getUserPresetDirectory().getChildFile (sanitized + ".solace");
    return file.existsAsFile();
}

// ============================================================================
// File I/O — User preset directory
// ============================================================================
juce::File SolacePresetManager::getUserPresetDirectory() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("Solace Synth")
        .getChildFile ("Presets")
        .getChildFile ("User");
}

void SolacePresetManager::ensureUserDirectoryExists() const
{
    auto dir = getUserPresetDirectory();
    if (! dir.isDirectory())
    {
        dir.createDirectory();
        SolaceLog::info ("PresetManager: created user preset directory: " + dir.getFullPathName());
    }
}

// ============================================================================
// File I/O — Write .solace file
//
// Format:
//   <SolacePreset name="..." author="..." version="1">
//       <Param id="masterVolume" value="0.8"/>
//       <Param id="ampAttack" value="0.01"/>
//       ...
//   </SolacePreset>
//
// Only synth parameters are written. UI/session metadata is excluded.
// ============================================================================
bool SolacePresetManager::writePresetFile (const juce::File& file,
                                           const juce::String& name,
                                           const juce::String& author)
{
    auto root = std::make_unique<juce::XmlElement> ("SolacePreset");
    root->setAttribute ("name", name);
    root->setAttribute ("author", author);
    root->setAttribute ("version", 1);

    for (auto* param : apvts.processor.getParameters())
    {
        if (auto* paramWithID = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
        {
            auto* rangedParam = apvts.getParameter (paramWithID->getParameterID());
            if (rangedParam != nullptr)
            {
                auto* paramEl = root->createNewChildElement ("Param");
                paramEl->setAttribute ("id", paramWithID->getParameterID());
                paramEl->setAttribute ("value", rangedParam->convertFrom0to1 (rangedParam->getValue()));
            }
        }
    }

    return root->writeTo (file);
}

// ============================================================================
// File I/O — Read .solace file
// ============================================================================
bool SolacePresetManager::readPresetFile (const juce::File& file,
                                          std::vector<std::pair<juce::String, float>>& outParams,
                                          juce::String& outName,
                                          juce::String& outAuthor)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName ("SolacePreset"))
    {
        SolaceLog::error ("PresetManager: invalid preset file: " + file.getFullPathName());
        return false;
    }

    outName = xml->getStringAttribute ("name", file.getFileNameWithoutExtension());
    outAuthor = xml->getStringAttribute ("author", "Unknown");

    outParams.clear();
    for (auto* paramEl : xml->getChildWithTagNameIterator ("Param"))
    {
        auto id = paramEl->getStringAttribute ("id");
        auto value = static_cast<float> (paramEl->getDoubleAttribute ("value", 0.0));

        if (id.isNotEmpty())
            outParams.emplace_back (id, value);
    }

    return true;
}
