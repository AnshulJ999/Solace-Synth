#pragma once

#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "SolaceSound.h"
#include "SolaceADSR.h"
#include "SolaceOscillator.h"
#include "SolaceFilter.h"

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
    // Int parameters are stored as float by APVTS; cast to int at note-on.
    const std::atomic<float>* osc1Waveform  = nullptr;
    const std::atomic<float>* osc1Octave    = nullptr;
    const std::atomic<float>* osc1Transpose = nullptr;
    const std::atomic<float>* osc1Tuning    = nullptr;  // float, -100 to +100 cents

    // --- Phase 6.3: Filter ---
    // filterCutoff:    float, 20–20000 Hz (skew 0.3, logarithmic feel)
    // filterResonance: float, 0.0–1.0
    // filterType:      int stored as float (0=LP12, 1=LP24, 2=HP12)
    const std::atomic<float>* filterCutoff    = nullptr;
    const std::atomic<float>* filterResonance = nullptr;
    const std::atomic<float>* filterType      = nullptr;
};

// ============================================================================
// SolaceVoice — One Polyphonic Synthesiser Voice
//
// A SynthesiserVoice renders one note at a time. The juce::Synthesiser
// manages a pool of these voices and assigns them to incoming MIDI note-on
// events.
//
// Phase 6.1 — Amplifier ADSR Envelope:
//   Replaces the manual exponential tail-off from Phase 5. ADSR parameters
//   are snapshotted from APVTS atomics at each note-on.
//
// Phase 6.2 — Waveforms + Osc1 Tuning:
//   SolaceOscillator replaces the inline std::sin(). Four waveforms
//   (Sine/Saw/Square/Triangle). Tuning offset applied via equal-temperament
//   multiplier (2^oct × 2^(semi/12) × 2^(cents/1200)) at note-on.
//
// Phase 6.3 — Filter (LadderFilter LP/HP):
//   SolaceFilter wraps juce::dsp::LadderFilter<float> (Moog-style ladder).
//   Processes one sample at a time via processSample() — per-sample cutoff
//   modulation is ready for Phase 6.4 (filter envelope) without refactoring.
//   baseCutoffHz is stored so Phase 6.4 can compute modulatedCutoff in-loop.
//
// Signal flow (per sample):
//   osc1.getNextSample()
//     → filter.processSample()     ← filter applied BEFORE amp
//       → * kVoiceGain
//         → * velocityScale
//           → * ampEnvelope.getNextSample()
//             → output buffer
//
// Architecture rules (audio thread):
//   - No allocations, no locks, no logging, no I/O.
//   - addSample() (not setSample()) — voices mix into a shared buffer.
//   - startSample offset MUST be respected — note-on can arrive mid-block.
// ============================================================================

class SolaceVoice : public juce::SynthesiserVoice
{
public:
    // ========================================================================
    // Constructor — stores APVTS parameter pointers. No audio yet.
    // All pointers in voiceParams must be non-null (jassert in debug builds).
    // ========================================================================
    explicit SolaceVoice (const SolaceVoiceParams& voiceParams)
        : params (voiceParams)
    {
        // Phase 6.1
        jassert (params.ampAttack  != nullptr);
        jassert (params.ampDecay   != nullptr);
        jassert (params.ampSustain != nullptr);
        jassert (params.ampRelease != nullptr);

        // Phase 6.2
        jassert (params.osc1Waveform  != nullptr);
        jassert (params.osc1Octave    != nullptr);
        jassert (params.osc1Transpose != nullptr);
        jassert (params.osc1Tuning    != nullptr);

        // Phase 6.3
        jassert (params.filterCutoff    != nullptr);
        jassert (params.filterResonance != nullptr);
        jassert (params.filterType      != nullptr);
    }

    // ========================================================================
    // prepare — initialise sample-rate-dependent DSP modules.
    //
    // Called from PluginProcessor::prepareToPlay() for every voice using:
    //   for (int i = 0; i < synth.getNumVoices(); ++i)
    //       if (auto* v = dynamic_cast<SolaceVoice*>(synth.getVoice(i)))
    //           v->prepare(spec);
    //
    // Note: setCurrentPlaybackSampleRate() is NOT called here.
    // juce::Synthesiser propagates sample rate to all voices automatically
    // via its own setCurrentPlaybackSampleRate() call — no need to repeat.
    // ========================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        ampEnvelope.prepare (spec.sampleRate);
        filter.prepare (spec);  // SolaceFilter overrides numChannels to 1 internally
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
    // All note-start setup happens here (not per-sample). Expensive operations
    // (pow(), atomic loads, filter mode changes) are all done once at note-on.
    // ========================================================================
    void startNote (int midiNoteNumber,
                    float velocity,
                    juce::SynthesiserSound* /*sound*/,
                    int /*currentPitchWheelPosition*/) override
    {
        // --- Oscillator 1 (Phase 6.2) ---
        osc1.reset();  // clear phase for click-free attack
        osc1.setWaveform  (static_cast<int> (params.osc1Waveform->load()));
        osc1.setTuningOffset (
            static_cast<int>  (params.osc1Octave->load()),
            static_cast<int>  (params.osc1Transpose->load()),
            params.osc1Tuning->load()
        );
        const double baseHz = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        osc1.setFrequency (baseHz, getSampleRate());

        // --- Filter (Phase 6.3) ---
        // Snapshot filter params from APVTS. Mode and resonance are static for
        // the note's lifetime (changed by setMode/setResonance below).
        // baseCutoffHz is stored as a member so Phase 6.4 can compute modulated
        // cutoff relative to it each sample inside renderNextBlock().
        baseCutoffHz = params.filterCutoff->load();
        filter.setMode      (static_cast<int> (params.filterType->load()));
        filter.setResonance (params.filterResonance->load());
        filter.setCutoff    (baseCutoffHz);  // static cutoff for Phase 6.3

        // --- Amplitude Envelope (Phase 6.1) ---
        // Snapshotting at note-on means envelope shape is fixed for this note.
        // Changes to ADSR knobs affect the next note-on, not the current note.
        velocityScale = velocity;
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
    // allowTailOff = true  → start ADSR release; voice stays alive until done.
    // allowTailOff = false → hard cut (voice stolen); silence + reset immediately.
    // ========================================================================
    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            ampEnvelope.release();
        }
        else
        {
            // Hard cut. Reset filter state to prevent transient pop when this
            // voice is immediately reused for a new note.
            ampEnvelope.reset();
            filter.reset();
            clearCurrentNote();
        }
    }

    // ========================================================================
    // renderNextBlock — fill the output buffer for this voice.
    //
    // Signal flow per sample:
    //   osc1 → filter → kVoiceGain × velocityScale × ampEnv → output
    //
    // Filter runs BEFORE amp envelope. This is the correct subtractive synth
    // order: oscillator generates the raw waveform, filter removes harmonics,
    // amp envelope shapes the final volume. Running amp after filter also means
    // the filter continues to self-resonate during the release phase correctly.
    //
    // Phase 6.4 integration point: to add filter envelope modulation, insert
    // a setCutoff() call inside the sample loop using the envelope value:
    //   float modCutoff = juce::jlimit(20f, 20000f, baseCutoffHz + envVal * depth * range);
    //   filter.setCutoff(modCutoff);
    // before the filter.processSample() call. SolaceFilter is ready for this.
    //
    // CRITICAL: addSample() not setSample() — voices mix into shared buffer.
    // CRITICAL: startSample offset respected — note may begin mid-block.
    // CRITICAL: No allocations, no locks, no logging on this thread.
    // ========================================================================
    void renderNextBlock (juce::AudioSampleBuffer& outputBuffer,
                          int startSample,
                          int numSamples) override
    {
        if (! ampEnvelope.isActive() && ! isVoiceActive())
            return;

        while (--numSamples >= 0)
        {
            // 1. Oscillator: raw waveform sample in [-1, +1]
            float oscSample = osc1.getNextSample();

            // 2. Filter: shape the spectrum (osc → filter → amp is correct order)
            //    Phase 6.4: add filter.setCutoff(modulatedCutoff) here before this line.
            float filteredSample = filter.processSample (oscSample);

            // 3. Amp envelope + velocity + gain
            float envValue      = ampEnvelope.getNextSample();
            float currentSample = filteredSample
                                  * kVoiceGain
                                  * velocityScale
                                  * envValue;

            // 4. Mix into all output channels (mono voice → stereo buffer)
            for (int ch = outputBuffer.getNumChannels(); --ch >= 0;)
                outputBuffer.addSample (ch, startSample, currentSample);

            ++startSample;

            // 5. Voice done check — AFTER getNextSample() so the ADSR's
            //    final idle transition is included in the output correctly.
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
    const SolaceVoiceParams params;

    // --- Phase 6.1: Amplitude envelope ---
    SolaceADSR ampEnvelope;

    // --- Phase 6.2: Oscillator 1 ---
    SolaceOscillator osc1;

    // --- Phase 6.3: Filter ---
    SolaceFilter filter;
    // Stored at note-on so Phase 6.4 can compute modulated cutoff per-sample:
    //   modulatedCutoff = baseCutoffHz + filterEnv.getNextSample() * depth * range
    float baseCutoffHz = 20000.0f;

    // Velocity scaling — set at note-on, multiplied per sample in renderNextBlock.
    float velocityScale = 0.0f;

    // Per-voice output gain. Limits peak output of a single voice to prevent
    // clipping when many voices are summed. 16-voice default.
    // TODO (Phase 6.7): Replace with dynamic normalisation for unison.
    static constexpr float kVoiceGain = 0.15f;
};
