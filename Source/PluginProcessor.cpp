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
    // Set up multi-level file logging
    // Log files: %TEMP%/SolaceSynth/ (trace.log, debug.log, info.log)
    solaceLogger = std::make_unique<SolaceLogger>();
    juce::Logger::setCurrentLogger (solaceLogger.get());

    SolaceLog::info ("=== Solace Synth started ===");
    SolaceLog::info ("Log directory: " + solaceLogger->getLogDirectory());

    // --- Phase 5: Polyphonic Synthesiser Setup ---
    // Add voices. Each SolaceVoice can play one note at a time.
    // Number of voices = maximum polyphony. 8 is a good starting point.
    constexpr int numVoices = 8;
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice (new SolaceVoice());

    // Add our sound descriptor.
    // SolaceSound::appliesToNote() and appliesToChannel() return true for all inputs,
    // so any note on any channel will trigger a SolaceVoice.
    synth.addSound (new SolaceSound());

    SolaceLog::info ("Synthesiser ready: " + juce::String (numVoices) + " voices, SolaceSound");
}

SolaceSynthProcessor::~SolaceSynthProcessor()
{
    SolaceLog::info ("=== Solace Synth shutting down ===");
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
    // The Synthesiser must know the sample rate before rendering.
    // This converts MIDI note frequencies to the correct angleDelta per sample.
    synth.setCurrentPlaybackSampleRate (sampleRate);

    SolaceLog::info ("prepareToPlay: sampleRate=" + juce::String (sampleRate)
        + " samplesPerBlock=" + juce::String (samplesPerBlock));

    juce::ignoreUnused (samplesPerBlock);
}

void SolaceSynthProcessor::releaseResources()
{
    // The Synthesiser manages its own voice state; nothing extra to free here.
    // Add cleanup here if future modules (reverb, filter, etc.) need it.
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
    juce::ScopedNoDenormals noDenormals;

    // --- Clear output buffer ---
    // The Synthesiser uses addSample() so it mixes INTO the buffer.
    // We must clear it first, otherwise we accumulate garbage.
    buffer.clear();

    // --- Inject on-screen keyboard MIDI ---
    // MidiKeyboardState collects note events from the MidiKeyboardComponent
    // (UI thread). We extract them here (audio thread) and merge them with
    // any incoming hardware MIDI — so both sources trigger the Synthesiser.
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    // --- Render all active voices ---
    // The Synthesiser scans the now-merged midiMessages buffer for events,
    // routes them to SolaceVoice instances, and renders audio for this block.
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // --- Apply master volume from APVTS ---
    // Get the current (possibly automated) value and scale all channels.
    auto masterVolume = apvts.getRawParameterValue ("masterVolume")->load();

    for (int channel = 0; channel < getTotalNumOutputChannels(); ++channel)
        buffer.applyGain (channel, 0, buffer.getNumSamples(), masterVolume);
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
