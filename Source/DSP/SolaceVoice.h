#pragma once

#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "SolaceSound.h"
#include "SolaceADSR.h"
#include "SolaceOscillator.h"

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

    // --- Phase 6.2: Oscillator 1 Waveform + Tuning ---
    // osc1Waveform: int stored as float (Sine=0, Saw=1, Square=2, Triangle=3)
    // osc1Octave:   int stored as float (-3 to +3, default 0)
    // osc1Transpose: int stored as float (-12 to +12 semitones, default 0)
    // osc1Tuning:   float (-100.0 to +100.0 cents, default 0.0)
    const std::atomic<float>* osc1Waveform  = nullptr;
    const std::atomic<float>* osc1Octave    = nullptr;
    const std::atomic<float>* osc1Transpose = nullptr;
    const std::atomic<float>* osc1Tuning    = nullptr;
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
// Phase 6.2 — Waveforms + Osc1 Tuning:
//   - Replaces the inline std::sin() with a SolaceOscillator (osc1).
//   - Waveform (Sine/Saw/Square/Triangle) is read from APVTS at note-on.
//   - Octave, Transpose, and Tuning offsets are applied via setTuningOffset()
//     which computes a frequency multiplier using equal-temperament math.
//   - All four waveforms produce output in [-1.0, +1.0] — same as sine.
//   - Naïve waveforms (Saw, Square) alias at high frequencies (V1 acceptable).
//     V2 will replace with PolyBLEP or wavetable.
//
// Signal flow (per sample):
//   osc1.getNextSample() * kVoiceGain * velocityScale * ampEnvelope.getNextSample()
//
// Architecture rules (audio thread):
//   - No allocations, no locks, no logging, no I/O.
//   - addSample() (not setSample()) so voices mix correctly in polyphony.
//   - startSample offset MUST be respected — a note-on can arrive mid-block.
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
        // Phase 6.1 — Amp ADSR
        jassert (params.ampAttack  != nullptr);
        jassert (params.ampDecay   != nullptr);
        jassert (params.ampSustain != nullptr);
        jassert (params.ampRelease != nullptr);

        // Phase 6.2 — Osc1 waveform + tuning
        jassert (params.osc1Waveform  != nullptr);
        jassert (params.osc1Octave    != nullptr);
        jassert (params.osc1Transpose != nullptr);
        jassert (params.osc1Tuning    != nullptr);
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
    // Note: setCurrentPlaybackSampleRate() is NOT called here.
    // juce::Synthesiser::setCurrentPlaybackSampleRate() (called first in
    // PluginProcessor::prepareToPlay) already propagates the sample rate
    // to every voice it owns. Calling it again here would be redundant.
    // ========================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        ampEnvelope.prepare (spec.sampleRate);
        // SolaceOscillator does not need a separate prepare() — sample rate is
        // passed directly to setFrequency() at each note-on via getSampleRate().
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
    // Snapshots the current APVTS values (atomic loads), configures the
    // oscillator waveform + tuning, triggers the amp envelope, and stores
    // velocity for per-sample scaling.
    //
    // All heavy setup (pow() for tuning, etc.) is done here — NOT per sample.
    // ========================================================================
    void startNote (int midiNoteNumber,
                    float velocity,
                    juce::SynthesiserSound* /*sound*/,
                    int /*currentPitchWheelPosition*/) override
    {
        // Reset oscillator for a clean, phase-coherent attack.
        osc1.reset();

        // Store MIDI velocity (normalised to 0.0–1.0 by JUCE) for per-sample
        // amplitude scaling. Full velocity = loudest note.
        velocityScale = velocity;

        // --- Osc1 Waveform (Phase 6.2) ---
        // AudioParameterInt values are stored as float in APVTS atomics.
        // Casting to int gives the discrete index: 0=Sine, 1=Saw, 2=Square, 3=Triangle.
        osc1.setWaveform (static_cast<int> (params.osc1Waveform->load()));

        // --- Osc1 Tuning Offset (Phase 6.2) ---
        // Must be called BEFORE setFrequency() so the multiplier is current.
        //   octave    → integer, read as float and cast back (APVTS stores ints as float)
        //   transpose → same, semitone integers
        //   cents     → genuine float (-100.0 to +100.0)
        osc1.setTuningOffset (
            static_cast<int>   (params.osc1Octave->load()),
            static_cast<int>   (params.osc1Transpose->load()),
            params.osc1Tuning->load()
        );

        // --- Osc1 Frequency (Phase 6.2) ---
        // Convert MIDI note to Hz, then set the oscillator's phase increment.
        // The tuning multiplier (set above) is applied inside setFrequency().
        const double baseHz = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        osc1.setFrequency (baseHz, getSampleRate());

        // --- Amp Envelope (Phase 6.1) ---
        // Snapshot ADSR parameters from APVTS atomics. .load() is thread-safe.
        // Snapshotting at note-on means the envelope shape is fixed for the
        // duration of this note. Changing Attack mid-note does not affect
        // already-sounding notes — it affects the next note-on. Standard synth UX.
        ampEnvelope.setParameters (
            params.ampAttack->load(),
            params.ampDecay->load(),
            params.ampSustain->load(),
            params.ampRelease->load()
        );
        ampEnvelope.trigger();
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
        }
    }

    // ========================================================================
    // renderNextBlock — fill the output buffer for this voice.
    //
    // Called every audio block while the voice is active.
    //
    // Signal flow per sample:
    //   osc1.getNextSample()       → waveform in [-1, +1]
    //   * kVoiceGain               → headroom guard (16-voice polyphony)
    //   * velocityScale            → MIDI velocity sensitivity (0.0–1.0)
    //   * ampEnvelope.getNextSample() → ADSR shape (0.0–1.0)
    //
    // CRITICAL: addSample() not setSample() — voices mix into a shared buffer.
    // CRITICAL: startSample offset respected — note may begin mid-block.
    // CRITICAL: No allocations, no locks, no logging on this thread.
    // ========================================================================
    void renderNextBlock (juce::AudioSampleBuffer& outputBuffer,
                          int startSample,
                          int numSamples) override
    {
        // Guard: osc1 will return zeros if setFrequency() hasn't been called yet
        // (angleDelta == 0). The envelope also won't be active. Belt-and-suspenders:
        if (! ampEnvelope.isActive() && ! isVoiceActive())
            return;

        while (--numSamples >= 0)
        {
            // Advance the ADSR and get the current envelope level (0.0–1.0).
            float envValue = ampEnvelope.getNextSample();

            // Compute output sample:
            //   osc1 sample → waveform oscillator (-1.0 to +1.0)
            //   * kVoiceGain → headroom guard
            //   * velocityScale → MIDI velocity sensitivity
            //   * envValue → envelope shape
            float currentSample = osc1.getNextSample()
                                  * kVoiceGain
                                  * velocityScale
                                  * envValue;

            // Mix into all output channels (handles both mono and stereo).
            for (int ch = outputBuffer.getNumChannels(); --ch >= 0;)
                outputBuffer.addSample (ch, startSample, currentSample);

            ++startSample;

            // Check envelope state AFTER getNextSample() — the ADSR transitions
            // to idle on the sample that reaches zero, not the one before it.
            // When the release has fully completed, mark this voice as free.
            if (! ampEnvelope.isActive())
            {
                clearCurrentNote();
                break;
            }
        }
    }

    // ========================================================================
    // Pitch wheel / controller — required overrides, not yet implemented.
    // Phase 6.8+ / V2: pitch wheel will modulate osc1 frequency.
    // ========================================================================
    void pitchWheelMoved (int /*newValue*/)                    override {}
    void controllerMoved (int /*controller*/, int /*value*/)   override {}

private:
    // APVTS parameter pointers — copied at construction, read at note-on.
    // Lightweight struct (eight raw pointers), safe to copy by value.
    const SolaceVoiceParams params;

    // --- Phase 6.1: Amplifier envelope ---
    SolaceADSR ampEnvelope;

    // --- Phase 6.2: Oscillator 1 ---
    SolaceOscillator osc1;

    // Velocity scaling — set at note-on, multiplied per sample.
    float velocityScale = 0.0f;

    // Per-voice output gain. Limits the maximum output of a single voice so
    // that summing many simultaneous voices does not clip the output buffer.
    // Value chosen for 16-voice polyphony with moderate overlap in practice.
    // TODO (Phase 6.7): Replace with dynamic normalisation based on unison count.
    static constexpr float kVoiceGain = 0.15f;
};
