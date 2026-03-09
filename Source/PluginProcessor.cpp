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

    // --- Phase 6.1: Polyphonic Synthesiser Setup ---
    // Build the parameter pointer struct. APVTS is fully initialised by this
    // point (it is a member initialised in the initialiser list before the
    // constructor body runs), so getRawParameterValue() is safe here.
    SolaceVoiceParams voiceParams;
    voiceParams.ampAttack  = apvts.getRawParameterValue ("ampAttack");
    voiceParams.ampDecay   = apvts.getRawParameterValue ("ampDecay");
    voiceParams.ampSustain = apvts.getRawParameterValue ("ampSustain");
    voiceParams.ampRelease = apvts.getRawParameterValue ("ampRelease");

    // Create 16 voices. We always create the maximum number up front — JUCE's
    // Synthesiser does not support safe voice addition/removal at runtime.
    // A polyphony cap (Phase 6.8) will be implemented inside startNote() later.
    constexpr int numVoices = 16;
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice (new SolaceVoice (voiceParams));

    // SolaceSound returns true for all MIDI notes and channels, so any
    // incoming note will trigger one of the SolaceVoice instances.
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
// Defines ALL automatable parameters for the plugin.
// IMPORTANT: Parameter IDs must be stable — changing them breaks saved
// presets and DAW automation. Never rename or remove an ID after shipping.
//
// Convention: {section}{ParamName}, version 1.
//   e.g. "ampAttack", "filterCutoff", "osc1Waveform"
// ============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout SolaceSynthProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // --- Master ---
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "masterVolume", 1 },
        "Master Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.8f));

    // --- Phase 6.1: Amplifier Envelope ---
    // Times in seconds. Sustain is a linear amplitude level (0.0–1.0).
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ampAttack", 1 },
        "Amp Attack",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f),
        0.01f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ampDecay", 1 },
        "Amp Decay",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f),
        0.1f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ampSustain", 1 },
        "Amp Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.8f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ampRelease", 1 },
        "Amp Release",
        juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f),
        0.3f));

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
    // Return the current amp release time so DAW hosts keep driving the plugin
    // long enough for the release tail to fully complete after transport stops
    // or during offline renders. Without this, a host receiving 0.0 will cut
    // audio immediately — the release phase is silently dropped in recordings.
    //
    // getRawParameterValue is const + noexcept in JUCE, safe to call here.
    // Default: 0.3s. Max: 10.0s (matches ampRelease APVTS range upper bound).
    // TODO (Phase 6.4): update to std::max(ampRelease, filterEnvRelease).
    auto* release = apvts.getRawParameterValue ("ampRelease");
    return release != nullptr ? static_cast<double> (release->load()) : 10.0;
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
    // Inform the Synthesiser of the sample rate. This must happen before
    // voice preparation so that getSampleRate() returns a valid value inside
    // each voice's prepare() call.
    synth.setCurrentPlaybackSampleRate (sampleRate);

    // Build a ProcessSpec and prepare each voice's DSP modules (Phase 6.1+).
    // Established pattern from production JUCE synths (BlackBird, ProPhat):
    //   iterate via getVoice(i) + dynamic_cast, call voice->prepare(spec).
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32> (getTotalNumOutputChannels());

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SolaceVoice*> (synth.getVoice (i)))
            voice->prepare (spec);

    SolaceLog::info ("prepareToPlay: sampleRate=" + juce::String (sampleRate)
        + " samplesPerBlock=" + juce::String (samplesPerBlock));
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
