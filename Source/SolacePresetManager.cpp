#include "SolacePresetManager.h"
#include "SolaceLogger.h"
#include <SolaceFactoryPresetData.h>

// ============================================================================
// Constructor
// ============================================================================
SolacePresetManager::SolacePresetManager (juce::AudioProcessorValueTreeState& a)
    : apvts (a)
{
    // NOTE: scanPresets() is NOT called here because the logger may not be
    // initialized yet (member init order in Processor). The Processor
    // constructor calls scanPresets() explicitly after logger setup.
}

// ============================================================================
// Parse preset XML from string (for embedded BinaryData factory presets)
// ============================================================================
bool SolacePresetManager::parsePresetXml (
    const juce::String& xmlText,
    std::vector<std::pair<juce::String, float>>& outParams,
    juce::String& outName, juce::String& outAuthor)
{
    auto xml = juce::XmlDocument::parse (xmlText);
    if (xml == nullptr || ! xml->hasTagName ("SolacePreset"))
        return false;

    outName = xml->getStringAttribute ("name", "");
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

    // --- Factory presets from embedded BinaryData (.solace files) ---
    // Parse each embedded resource to extract name/author for the list.
    std::vector<PresetInfo> factoryPresets;

    for (int i = 0; i < SolaceFactoryPresetData::namedResourceListSize; ++i)
    {
        int dataSize = 0;
        const auto* data = SolaceFactoryPresetData::getNamedResource (
            SolaceFactoryPresetData::namedResourceList[i], dataSize);

        if (data == nullptr || dataSize == 0)
            continue;

        juce::String xmlText (juce::CharPointer_UTF8 (reinterpret_cast<const char*> (data)),
                              static_cast<size_t> (dataSize));

        juce::String name, author;
        std::vector<std::pair<juce::String, float>> dummyParams;

        if (parsePresetXml (xmlText, dummyParams, name, author) && name.isNotEmpty())
        {
            PresetInfo info;
            info.name = name;
            info.author = author;
            info.isFactory = true;
            factoryPresets.push_back (info);
        }
    }

    // Sort factory presets alphabetically, but keep "Default" at the front
    std::sort (factoryPresets.begin(), factoryPresets.end(),
        [] (const PresetInfo& a, const PresetInfo& b) {
            bool aIsDefault = a.name.equalsIgnoreCase ("Default");
            bool bIsDefault = b.name.equalsIgnoreCase ("Default");
            if (aIsDefault != bIsDefault) return aIsDefault;
            return a.name.compareIgnoreCase (b.name) < 0;
        });

    for (auto& info : factoryPresets)
    {
        info.listIndex = static_cast<int> (presets.size());
        presets.push_back (std::move (info));
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
    return "Default";
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

    // Parse the preset data BEFORE modifying any synth state.
    // If parsing fails, the current sound is preserved (no silent reset).
    std::vector<std::pair<juce::String, float>> paramValues;
    juce::String parsedName, parsedAuthor;

    if (preset.isFactory)
    {
        // Factory preset: find matching embedded BinaryData resource by name
        bool found = false;
        for (int i = 0; i < SolaceFactoryPresetData::namedResourceListSize; ++i)
        {
            int dataSize = 0;
            const auto* data = SolaceFactoryPresetData::getNamedResource (
                SolaceFactoryPresetData::namedResourceList[i], dataSize);

            if (data == nullptr || dataSize == 0)
                continue;

            juce::String xmlText (juce::CharPointer_UTF8 (reinterpret_cast<const char*> (data)),
                                  static_cast<size_t> (dataSize));

            if (parsePresetXml (xmlText, paramValues, parsedName, parsedAuthor)
                && parsedName == preset.name)
            {
                found = true;
                break;
            }

            paramValues.clear();
        }

        if (! found)
        {
            SolaceLog::error ("PresetManager: factory preset not found in BinaryData: " + preset.name);
            return false;
        }
    }
    else
    {
        // User preset: read from .solace file on disk
        if (! readPresetFile (preset.file, paramValues, parsedName, parsedAuthor))
        {
            SolaceLog::error ("PresetManager: failed to read preset file: " + preset.file.getFullPathName());
            return false;
        }
    }

    // Parse succeeded — now safe to modify synth state.
    // Set loading guard — suppresses isModified during bulk parameter sets.
    loadingPreset = true;

    // Reset to defaults first, then apply preset values.
    // This ensures any params missing from the file get their defaults.
    resetToDefaults();
    applyParameterValues (paramValues);

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
// Reset to defaults (Default preset values)
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
    if (loadPreset (next))
        return next;
    return currentIndex;  // return old index on failure
}

int SolacePresetManager::loadPreviousPreset()
{
    if (presets.empty()) return -1;
    int prev = currentIndex - 1;
    if (prev < 0) prev = static_cast<int> (presets.size()) - 1;
    if (loadPreset (prev))
        return prev;
    return currentIndex;  // return old index on failure
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

    // Move the file to the new name
    if (preset.file != newFile)
    {
        if (! preset.file.moveFileTo (newFile))
            return false;
    }
    else
    {
        // Case-only rename on case-insensitive filesystem (Windows) —
        // moveFileTo would fail because the OS sees src == dst.
        // Use a temp intermediary to force the rename.
        auto tempFile = getUserPresetDirectory().getChildFile ("__rename_temp__.solace");
        if (! preset.file.moveFileTo (tempFile) || ! tempFile.moveFileTo (newFile))
        {
            tempFile.deleteFile();  // cleanup on failure
            return false;
        }
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

    bool wasCurrentPreset = (index == currentIndex);

    rebuildPresetList();

    if (wasCurrentPreset)
    {
        // Deleted the active preset — load the nearest valid one (Default = index 0)
        int fallbackIndex = juce::jlimit (0, juce::jmax (0, static_cast<int> (presets.size()) - 1), index);
        loadPreset (fallbackIndex);
    }
    else
    {
        // Deleted a different preset — just fix currentIndex if it shifted
        if (index < currentIndex)
            --currentIndex;

        if (currentIndex >= static_cast<int> (presets.size()))
            currentIndex = 0;
    }

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

    // Store isFactory to disambiguate presets with the same name
    bool isFactory = false;
    if (currentIndex >= 0 && currentIndex < static_cast<int> (presets.size()))
        isFactory = presets[static_cast<size_t> (currentIndex)].isFactory;
    apvts.state.setProperty ("lastPresetIsFactory", isFactory, nullptr);
}

void SolacePresetManager::restoreFromState()
{
    auto savedName = apvts.state.getProperty ("lastPresetName", "Default").toString();
    bool savedIsFactory = static_cast<bool> (apvts.state.getProperty ("lastPresetIsFactory", true));

    // Find the preset by name + isFactory to disambiguate same-name presets
    for (int i = 0; i < static_cast<int> (presets.size()); ++i)
    {
        if (presets[static_cast<size_t> (i)].name.equalsIgnoreCase (savedName)
            && presets[static_cast<size_t> (i)].isFactory == savedIsFactory)
        {
            currentIndex = i;
            isModified = false;
            SolaceLog::info ("PresetManager: restored current preset to \"" + savedName + "\" (index " + juce::String (i) + ")");
            return;
        }
    }

    // Exact match not found — try name-only fallback
    for (int i = 0; i < static_cast<int> (presets.size()); ++i)
    {
        if (presets[static_cast<size_t> (i)].name.equalsIgnoreCase (savedName))
        {
            currentIndex = i;
            isModified = false;
            SolaceLog::info ("PresetManager: restored current preset to \"" + savedName + "\" (index " + juce::String (i) + ", isFactory mismatch)");
            return;
        }
    }

    // Preset not found — default to Default (index 0)
    currentIndex = 0;
    isModified = false;
    SolaceLog::warn ("PresetManager: saved preset \"" + savedName + "\" not found, defaulting to Default");
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
