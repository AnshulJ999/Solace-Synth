#include "PluginProcessor.h"
#include "PluginEditor.h"

// ============================================================================
// Constructor
// ============================================================================
SolaceSynthProcessor::SolaceSynthProcessor()
    : AudioProcessor (BusesProperties()
                        // Synth: no audio input, stereo output
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Set up file-based logging so we can capture debug output
    // Log file: %TEMP%/SolaceSynth/SolaceSynth.log
    auto logFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                       .getChildFile ("SolaceSynth")
                       .getChildFile ("SolaceSynth.log");

    fileLogger = std::make_unique<juce::FileLogger> (logFile, "Solace Synth Log", 1024 * 512); // 512KB max
    juce::Logger::setCurrentLogger (fileLogger.get());

    juce::Logger::writeToLog ("=== Solace Synth started ===");
    juce::Logger::writeToLog ("Log file: " + logFile.getFullPathName());
}

SolaceSynthProcessor::~SolaceSynthProcessor()
{
    juce::Logger::writeToLog ("=== Solace Synth shutting down ===");
    juce::Logger::setCurrentLogger (nullptr);
}

// ============================================================================
// Parameter Layout
//
// This defines ALL automatable parameters for the plugin.
// Currently just a master volume for testing. Full parameter set will be
// added when DSP modules are implemented.
//
// IMPORTANT: Parameter IDs must be stable — changing them breaks saved
// presets and DAW automation. Choose IDs carefully.
// ============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout SolaceSynthProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Master volume (0.0 to 1.0, default 0.8)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "masterVolume", 1 },
        "Master Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.8f));

    return { params.begin(), params.end() };
}

// ============================================================================
// Plugin Info
// ============================================================================
const juce::String SolaceSynthProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SolaceSynthProcessor::acceptsMidi() const
{
    return true; // Synth — always accepts MIDI
}

bool SolaceSynthProcessor::producesMidi() const
{
    return false;
}

bool SolaceSynthProcessor::isMidiEffect() const
{
    return false;
}

double SolaceSynthProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

// ============================================================================
// Programs (Presets) — minimal implementation for now
// ============================================================================
int SolaceSynthProcessor::getNumPrograms()
{
    return 1; // DAWs expect at least 1
}

int SolaceSynthProcessor::getCurrentProgram()
{
    return 0;
}

void SolaceSynthProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String SolaceSynthProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void SolaceSynthProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

// ============================================================================
// Audio Processing
// ============================================================================
void SolaceSynthProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // TODO: Initialize DSP modules here (oscillators, filters, envelopes)
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void SolaceSynthProcessor::releaseResources()
{
    // TODO: Free DSP resources if needed
}

bool SolaceSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Synth: no input required, output must be mono or stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void SolaceSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear all channels — silence for now
    // TODO: Replace with actual synth DSP (oscillator → filter → envelope → output)
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
}

// ============================================================================
// Editor
// ============================================================================
bool SolaceSynthProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* SolaceSynthProcessor::createEditor()
{
    return new SolaceSynthEditor (*this);
}

// ============================================================================
// State Persistence — save/load parameters for DAW recall
// ============================================================================
void SolaceSynthProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save the entire parameter tree as XML
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SolaceSynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore the parameter tree from XML
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
        }
    }
}

// ============================================================================
// Factory — JUCE calls this to create plugin instances
// ============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SolaceSynthProcessor();
}
