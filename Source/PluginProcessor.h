#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "SolaceLogger.h"
#include "DSP/SolaceVoice.h"
#include "DSP/SolaceSound.h"

// ============================================================================
// SolaceSynthesiser — juce::Synthesiser subclass with runtime polyphony cap.
//
// Override noteOn() to enforce a dynamic voice limit. When the number of
// currently active voices equals or exceeds voiceLimit, JUCE's normal note-on
// path is still called -- which triggers its voice-stealing algorithm so the
// oldest playing voice is stopped and re-used for the new note. When we are
// below the cap, the new note simply gets the next free voice.
//
// This is the JUCE-idiomatic approach: polyphony policy lives in the Synthesiser
// subclass, not in SolaceVoice::startNote(). The Synthesiser owns voice
// allocation; the voice only knows how to render one note.
//
// All 16 SolaceVoice instances are always created and pre-prepared. At runtime
// only voiceLimit voices are engaged for new notes; the rest stay silent.
//
// Thread safety: setVoiceLimit() is called from processBlock (audio thread).
// voiceLimit is read inside noteOn() which is also audio thread. No UI-thread
// access needed -- we read from an APVTS atomic, not from this field.
// ============================================================================
class SolaceSynthesiser : public juce::Synthesiser
{
public:
    // Called once per processBlock to sync the current APVTS voiceCount value.
    // Call BEFORE synth.renderNextBlock() to ensure it's current.
    void setVoiceLimit (int limit) noexcept
    {
        voiceLimit = juce::jlimit (1, 16, limit);
    }

    void noteOn (int midiChannel, int midiNoteNumber, float velocity) override
    {
        // Count voices currently producing audio (held + in release tail).
        int activeCount = 0;
        for (int i = 0; i < getNumVoices(); ++i)
            if (getVoice (i)->isVoiceActive())
                ++activeCount;

        if (activeCount >= voiceLimit)
        {
            // At the polyphony cap: call the base noteOn() so JUCE's voice-
            // stealing algorithm picks the oldest/lowest-priority voice to stop
            // and immediately assigns it to this new note. The caller hears the
            // new note start and an old note stop -- standard hardware behaviour.
            // (Contrast with silently dropping the note, which would feel broken.)
            juce::Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
            return;
        }

        // Below cap: normal path -- JUCE finds a free voice.
        juce::Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
    }

private:
    int voiceLimit = 16;  // updated each processBlock from APVTS voiceCount
};

// ============================================================================
// Solace Synth — Audio Processor (DSP Engine)
//
// This is the heart of the plugin. It handles:
// - Audio processing (oscillators, filters, envelopes, etc.)
// - MIDI input (via juce::Synthesiser — polyphonic note management)
// - Parameter management via AudioProcessorValueTreeState
// - State save/load for DAW recall
//
// Phase 5: juce::Synthesiser with SolaceVoice (sine wave) and SolaceSound
// processes MIDI in processBlock. MIDI note-on → note plays. Note-off → tail.
// ============================================================================

class SolaceSynthProcessor : public juce::AudioProcessor
{
public:
    SolaceSynthProcessor();
    ~SolaceSynthProcessor() override;

    // --- Audio Processing ---
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // --- Editor ---
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    // --- Plugin Info ---
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    // --- Programs (presets) ---
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    // --- State Persistence ---
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- Parameter Tree ---
    // All plugin parameters live here. This is the contract between DSP and UI.
    // The UI reads/writes parameters through this tree.
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // --- MIDI Keyboard State (for on-screen keyboard in the Editor) ---
    // Lives in the Processor so the audio thread can call
    // processNextMidiBuffer() safely. MidiKeyboardState is thread-safe.
    juce::MidiKeyboardState& getMidiKeyboardState() { return keyboardState; }

private:
    // Parameter tree — defines all automatable parameters
    juce::AudioProcessorValueTreeState apvts;

    // MIDI keyboard state — shared between Editor (UI thread) and processBlock (audio thread).
    // Thread-safe: MidiKeyboardState uses an internal lock for cross-thread access.
    juce::MidiKeyboardState keyboardState;

    // Polyphonic synthesiser engine — custom subclass adds runtime polyphony cap.
    // All 16 voices are always allocated. voiceLimit is updated each processBlock.
    SolaceSynthesiser synth;

    // File-based logger — output goes to %TEMP%/SolaceSynth/ (trace.log, debug.log, info.log)
    std::unique_ptr<SolaceLogger> solaceLogger;

    // Creates the parameter layout (called once in constructor)
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SolaceSynthProcessor)
};
