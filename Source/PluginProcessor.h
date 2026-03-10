#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "SolaceLogger.h"
#include "DSP/SolaceVoice.h"
#include "DSP/SolaceSound.h"
#include "DSP/SolaceDistortion.h"

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

protected:
    // ========================================================================
    // findFreeVoice — restricts the free-voice search to [0, voiceLimit).
    //
    // This is the key JUCE extension point for polyphony capping. The base
    // Synthesiser::noteOn() calls findFreeVoice() to pick the next voice.
    // By restricting the scan to the first voiceLimit voices, inactive voices
    // beyond the cap are never touched — voices [voiceLimit, 15] stay silent
    // regardless of how many notes are playing.
    //
    // When all voiceLimit voices are active and stealIfNoneAvailable is true,
    // we delegate to findVoiceToSteal() (also capped) to pick a victim.
    // ========================================================================
    juce::SynthesiserVoice* findFreeVoice (juce::SynthesiserSound* soundToPlay,
                                           int midiChannel,
                                           int midiNoteNumber,
                                           bool stealIfNoneAvailable) const override
    {
        const int limit = voiceLimit;

        // Scan only within [0, limit) for a free (non-active) voice.
        for (int i = 0; i < limit; ++i)
        {
            auto* v = getVoice (i);
            if (v != nullptr && !v->isVoiceActive())
                return v;
        }

        // No free voice within the cap. Steal one if stealing is enabled.
        if (stealIfNoneAvailable)
            return findVoiceToSteal (soundToPlay, midiChannel, midiNoteNumber);

        return nullptr;
    }

    // ========================================================================
    // findVoiceToSteal — steals from within [0, voiceLimit) only.
    //
    // Prefers releasing voices (fading out, least audible to steal), then
    // falls back to the oldest held voice. This is a simplified version of
    // JUCE's default algorithm, restricted to the capped voice pool.
    // ========================================================================
    juce::SynthesiserVoice* findVoiceToSteal (juce::SynthesiserSound* /*soundToPlay*/,
                                               int /*midiChannel*/,
                                               int /*midiNoteNumber*/) const override
    {
        const int limit = voiceLimit;

        juce::SynthesiserVoice* oldestHeld     = nullptr;
        juce::SynthesiserVoice* oldestReleased = nullptr;

        for (int i = 0; i < limit; ++i)
        {
            auto* v = getVoice (i);
            if (v == nullptr || !v->isVoiceActive())
                continue;

            if (v->isPlayingButReleased())
            {
                // Prefer stealing releasing voices -- they are already fading.
                if (oldestReleased == nullptr || v->wasStartedBefore (*oldestReleased))
                    oldestReleased = v;
            }
            else
            {
                if (oldestHeld == nullptr || v->wasStartedBefore (*oldestHeld))
                    oldestHeld = v;
            }
        }

        // Releasing voice first (least audible). Fall back to oldest held.
        return oldestReleased != nullptr ? oldestReleased : oldestHeld;
    }

private:
    int voiceLimit = 16;  // updated each processBlock via setVoiceLimit()
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
