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

    // --- Phase 6.4: Filter Envelope ---
    // filterEnvDepth: float, -1.0 to +1.0. Controls direction and intensity:
    //   +1.0 = full upward sweep (bright attack, dark sustain — classic pluck)
    //    0.0 = no envelope effect (filter static at filterCutoff)
    //   -1.0 = full downward sweep (dark attack, bright sustain — inverted)
    // filterEnvAttack/Decay/Sustain/Release: same units as ampEnvelope.
    const std::atomic<float>* filterEnvDepth   = nullptr;
    const std::atomic<float>* filterEnvAttack  = nullptr;
    const std::atomic<float>* filterEnvDecay   = nullptr;
    const std::atomic<float>* filterEnvSustain = nullptr;
    const std::atomic<float>* filterEnvRelease = nullptr;

    // --- Phase 6.5: Oscillator 2 + Osc Mix ---
    // Osc2 uses the same SolaceOscillator API as Osc1.
    // Int parameters are stored as float by APVTS; cast to int at note-on.
    // oscMix: float, 0.0–1.0. 0 = Osc1 only, 1 = Osc2 only, 0.5 = equal blend.
    // oscMix is snapshotted per render-block (not note-on) for live crossfader response.
    const std::atomic<float>* osc2Waveform  = nullptr;
    const std::atomic<float>* osc2Octave    = nullptr;
    const std::atomic<float>* osc2Transpose = nullptr;
    const std::atomic<float>* osc2Tuning    = nullptr;  // float, -100 to +100 cents
    const std::atomic<float>* oscMix        = nullptr;  // float, 0.0–1.0
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
//   Per-sample via 1-sample AudioBlock (processSample is protected in JUCE 8).
//   baseCutoffHz (knob) read per render-block; filter setCutoff() called per-sample
//   so the filter envelope can modulate it every sample (see Phase 6.4).
//
// Phase 6.4 — Filter Envelope:
//   filterEnvelope is a second SolaceADSR that runs independently of the amp
//   envelope. Each sample: modulatedCutoff = baseCutoffHz + envVal * depth * modRange.
//   modRange = 10000 Hz constant (V1). filterEnvDepth (-1 to +1) controls direction.
//   filter.setCutoff() clamps to [20, 20000] Hz — safe for any modulation value.
//   filterEnvelope.reset() called before trigger() at note-on to prevent ADSR
//   restart from non-zero level when voice is reused mid-release.
//
// Phase 6.5 — Second Oscillator + Osc Mix:
//   osc2 is a second SolaceOscillator with its own waveform and tuning stack,
//   configured at note-on from osc2Waveform/Octave/Transpose/Tuning APVTS params.
//   oscMix (0.0–1.0) crossfades between them per render-block:
//     oscSample = osc1.getNextSample() * (1.0f - oscMix) + osc2.getNextSample() * oscMix
//   The blended signal feeds into the filter unchanged — signal chain is additive.
//
// Signal flow (per sample, Phase 6.5+):
//   blendedOsc = osc1 * (1 - oscMix) + osc2 * oscMix          ← crossfader
//     → filter.setCutoff(baseCutoffHz + filterEnv * depth * 10000)  ← per sample
//     → filter.processSample(blendedOsc)                            ← filtered
//       → * kVoiceGain * velocityScale * ampEnvelope.getNextSample()
//         → output buffer
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

        // Phase 6.4
        jassert (params.filterEnvDepth   != nullptr);
        jassert (params.filterEnvAttack  != nullptr);
        jassert (params.filterEnvDecay   != nullptr);
        jassert (params.filterEnvSustain != nullptr);
        jassert (params.filterEnvRelease != nullptr);

        // Phase 6.5
        jassert (params.osc2Waveform  != nullptr);
        jassert (params.osc2Octave    != nullptr);
        jassert (params.osc2Transpose != nullptr);
        jassert (params.osc2Tuning    != nullptr);
        jassert (params.oscMix        != nullptr);
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
        ampEnvelope.prepare    (spec.sampleRate);
        filterEnvelope.prepare (spec.sampleRate);
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

        // --- Oscillator 2 (Phase 6.5) ---
        // Same setup pattern as Osc1. Both oscillators receive the same base MIDI
        // frequency — their individual tuning offsets give them independent pitches.
        // oscMix is NOT snapshotted here — it is read per render-block in
        // renderNextBlock() so the crossfader is live while notes are held.
        osc2.reset();  // phase-coherent start per note, same reasoning as osc1
        osc2.setWaveform  (static_cast<int> (params.osc2Waveform->load()));
        osc2.setTuningOffset (
            static_cast<int>  (params.osc2Octave->load()),
            static_cast<int>  (params.osc2Transpose->load()),
            params.osc2Tuning->load()
        );
        osc2.setFrequency (baseHz, getSampleRate());

        // --- Filter (Phase 6.3) ---
        // filterType is snapshotted at note-on — mode changes mid-note cause a
        // transient click (state-vector discontinuity). Cutoff and resonance are
        // read live per-block in renderNextBlock(), so knob moves are audible.
        filter.setMode      (static_cast<int> (params.filterType->load()));
        filter.setResonance (params.filterResonance->load());
        // baseCutoffHz is updated per-block in renderNextBlock(); initialise it
        // here so it holds a valid value on the very first block of this note.
        baseCutoffHz = params.filterCutoff->load();
        filter.setCutoff (baseCutoffHz);

        // --- Filter Envelope (Phase 6.4) ---
        // Snapshotted at note-on — ADSR shape is fixed for the note's lifetime.
        // filterEnvDepth is read per-block in renderNextBlock() (cheap, per-block
        // resolution is fine for a UI knob the user may sweep mid-note).
        //
        // reset() before trigger() is REQUIRED: the voice is freed when the amp
        // envelope finishes, which may happen before the filter envelope release
        // completes (if filterEnvRelease > ampRelease). If this voice is then
        // immediately reused, the filter envelope could still be mid-release.
        // JUCE's ADSR::noteOn() restarts from the current level (not from 0),
        // so without reset(), the new note's filter attack would start from a
        // non-zero level — an audible artifact on the pluck attack.
        filterEnvelope.reset();
        filterEnvelope.setParameters (
            params.filterEnvAttack->load(),
            params.filterEnvDecay->load(),
            params.filterEnvSustain->load(),
            params.filterEnvRelease->load()
        );
        filterEnvelope.trigger();

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
            // Soft note-off: both envelopes begin their release stage together.
            // The voice stays alive until ampEnvelope.isActive() returns false —
            // the filter envelope continues running naturally throughout the release.
            ampEnvelope.release();
            filterEnvelope.release();
        }
        else
        {
            // Hard cut (voice stolen). Reset all state so this voice can be
            // immediately reused without transient pops or stale envelope values.
            ampEnvelope.reset();
            filterEnvelope.reset();
            filter.reset();
            clearCurrentNote();
        }
    }

    // ========================================================================
    // renderNextBlock — fill the output buffer for this voice.
    //
    // Signal flow (Phase 6.5+, per sample):
    //   blendedOsc = osc1 * (1-mix) + osc2 * mix         ← crossfader (per block)
    //   blendedOsc → filter (cutoff modulated by filterEnv per sample)
    //     → * kVoiceGain * velocityScale * ampEnv → output
    //
    // Filter runs BEFORE amp envelope — correct subtractive order:
    //   filter self-resonance decays naturally during amp release phase.
    //
    // Per-block (before the sample loop):
    //   - baseCutoffHz, filterResonance, envDepth, oscMix read from APVTS atomics
    //
    // Per-sample (inside the loop):
    //   - osc1 + osc2 both advance always (phase-continuous crossfader sweep)
    //   - filterEnvelope.getNextSample() → modulated cutoff → filter.setCutoff()
    //   - ampEnvelope.getNextSample()    → final amplitude scale
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

        // --- Per-block parameter refresh ---
        // Read live APVTS values once per block (not per sample) to give
        // immediate knob response without atomic load overhead per sample.
        //
        // filterType excluded — mode changes mid-note cause a transient click
        // (state-vector discontinuity). Stays at note-on snapshot for V1.
        baseCutoffHz = params.filterCutoff->load();
        filter.setResonance (params.filterResonance->load());

        // filterEnvDepth: continuous float the user may sweep mid-note.
        const float envDepth = params.filterEnvDepth->load();

        // oscMix: read per-block so moving the crossfader while holding a note
        // is immediately audible. Clamped to [0, 1] for safety.
        const float mix = juce::jlimit (0.0f, 1.0f, params.oscMix->load());

        // modRange: maximum Hz the filter envelope can add or subtract.
        // 10000 Hz gives dramatic sweeps while keeping headroom on both edges.
        // V1 constant — could become an APVTS param in V2 if needed.
        constexpr float kModRange = 10000.0f;

        while (--numSamples >= 0)
        {
            // 1. Oscillators: blend Osc1 and Osc2 via oscMix crossfader.
            //    mix=0 → Osc1 only; mix=1 → Osc2 only; mix=0.5 → equal blend.
            //    Both oscillators always advance their phase accumulators each
            //    sample (getNextSample() is called regardless of mix value) so
            //    neither oscillator drifts out of phase if mix is swept back.
            const float osc1Sample  = osc1.getNextSample();
            const float osc2Sample  = osc2.getNextSample();
            const float oscSample   = osc1Sample * (1.0f - mix) + osc2Sample * mix;

            // 2. Filter envelope modulation — per sample for smooth sweep.
            //    filterEnvelope returns 0.0 → 1.0. envDepth (-1 to +1) controls
            //    direction. SolaceFilter::setCutoff() clamps to [20, 20000] Hz.
            const float filterEnvVal    = filterEnvelope.getNextSample();
            const float modulatedCutoff = baseCutoffHz + filterEnvVal * envDepth * kModRange;
            filter.setCutoff (modulatedCutoff);

            // 3. Filter: shape the spectrum (blended osc → filter → amp)
            const float filteredSample = filter.processSample (oscSample);

            // 4. Amp envelope + velocity + gain
            const float ampEnvVal     = ampEnvelope.getNextSample();
            const float currentSample = filteredSample
                                        * kVoiceGain
                                        * velocityScale
                                        * ampEnvVal;

            // 5. Mix into all output channels (mono voice → stereo buffer)
            for (int ch = outputBuffer.getNumChannels(); --ch >= 0;)
                outputBuffer.addSample (ch, startSample, currentSample);

            ++startSample;

            // 6. Voice done check — AFTER getNextSample() so the ADSR's
            //    final idle transition is captured correctly in the output.
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
    // baseCutoffHz: the APVTS filterCutoff value in Hz, read per render-block.
    // The filter envelope adds to this per-sample: baseCutoffHz + envVal * depth * modRange.
    float baseCutoffHz = 20000.0f;

    // --- Phase 6.4: Filter Envelope ---
    SolaceADSR filterEnvelope;

    // --- Phase 6.5: Oscillator 2 ---
    // oscMix is cached per render-block (not stored here as a member).
    // The per-block load from APVTS atomics gives live crossfader response.
    SolaceOscillator osc2;

    // Velocity scaling — set at note-on, multiplied per sample in renderNextBlock.
    float velocityScale = 0.0f;

    // Per-voice output gain. Limits peak output of a single voice to prevent
    // clipping when many voices are summed. 16-voice default.
    // TODO (Phase 6.7): Replace with dynamic normalisation for unison.
    static constexpr float kVoiceGain = 0.15f;
};
