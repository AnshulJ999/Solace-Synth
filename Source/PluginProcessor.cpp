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

    // Build the parameter pointer struct. APVTS is fully initialised by this
    // point (it is a member initialised in the initialiser list before the
    // constructor body runs), so getRawParameterValue() is safe here.

    // --- Phase 6.1: Amplitude Envelope ---
    SolaceVoiceParams voiceParams;
    voiceParams.ampAttack  = apvts.getRawParameterValue ("ampAttack");
    voiceParams.ampDecay   = apvts.getRawParameterValue ("ampDecay");
    voiceParams.ampSustain = apvts.getRawParameterValue ("ampSustain");
    voiceParams.ampRelease = apvts.getRawParameterValue ("ampRelease");

    // --- Phase 6.2: Oscillator 1 Waveform + Tuning ---
    voiceParams.osc1Waveform  = apvts.getRawParameterValue ("osc1Waveform");
    voiceParams.osc1Octave    = apvts.getRawParameterValue ("osc1Octave");
    voiceParams.osc1Transpose = apvts.getRawParameterValue ("osc1Transpose");
    voiceParams.osc1Tuning    = apvts.getRawParameterValue ("osc1Tuning");

    // --- Phase 6.3: Filter ---
    voiceParams.filterCutoff    = apvts.getRawParameterValue ("filterCutoff");
    voiceParams.filterResonance = apvts.getRawParameterValue ("filterResonance");
    voiceParams.filterType      = apvts.getRawParameterValue ("filterType");

    // --- Phase 6.4: Filter Envelope ---
    voiceParams.filterEnvDepth   = apvts.getRawParameterValue ("filterEnvDepth");
    voiceParams.filterEnvAttack  = apvts.getRawParameterValue ("filterEnvAttack");
    voiceParams.filterEnvDecay   = apvts.getRawParameterValue ("filterEnvDecay");
    voiceParams.filterEnvSustain = apvts.getRawParameterValue ("filterEnvSustain");
    voiceParams.filterEnvRelease = apvts.getRawParameterValue ("filterEnvRelease");

    // --- Phase 6.5: Oscillator 2 + Osc Mix ---
    voiceParams.osc2Waveform  = apvts.getRawParameterValue ("osc2Waveform");
    voiceParams.osc2Octave    = apvts.getRawParameterValue ("osc2Octave");
    voiceParams.osc2Transpose = apvts.getRawParameterValue ("osc2Transpose");
    voiceParams.osc2Tuning    = apvts.getRawParameterValue ("osc2Tuning");
    voiceParams.oscMix        = apvts.getRawParameterValue ("oscMix");

    // --- Phase 6.6: LFO ---
    voiceParams.lfoWaveform = apvts.getRawParameterValue ("lfoWaveform");
    voiceParams.lfoRate     = apvts.getRawParameterValue ("lfoRate");
    voiceParams.lfoAmount   = apvts.getRawParameterValue ("lfoAmount");
    voiceParams.lfoTarget1  = apvts.getRawParameterValue ("lfoTarget1");
    voiceParams.lfoTarget2  = apvts.getRawParameterValue ("lfoTarget2");
    voiceParams.lfoTarget3  = apvts.getRawParameterValue ("lfoTarget3");

    // --- Phase 6.7: Unison ---
    voiceParams.unisonCount  = apvts.getRawParameterValue ("unisonCount");
    voiceParams.unisonDetune = apvts.getRawParameterValue ("unisonDetune");
    voiceParams.unisonSpread = apvts.getRawParameterValue ("unisonSpread");

    // --- Phase 6.8: Voicing Parameters ---
    voiceParams.velocityRange      = apvts.getRawParameterValue ("velocityRange");
    voiceParams.velocityModTarget1 = apvts.getRawParameterValue ("velocityModTarget1");
    voiceParams.velocityModTarget2 = apvts.getRawParameterValue ("velocityModTarget2");
    voiceParams.voiceCount         = apvts.getRawParameterValue ("voiceCount");

    // Create 16 voices. All 16 are always allocated -- SolaceSynthesiser::noteOn()
    // enforces the runtime voiceCount polyphony cap by reading the APVTS value each
    // processBlock and passing it to setVoiceLimit().
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

    // --- Phase 6.2: Oscillator 1 Waveform ---
    // AudioParameterInt: stored internally as a float in APVTS.
    // Waveform index: 0=Sine, 1=Sawtooth, 2=Square, 3=Triangle.
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "osc1Waveform", 1 },
        "Osc1 Waveform",
        0, 3, 0));  // default: 0 = Sine

    // --- Phase 6.2: Oscillator 1 Tuning ---
    // Octave and Transpose are AudioParameterInt (discrete steps).
    // Tuning (cents) is AudioParameterFloat (continuous fine-tune).
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "osc1Octave", 1 },
        "Osc1 Octave",
        -3, 3, 0));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "osc1Transpose", 1 },
        "Osc1 Transpose",
        -12, 12, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "osc1Tuning", 1 },
        "Osc1 Tuning",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.01f),
        0.0f));

    // --- Phase 6.3: Filter ---
    // filterCutoff uses a skew factor of 0.3 to make the slider feel logarithmic
    // (musical). Without skew, the slider spends ~90% of its travel above 10kHz
    // and the lower half — the musically interesting range — feels unusable.
    // The 4th argument to NormalisableRange is the skew factor (< 1 = log feel).
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterCutoff", 1 },
        "Filter Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f),
        20000.0f));  // default: fully open (no filtering)

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterResonance", 1 },
        "Filter Resonance",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f));

    // filterType: 0=LP12, 1=LP24 (default), 2=HP12
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "filterType", 1 },
        "Filter Type",
        0, 2, 1));  // default: 1 = LP24

    // --- Phase 6.4: Filter Envelope ---
    // filterEnvDepth is bipolar (-1.0 to +1.0): positive = cutoff sweeps up,
    // negative = cutoff sweeps down. Default 0.0 = no envelope effect on launch.
    // filterEnvSustain default 0.0: classic pluck response without extra config.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterEnvDepth", 1 },
        "Filter Env Depth",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f),
        0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterEnvAttack", 1 },
        "Filter Env Attack",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f),
        0.01f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterEnvDecay", 1 },
        "Filter Env Decay",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f),
        0.3f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterEnvSustain", 1 },
        "Filter Env Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterEnvRelease", 1 },
        "Filter Env Release",
        juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f),
        0.3f));

    // --- Phase 6.5: Oscillator 2 + Osc Mix ---
    // osc2Waveform default 2 = Square (matches plan — Figma shows Square as Osc2 default).
    // osc2Octave default 1 = one octave above Osc1 (plan: Figma design shows Octave=1).
    // oscMix default 0.5 = equal blend of both oscillators on first launch.
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "osc2Waveform", 1 },
        "Osc2 Waveform",
        0, 3, 2));  // default: 2 = Square

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "osc2Octave", 1 },
        "Osc2 Octave",
        -3, 3, 1));  // default: 1 = one octave above Osc1

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "osc2Transpose", 1 },
        "Osc2 Transpose",
        -12, 12, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "osc2Tuning", 1 },
        "Osc2 Tuning",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.01f),
        0.0f));

    // oscMix: 0.0 = Osc1 only, 1.0 = Osc2 only, 0.5 = equal blend (default).
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscMix", 1 },
        "Osc Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.5f));

    // --- Phase 6.6: LFO ---
    // lfoWaveform: int, 0=Sine, 1=Triangle, 2=Sawtooth, 3=Square, 4=S&H. Default 0=Sine.
    // lfoRate uses the same skew factor (0.3) as filterCutoff -- the musically useful
    // range (0.1-10 Hz) would be cramped at the bottom of a linear slider without it.
    // lfoAmount default 0.0 means LFO has zero effect on launch; user opts in.
    // lfoTarget1 default 1=FilterCutoff -- the most common/expected first LFO target.
    // lfoTarget2/3 default 0=None -- additional slots inactive until user assigns.
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "lfoWaveform", 1 },
        "LFO Waveform",
        0, 4, 0));  // default: 0 = Sine

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lfoRate", 1 },
        "LFO Rate",
        juce::NormalisableRange<float> (0.01f, 50.0f, 0.01f, 0.3f),
        1.0f));  // default: 1 Hz

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lfoAmount", 1 },
        "LFO Amount",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f));  // default: 0 = no effect on launch

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "lfoTarget1", 1 },
        "LFO Target 1",
        0, 7, 1));  // default: 1 = FilterCutoff

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "lfoTarget2", 1 },
        "LFO Target 2",
        0, 7, 0));  // default: 0 = None

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "lfoTarget3", 1 },
        "LFO Target 3",
        0, 7, 0));  // default: 0 = None

    // --- Phase 6.7: Unison ---
    // unisonCount: int 1-8. Default 1 = no unison (identical to pre-6.7 behaviour).
    //   At unisonCount=1, detuneOffset=0 and all pan gains collapse to centre --
    //   the output is audibly identical to Phase 6.6 (regression-safe default).
    // unisonDetune: 0-100 cents, default 0.0. Linear slider -- musical range is
    //   concentrated in the lower end (0-30 cents) so no skew needed.
    // unisonSpread: 0-1, default 0.5. Moderate width on first use. Full left/right
    //   (1.0) is too wide for casual listening; 0.5 gives a pleasant default image.
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "unisonCount", 1 },
        "Unison Count",
        1, 8, 1));  // default: 1 = no unison

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "unisonDetune", 1 },
        "Unison Detune",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        0.0f));  // default: 0 = no detune

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "unisonSpread", 1 },
        "Unison Spread",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.5f));  // default: 0.5 = moderate stereo width

    // --- Phase 6.8: Voicing Parameters ---
    // voiceCount: int 1-16. Default 16 = full polyphony. The polyphony cap is
    //   enforced per-block in processBlock() via SolaceSynthesiser::setVoiceLimit().
    //   When the cap is reached and a new note arrives, JUCE steals the oldest voice
    //   (standard hardware synth behaviour) rather than dropping the new note.
    // velocityRange: float 0.0-1.0. Bridge between velocity and the mod targets.
    //   0.0 = no dynamics (all notes at full level / full attack).
    //   1.0 = full sensitivity (level/attack scales with velocity).
    // velocityModTarget1/2: int 0-4.
    //   0=None, 1=AmpLevel, 2=AmpAttack, 3=FilterCutoff, 4=FilterResonance.
    //   Two routing slots; both can be active simultaneously.
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "voiceCount", 1 },
        "Voice Count",
        1, 16, 16));  // default: 16 = full polyphony

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "velocityRange", 1 },
        "Velocity Range",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        1.0f));  // default: 1.0 = full velocity sensitivity

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "velocityModTarget1", 1 },
        "Velocity Mod Target 1",
        0, 4, 2));  // default: 2 = AmpAttack (Figma default)

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "velocityModTarget2", 1 },
        "Velocity Mod Target 2",
        0, 4, 0));  // default: 0 = None

    // --- Phase 6.9: Master Distortion ---
    // Applied globally to the stereo mix after all voices have rendered.
    // tanh soft-clip: y = tanh(k*x) / tanh(k), k = 1 + drive * 9.
    // Default drive=0.0 leaves sound near-clean. drive=1.0 = heavy saturation.
    // Processing per-sample ensures correct non-linear behaviour
    // (applyGain() is linear and can't be used for per-sample saturation).
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "masterDistortion", 1 },
        "Master Distortion",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f));  // default: 0.0 = near-clean

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
    //
    // Returns ampRelease only — NOT max(ampRelease, filterEnvRelease).
    //
    // Rationale: audio output = filteredSample * kVoiceGain * velocityScale * ampEnvVal.
    // When the amp envelope finishes, ampEnvVal reaches 0 and the voice calls
    // clearCurrentNote() — no more audio contribution from that voice regardless
    // of where the filter envelope is in its cycle. The actual audible tail ends
    // exactly when ampRelease completes. Reporting filterEnvRelease to the host
    // would cause unnecessary silence to be recorded during offline bouncing
    // without any audio to capture for those extra seconds.
    //
    // Update this if voice lifetime is ever extended to outlast the amp envelope
    // (e.g., for a self-oscillating filter resonance tail in a future phase).
    auto* ampRel = apvts.getRawParameterValue ("ampRelease");
    return (ampRel != nullptr) ? static_cast<double> (ampRel->load()) : 10.0;

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
        + " samplesPerBlock=" + juce::String (samplesPerBlock)
        + " voices=" + juce::String (synth.getNumVoices())
        + " params: amp(4) osc1(4) filter(3) filterEnv(5) osc2+mix(5) lfo(6) unison(3) voicing(4)");
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

    // --- Update polyphony cap (Phase 6.8) ---
    // SolaceSynthesiser::noteOn() checks voiceLimit before admitting new notes.
    // When at the cap it lets JUCE's voice-stealing algorithm pick the oldest
    // voice to stop and hand to the new note -- steal mode, not drop mode.
    synth.setVoiceLimit (static_cast<int> (apvts.getRawParameterValue ("voiceCount")->load()));

    // --- Render all active voices ---
    // The Synthesiser scans the now-merged midiMessages buffer for events,
    // routes them to SolaceVoice instances, and renders audio for this block.
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // --- Apply master distortion (Phase 6.9) ---
    // Soft-clip the mixed audio before the volume stage. Distortion character
    // changes with signal level, so it must come before the volume scaling.
    // Process each channel independently (L and R get the same drive, but have
    // different sample values from stereo unison spread, so outputs differ).
    const float drive = apvts.getRawParameterValue ("masterDistortion")->load();
    if (drive > 0.0f)
    {
        const int numSamples  = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* channelData = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                channelData[i] = SolaceDistortion::processSample (channelData[i], drive);
        }
    }

    // --- Apply master volume from APVTS ---
    // Get the current (possibly automated) value and scale all channels.
    const float masterVolume = apvts.getRawParameterValue ("masterVolume")->load();

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
