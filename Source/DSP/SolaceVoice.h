#pragma once

#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "SolaceSound.h"
#include "SolaceADSR.h"

// ============================================================================
// SolaceVoiceParams — APVTS Parameter Pointers for SolaceVoice
//
// Holds std::atomic<float>* pointers to every APVTS parameter that
// SolaceVoice needs to read at note-on time. Populated once in
// PluginProcessor's constructor after APVTS is fully initialised, then
// passed by value to each SolaceVoice at construction.
//
// Thread safety:
//   Reading a std::atomic<float> with .load() is safe from any thread.
//   APVTS owns the atomic objects for the entire plugin lifetime, so the
//   pointers remain valid until the processor is destroyed.
//
// Extending (future phases):
//   Add new pointer members here, populate them in PluginProcessor's
//   constructor, and read them in SolaceVoice::startNote().
// ============================================================================

struct SolaceVoiceParams
{
    // --- Phase 6.1: Amplifier Envelope ---
    const std::atomic<float>* ampAttack  = nullptr;
    const std::atomic<float>* ampDecay   = nullptr;
    const std::atomic<float>* ampSustain = nullptr;
    const std::atomic<float>* ampRelease = nullptr;
};

// ============================================================================
// SolaceVoice — One Polyphonic Synthesiser Voice
//
// A SynthesiserVoice renders one note at a time. The juce::Synthesiser
// manages a pool of these voices and assigns them to incoming MIDI note-on
// events.
//
// Phase 6.1 — Amplifier ADSR Envelope:
//   - Replaces the manual exponential tail-off from Phase 5.
//   - ADSR parameters are read from APVTS atomics at each note-on (snapshot).
//   - Velocity is stored as a 0.0–1.0 float and multiplied per-sample.
//   - prepare() initialises sample-rate-dependent DSP state; called from
//     PluginProcessor::prepareToPlay() via the established JUCE voice-iterate
//     pattern: synth.getVoice(i) + dynamic_cast<SolaceVoice*>.
//
// Signal flow (per sample):
//   sin(angle) * kVoiceGain * velocityScale * ampEnvelope.getNextSample()
//
// Architecture rules (audio thread):
//   - No allocations, no locks, no logging, no I/O.
//   - addSample() (not setSample()) so voices mix correctly in polyphony.
//   - startSample offset MUST be respected — a note-on can arrive mid-block.
//   - angleDelta == 0.0 is the "not yet started" guard.
// ============================================================================

class SolaceVoice : public juce::SynthesiserVoice
{
public:
    // ========================================================================
    // Constructor — stores APVTS parameter pointers. No audio yet.
    // All pointers in voiceParams must be non-null (asserted in debug builds).
    // ========================================================================
    explicit SolaceVoice (const SolaceVoiceParams& voiceParams)
        : params (voiceParams)
    {
        jassert (params.ampAttack  != nullptr);
        jassert (params.ampDecay   != nullptr);
        jassert (params.ampSustain != nullptr);
        jassert (params.ampRelease != nullptr);
    }

    // ========================================================================
    // prepare — initialise sample-rate-dependent DSP modules.
    //
    // Called from PluginProcessor::prepareToPlay() for every voice in the
    // pool using the established pattern:
    //
    //   for (int i = 0; i < synth.getNumVoices(); ++i)
    //       if (auto* v = dynamic_cast<SolaceVoice*>(synth.getVoice(i)))
    //           v->prepare(spec);
    //
    // setCurrentPlaybackSampleRate() makes getSampleRate() valid for the
    // lifetime of this prepare/release cycle.
    // ========================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        setCurrentPlaybackSampleRate (spec.sampleRate);
        ampEnvelope.prepare (spec.sampleRate);
    }

    // ========================================================================
    // canPlaySound — tell the Synthesiser which sound types this voice handles.
    // ========================================================================
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SolaceSound*> (sound) != nullptr;
    }

    // ========================================================================
    // startNote — called by the Synthesiser on each MIDI note-on.
    //
    // Snapshots the current APVTS ADSR values (atomic loads), configures and
    // triggers the envelope, stores velocity, and computes the oscillator
    // phase increment for this note's frequency.
    // ========================================================================
    void startNote (int midiNoteNumber,
                    float velocity,
                    juce::SynthesiserSound* /*sound*/,
                    int /*currentPitchWheelPosition*/) override
    {
        // Reset oscillator phase for a clean, click-free attack.
        currentAngle = 0.0;

        // Store MIDI velocity (already normalised to 0.0–1.0 by JUCE) for
        // per-sample amplitude scaling. Full velocity = loudest note.
        velocityScale = velocity;

        // Snapshot ADSR parameters from APVTS atomics.
        // .load() is thread-safe. Snapshotting at note-on means the envelope
        // shape is fixed for the duration of this note, which is correct
        // behaviour: changing Attack mid-note does not affect already-sounding
        // notes (it affects the next note-on). This matches standard synth UX.
        ampEnvelope.setParameters (
            params.ampAttack->load(),
            params.ampDecay->load(),
            params.ampSustain->load(),
            params.ampRelease->load()
        );
        ampEnvelope.trigger();

        // Convert MIDI note number to frequency and then to a per-sample
        // phase increment (angleDelta) using equal temperament.
        // A4 = MIDI 69 = 440 Hz. Each semitone = 2^(1/12).
        double frequencyHz     = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        double cyclesPerSample = frequencyHz / getSampleRate();
        angleDelta = cyclesPerSample * juce::MathConstants<double>::twoPi;
    }

    // ========================================================================
    // stopNote — called by the Synthesiser on MIDI note-off or voice stealing.
    //
    // allowTailOff = true  → start ADSR release phase; voice stays alive
    //                         until the release completes.
    // allowTailOff = false → hard cut (voice stolen); silence immediately.
    // ========================================================================
    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            // Begin ADSR release. renderNextBlock() will continue running
            // until ampEnvelope.isActive() returns false, then clear the voice.
            ampEnvelope.release();
        }
        else
        {
            // Immediate cut — voice is being stolen or All-Notes-Off received.
            // Reset the envelope to idle and mark this voice as free.
            ampEnvelope.reset();
            clearCurrentNote();
            angleDelta = 0.0;
        }
    }

    // ========================================================================
    // renderNextBlock — fill the output buffer for this voice.
    //
    // Called every audio block while the voice is active. Renders the sine
    // oscillator shaped by the ADSR envelope and velocity.
    //
    // CRITICAL: addSample() not setSample() — voices mix into a shared buffer.
    // CRITICAL: startSample offset respected — note may begin mid-block.
    // CRITICAL: No allocations, no locks, no logging on this thread.
    // ========================================================================
    void renderNextBlock (juce::AudioSampleBuffer& outputBuffer,
                          int startSample,
                          int numSamples) override
    {
        // Guard: angleDelta is 0.0 until startNote() has been called.
        if (angleDelta == 0.0)
            return;

        while (--numSamples >= 0)
        {
            // Advance the ADSR and get the current envelope level (0.0–1.0).
            float envValue = ampEnvelope.getNextSample();

            // Compute output sample:
            //   sin(angle)    → oscillator (-1.0 to +1.0)
            //   * kVoiceGain  → headroom guard (prevents clipping with many voices)
            //   * velocityScale → MIDI velocity sensitivity
            //   * envValue    → envelope shape (attack / decay / sustain / release)
            auto currentSample = static_cast<float> (std::sin (currentAngle))
                                 * kVoiceGain
                                 * velocityScale
                                 * envValue;

            // Mix into all output channels (mono and stereo both handled).
            for (int ch = outputBuffer.getNumChannels(); --ch >= 0;)
                outputBuffer.addSample (ch, startSample, currentSample);

            // Advance oscillator phase.
            currentAngle += angleDelta;
            ++startSample;

            // Check envelope state AFTER getNextSample() — the ADSR transitions
            // to idle on the sample that reaches zero, not the one before it.
            // When the release has fully completed, mark this voice as free.
            if (! ampEnvelope.isActive())
            {
                clearCurrentNote();
                angleDelta = 0.0;
                break;
            }
        }
    }

    // ========================================================================
    // Pitch wheel / controller — required overrides, not yet implemented.
    // ========================================================================
    void pitchWheelMoved (int /*newValue*/)                    override {}
    void controllerMoved (int /*controller*/, int /*value*/)   override {}

private:
    // APVTS parameter pointers — copied at construction, read at note-on.
    // Lightweight struct (four raw pointers), safe to copy by value.
    const SolaceVoiceParams params;

    // Amplifier envelope — shapes each note's attack, decay, sustain, release.
    SolaceADSR ampEnvelope;

    // Oscillator phase state.
    double currentAngle = 0.0;  // Current phase in radians (accumulates per sample)
    double angleDelta   = 0.0;  // Phase increment per sample. 0.0 = not yet started.

    // Velocity scaling — set at note-on, multiplied per sample.
    float velocityScale = 0.0f;

    // Per-voice output gain. Limits the maximum output of a single voice so
    // that summing many simultaneous voices does not clip the output buffer.
    // Value chosen for 16-voice polyphony with moderate overlap in practice.
    // TODO (Phase 6.7): Replace with dynamic normalisation based on unison count.
    static constexpr float kVoiceGain = 0.15f;
};
