#pragma once

#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "SolaceSound.h"
#include "SolaceADSR.h"
#include "SolaceOscillator.h"
#include "SolaceFilter.h"
#include "SolaceLFO.h"
#include "SolaceDistortion.h"  // Phase 6.8b: per-voice velocity-to-distortion target

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
    const std::atomic<float>* osc2Waveform  = nullptr;
    const std::atomic<float>* osc2Octave    = nullptr;
    const std::atomic<float>* osc2Transpose = nullptr;
    const std::atomic<float>* osc2Tuning    = nullptr;
    const std::atomic<float>* oscMix        = nullptr;

    // --- Phase 6.6: LFO ---
    const std::atomic<float>* lfoWaveform = nullptr;
    const std::atomic<float>* lfoRate     = nullptr;
    const std::atomic<float>* lfoAmount   = nullptr;
    const std::atomic<float>* lfoTarget1  = nullptr;
    const std::atomic<float>* lfoTarget2  = nullptr;
    const std::atomic<float>* lfoTarget3  = nullptr;

    // --- Phase 6.7: Unison ---
    // unisonCount: int stored as float (1-8). Default 1 = no unison (no change to sound).
    //   Snapshotted at note-on -- changing count mid-note would require resizing the active
    //   oscillator array, which is not safe. Users hear the new value on the next note.
    // unisonDetune: float, 0-100 cents. Spread of detuning from -detune/2 to +detune/2
    //   across all N voices. Voice 0 = -detune/2, voice N-1 = +detune/2, centre voice = 0.
    //   Snapshotted at note-on for the same reason as unisonCount.
    // unisonSpread: float, 0.0-1.0. Controls the stereo width of the panned unison voices.
    //   0.0 = all voices centre (mono). 1.0 = outer voices hard left/right.
    //   Read per-block for live knob response (only affects pan gains, not oscillator state).
    const std::atomic<float>* unisonCount  = nullptr;  // int stored as float
    const std::atomic<float>* unisonDetune = nullptr;  // float, 0-100 cents
    const std::atomic<float>* unisonSpread = nullptr;  // float, 0.0-1.0

    // --- Phase 6.8 / 6.8b: Voicing Parameters ---
    // velocityRange: float, 0.0-1.0. How strongly velocity modulates the targets.
    //   0.0 = flat (all notes same level/attack). 1.0 = full velocity sensitivity.
    //   Read at note-on and applied immediately per target.
    // velocityModTarget1/2/3: int 0-7. Three velocity-to-parameter routing slots.
    //   0=None, 1=AmpLevel, 2=AmpAttack, 3=FilterCutoff, 4=FilterResonance,
    //   5=Distortion, 6=Osc1Pitch, 7=OscMix.
    //   Additive modulations on top of the knob values, same pattern as LFO targets.
    //   If AmpLevel is in no slot, velocityScale is 1.0 (flat -- every note same volume).
    // voiceCount: int 1-16 stored as float. Read only by SolaceSynthesiser in
    //   PluginProcessor.h -- NOT read inside SolaceVoice. Kept here for
    //   symmetry and to keep all APVTS pointers in one struct.
    const std::atomic<float>* velocityRange      = nullptr;
    const std::atomic<float>* velocityModTarget1 = nullptr;
    const std::atomic<float>* velocityModTarget2 = nullptr;
    const std::atomic<float>* velocityModTarget3 = nullptr;  // Phase 6.8b
    const std::atomic<float>* voiceCount         = nullptr;

    // --- Phase 7 DSP: Mod Wheel ---
    // Owned by SolaceSynthesiser (atomic member). Pointer held here so voices
    // can read modWheelValue per-block to scale LFO amount.
    //
    // Note: pitch wheel position at note-on is passed directly as the
    // currentPitchWheelPosition parameter in startNote() by JUCE's Synthesiser
    // (it tracks lastPitchWheelValues[channel] internally). No need for a
    // separate pointer here -- JUCE already handles it correctly per channel.
    //
    // modWheelValue: 0-127. Updated via handleMidiEvent for CC#1.
    const std::atomic<int>* modWheelValue = nullptr;
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
// Phase 6.7 -- Unison:
//   Each SolaceVoice now owns an array of UnisonVoice structs (max 8). Each
//   UnisonVoice contains its own osc1+osc2 pair, independently detuned from
//   the MIDI note frequency. At unisonCount=1 (default), behaviour is identical
//   to Phase 6.6 -- no audible change.
//
//   Phase 6.7 stereo architecture: each unison voice's oscillator mix is
//   accumulated into preFiltL and preFiltR separately using its individual
//   constant-power pan gains BEFORE the filter stage. filterL and filterR
//   each receive a different blend of detuned oscillators, producing
//   genuinely different spectral content per channel. At N=1 spread=0,
//   preFiltL == preFiltR and the output is centred mono (backward-compatible).
//   CPU cost: 2 filter instances per polyphonic voice (1 per channel).
//
//   Detune distribution (symmetric around MIDI note pitch):
//     N=1 → 0 cents (no detune).
//     N>1 → voice i gets: ((i / (N-1)) - 0.5) * unisonDetune cents
//           → voice 0 at -detune/2, voice N-1 at +detune/2, centre voice = 0.
//
//   Equal-power pan law (constant-power stereo spread):
//     pan_i = ((i / (N-1)) * 2 - 1) * unisonSpread  (range -1 to +1)
//     leftGain  = sqrt(0.5 * (1 - pan_i))
//     rightGain = sqrt(0.5 * (1 + pan_i))
//   Recomputed per block from unisonSpread for live knob response.
//
//   Level normalisation (mandatory):
//     voiceGain = kBaseVoiceGain / sqrt(unisonCount)
//     Prevents loudness jump when unison is increased (equal-power sum).
//
// Signal flow (per sample, Phase 6.7+ / 6.8 velocity mods):
//   [Per block] unisonSpread read -> per-voice panL/panR recomputed
//   [Per block] lfoWaveform/rate/amount/targets read; osc pitch LFO mult set
//   [Per block] baseCutoffHz, baseResonance, envDepth, oscMix read
//
//   [Per sample, inner loop over activeUnisonCount voices]:
//     osc1/osc2 advance -> apply osc-level LFO if targeted
//     uMixed = blend(osc1s, osc2s, oscMix)
//     preFiltL += uMixed * panL[u]    -- pre-filter stereo accumulation
//     preFiltR += uMixed * panR[u]
//
//   [Per sample, after all unison voices]:
//     filterEnvVal = filterEnvelope.getNextSample()
//     modulatedCutoff = baseCutoffHz + filterEnvVal*envDepth*kModRange
//                     + velModCutoffHz + [lfoSample*kLFOCutoffRange if target]
//     modulatedRes    = jlimit(0,1, baseRes + velModRes + [lfoSample*kLFOResRange if target])
//     filterL.setCutoff/setResonance(modulatedCutoff, modulatedRes)
//     filterR.setCutoff/setResonance(modulatedCutoff, modulatedRes)
//     filteredL = filterL.processSample(preFiltL)
//     filteredR = filterR.processSample(preFiltR)
//     ampEnvVal = ampEnvelope.getNextSample()
//     gainScalar = voiceGain * velocityScale * ampEnvVal * ampMod
//     sampleL = filteredL * gainScalar  -> outputBuffer channel 0
//     sampleR = filteredR * gainScalar  -> outputBuffer channel 1
//     (mono host: 0.5*(sampleL+sampleR) -> channel 0)
//
// Architecture rules (audio thread):
//   - No allocations, no locks, no logging, no I/O.
//   - addSample() (not setSample()) -- voices mix into a shared buffer.
//   - startSample offset MUST be respected -- note-on can arrive mid-block.
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

        // Phase 6.6
        jassert (params.lfoWaveform != nullptr);
        jassert (params.lfoRate     != nullptr);
        jassert (params.lfoAmount   != nullptr);
        jassert (params.lfoTarget1  != nullptr);
        jassert (params.lfoTarget2  != nullptr);
        jassert (params.lfoTarget3  != nullptr);

        // Phase 6.7
        jassert (params.unisonCount  != nullptr);
        jassert (params.unisonDetune != nullptr);
        jassert (params.unisonSpread != nullptr);

        // Phase 6.8
        jassert (params.velocityRange      != nullptr);
        jassert (params.velocityModTarget1 != nullptr);
        jassert (params.velocityModTarget2 != nullptr);
        jassert (params.velocityModTarget3 != nullptr);  // Phase 6.8b: third slot
        // params.voiceCount is NOT required by SolaceVoice -- it is read directly
        // from APVTS in SolaceSynthesiser::processBlock(). The pointer must still
        // be non-null (hence the jassert) but SolaceVoice never dereferences it.
        jassert (params.voiceCount != nullptr);

        // Phase 7 DSP: mod wheel (pitch wheel uses JUCE's built-in per-channel tracking)
        jassert (params.modWheelValue != nullptr);
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
        filterL.prepare (spec);  // One per channel for true per-channel stereo filtering
        filterR.prepare (spec);
        // LFO has no sample-rate-dependent state -- setRate() handles this per block.
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
                    int currentPitchWheelPosition) override
    {
        const double baseHz = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

        // --- Unison setup (Phase 6.7) ---
        // unisonCount and unisonDetune are snapshotted at note-on.
        // Changing either mid-note (mid-block) would require re-initialising
        // the oscillator array, which is not safe on the audio thread.
        activeUnisonCount = juce::jlimit (1, kMaxUnison,
            static_cast<int> (params.unisonCount->load()));
        const float detuneCents = params.unisonDetune->load();

        // Derive waveforms and tuning offsets from APVTS (same as pre-unison).
        const int   osc1Wave    = static_cast<int> (params.osc1Waveform->load());
        const int   osc1Oct     = static_cast<int> (params.osc1Octave->load());
        const int   osc1Trans   = static_cast<int> (params.osc1Transpose->load());
        const float osc1Cents   = params.osc1Tuning->load();
        const int   osc2Wave    = static_cast<int> (params.osc2Waveform->load());
        const int   osc2Oct     = static_cast<int> (params.osc2Octave->load());
        const int   osc2Trans   = static_cast<int> (params.osc2Transpose->load());
        const float osc2Cents   = params.osc2Tuning->load();

        for (int u = 0; u < activeUnisonCount; ++u)
        {
            // Detune offset for this unison voice:
            //   N=1 → 0 cents; N>1 → symmetric spread from -detune/2 to +detune/2.
            const float detuneOffset = (activeUnisonCount > 1)
                ? ((float)u / (float)(activeUnisonCount - 1) - 0.5f) * detuneCents
                : 0.0f;

            // Osc1 setup: waveform + (MIDI tuning stack + this voice's detune).
            unisonVoices[u].osc1.reset();
            unisonVoices[u].osc1.setWaveform     (osc1Wave);
            unisonVoices[u].osc1.setTuningOffset (osc1Oct, osc1Trans, osc1Cents + detuneOffset);
            unisonVoices[u].osc1.setFrequency    (baseHz, getSampleRate());

            // Osc2 setup: same detune offset applied on top of osc2's own tuning.
            unisonVoices[u].osc2.reset();
            unisonVoices[u].osc2.setWaveform     (osc2Wave);
            unisonVoices[u].osc2.setTuningOffset (osc2Oct, osc2Trans, osc2Cents + detuneOffset);
            unisonVoices[u].osc2.setFrequency    (baseHz, getSampleRate());
        }

        // Equal-power level normalisation: prevents loudness jump as unison increases.
        // At unisonCount=1 this reduces to kBaseVoiceGain / sqrt(1) = kBaseVoiceGain.
        voiceGain = kBaseVoiceGain / std::sqrt (static_cast<float> (activeUnisonCount));

        // --- Filter (Phase 6.3) ---
        // Both filter instances reset and initialised identically at note-on.
        // They diverge per sample only because preFiltL and preFiltR carry
        // different detuned oscillator mixes based on each unison voice's pan position.
        const int filterMode = static_cast<int> (params.filterType->load());
        const float filterRes = params.filterResonance->load();
        baseCutoffHz = params.filterCutoff->load();
        filterL.reset();        filterR.reset();
        filterL.setMode (filterMode);   filterR.setMode (filterMode);
        filterL.setResonance (filterRes); filterR.setResonance (filterRes);
        filterL.setCutoff (baseCutoffHz); filterR.setCutoff (baseCutoffHz);

        // --- Filter Envelope (Phase 6.4) ---
        filterEnvelope.reset();
        filterEnvelope.setParameters (
            params.filterEnvAttack->load(),
            params.filterEnvDecay->load(),
            params.filterEnvSustain->load(),
            params.filterEnvRelease->load()
        );
        filterEnvelope.trigger();

        // --- Amplitude Envelope (Phase 6.1 + 6.8 velocity modulation) ---
        // velocityRange=0 → flat dynamics (velocityScale fixed at 1.0, attack unchanged).
        // velocityRange=1 → full velocity sensitivity.
        //
        // Velocity mod targets (Phase 6.8):
        //   1 = AmpLevel:    velocityScale = lerp(1.0, velocity, velocityRange)
        //                    velocityRange=0 → all notes at full level.
        //                    velocityRange=1 → level scales linearly with velocity.
        //   2 = AmpAttack:   attackTime = attackParam * lerp(1.0, 0.1, vel * range)
        //                    Hard hit → shorter attack (bite). Soft hit → full attack.
        //                    0.1 floor prevents zero-length clicks at vel=1.
        //   3 = FilterCutoff: modulatedCutoff += vel * range * kVelCutoffRange
        //                     Applied additively at note-on, consistent with
        //                     filter-env and LFO modulation patterns.
        //   4 = FilterRes:   modulatedRes = baseRes + vel * range * kVelResRange
        //
        // Note: filter cutoff/resonance mods (targets 3, 4) require applying
        // the velocity offset before the filter env in renderNextBlock(). We store
        // them as velocity-derived offsets in velModCutoffHz and velModRes so they
        // are only computed once at note-on and referenced each block.
        // --- Velocity modulation (Phase 6.8 / 6.8b) ---
        // velocityModTarget enum:
        //   0=None, 1=AmpLevel, 2=AmpAttack, 3=FilterCutoff, 4=FilterResonance,
        //   5=Distortion, 6=Osc1Pitch, 7=OscMix.
        //
        // IMPORTANT: if AmpLevel (1) is in no slot, velocityScale = 1.0 (flat).
        // Every note plays at full volume. Set target to AmpLevel to re-enable dynamics.
        //
        // Distortion  (5): harder hit -> more per-voice saturation pre-filter.
        //                  Applied via SolaceDistortion::processSample() on preFiltL/R.
        // Osc1Pitch   (6): harder hit -> pitch rises up to kVelPitchCents cents sharp.
        //                  Applied at note-on via adjusted setTuningOffset call.
        // OscMix      (7): harder hit -> mix blends further toward Osc2.
        //                  Applied per-block: velModOscMixOffset added to APVTS oscMix.
        const float velRange = juce::jlimit (0.0f, 1.0f, params.velocityRange->load());
        const int   velTgt1  = static_cast<int> (params.velocityModTarget1->load());
        const int   velTgt2  = static_cast<int> (params.velocityModTarget2->load());
        const int   velTgt3  = static_cast<int> (params.velocityModTarget3->load());

        const bool velToAmpLevel   = (velTgt1 == 1 || velTgt2 == 1 || velTgt3 == 1);
        const bool velToAmpAttack  = (velTgt1 == 2 || velTgt2 == 2 || velTgt3 == 2);
        const bool velToFilterCut  = (velTgt1 == 3 || velTgt2 == 3 || velTgt3 == 3);
        const bool velToFilterRes  = (velTgt1 == 4 || velTgt2 == 4 || velTgt3 == 4);
        const bool velToDist       = (velTgt1 == 5 || velTgt2 == 5 || velTgt3 == 5);
        const bool velToOscPitch   = (velTgt1 == 6 || velTgt2 == 6 || velTgt3 == 6);
        const bool velToOscMix     = (velTgt1 == 7 || velTgt2 == 7 || velTgt3 == 7);

        // AmpLevel: lerp(1.0, velocity, velRange). Range=0 -> all notes full level.
        //           Range=1 -> level scales linearly with velocity.
        //           If NOT in any slot: flat (1.0f) -- velocity does not affect volume.
        velocityScale = velToAmpLevel
            ? juce::jmap (velRange, 0.0f, 1.0f, 1.0f, velocity)  // lerp(1.0, vel, range)
            : 1.0f;  // flat -- no amplitude dynamics when AmpLevel is not routed

        // AmpAttack: hard hit = shorter attack (punch). baseAttack floor = 0.1x.
        const float baseAttack = params.ampAttack->load();
        const float modAttack  = velToAmpAttack
            ? baseAttack * juce::jmap (velocity * velRange, 0.0f, 1.0f, 1.0f, 0.1f)
            : baseAttack;

        ampEnvelope.setParameters (
            modAttack,
            params.ampDecay->load(),
            params.ampSustain->load(),
            params.ampRelease->load()
        );
        ampEnvelope.trigger();

        // FilterCutoff / FilterResonance: stored as note-on offsets, applied per-sample.
        constexpr float kVelCutoffRange = 5000.0f;
        constexpr float kVelResRange    = 0.5f;
        velModCutoffHz = velToFilterCut ? velocity * velRange * kVelCutoffRange : 0.0f;
        velModRes      = velToFilterRes ? velocity * velRange * kVelResRange    : 0.0f;

        // Distortion (target 5): harder hit = more per-voice saturation pre-filter.
        // kVelDistRange=0.7: at max velocity+range, per-voice drive = 0.7.
        // Applied via SolaceDistortion::processSample() on preFiltL/R each sample.
        constexpr float kVelDistRange = 0.7f;
        velModDistDrive = velToDist ? velocity * velRange * kVelDistRange : 0.0f;

        // Osc1Pitch (target 6): harder hit = pitch shifts sharp by up to 100 cents.
        // Applied now at note-on by adding the cents offset to the tuning call.
        // Both osc1 and osc2 in all unison slots are adjusted identically so the
        // interval between them (and the detune spread) is preserved exactly.
        constexpr float kVelPitchCents = 100.0f;
        const float velPitchCents = velToOscPitch ? velocity * velRange * kVelPitchCents : 0.0f;

        if (velToOscPitch && velPitchCents != 0.0f)
        {
            // Store the velocity pitch as a multiplier on each oscillator.
            // setVelPitchMultiplier() is a separate member from setLFOPitchMultiplier(),
            // so LFO vibrato and velocity pitch are fully independent -- applied
            // multiplicatively in getNextSample(). This is set once at note-on and
            // stays constant for the note's lifetime (velocity doesn't change mid-note).
            const double velPitchMult = std::pow (2.0, static_cast<double>(velPitchCents) / 1200.0);
            for (int u = 0; u < activeUnisonCount; ++u)
            {
                unisonVoices[u].osc1.setVelPitchMultiplier (velPitchMult);
                unisonVoices[u].osc2.setVelPitchMultiplier (velPitchMult);
            }
        }
        else
        {
            for (int u = 0; u < activeUnisonCount; ++u)
            {
                unisonVoices[u].osc1.setVelPitchMultiplier (1.0);
                unisonVoices[u].osc2.setVelPitchMultiplier (1.0);
            }
        }

        // OscMix (target 7): harder hit = more Osc2. Stored as offset applied per-block.
        // velModOscMixOffset is clamped with base oscMix in renderNextBlock().
        constexpr float kVelOscMixRange = 0.5f;
        velModOscMixOffset = velToOscMix ? velocity * velRange * kVelOscMixRange : 0.0f;

        // LFO (Phase 6.6): no reset here -- free-running by design.
        // Pan gains (Phase 6.7): initialised in renderNextBlock() per-block from
        // unisonSpread, so live spread knob changes are audible while holding notes.

        // --- Pitch Bend at note-on (Phase 7 DSP) ---
        // JUCE's Synthesiser tracks lastPitchWheelValues[midiChannel] and passes
        // the correct per-channel value here as currentPitchWheelPosition.
        // Using it directly is simpler and more correct than reading a global atomic.
        _applyPitchBend (currentPitchWheelPosition);
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
            filterL.reset();
            filterR.reset();
            clearCurrentNote();
        }
    }

    // ========================================================================
    // renderNextBlock -- fill the output buffer for this voice.
    //
    // Signal flow (Phase 6.6+, per sample):
    //   lfoValue  = lfo.getNextSample() * lfoAmount      (per sample)
    //   osc1+osc2 blended by oscMix, with LFO pitch multiplier per block
    //   blendedOsc -> filterL/R (cutoff = filterEnv + LFO; resonance = base + LFO)
    //     -> * voiceGain * velocity * ampEnv * (amp LFO) -> output
    //
    // Per-block reads (before sample loop):
    //   baseCutoffHz, baseResonance, envDepth, oscMix
    //   lfoShape, lfoRate, lfoAmount, lfoTarget1/2/3
    //   osc pitch multipliers (from LFO, using getCurrentValue -- no phase advance)
    //
    // Per-sample reads (inside loop):
    //   osc1 + osc2 samples, filterEnvelope, filter.setCutoff/setResonance/process,
    //   ampEnvelope, lfoSample for filter/amp/level targets
    //
    // CRITICAL: addSample() not setSample() -- voices mix into shared buffer.
    // CRITICAL: startSample offset respected -- note may begin mid-block.
    // CRITICAL: No allocations, no locks, no logging on this thread.
    // ========================================================================
    void renderNextBlock (juce::AudioSampleBuffer& outputBuffer,
                          int startSample,
                          int numSamples) override
    {
        if (! ampEnvelope.isActive() && ! isVoiceActive())
            return;

        // --- Per-block parameter refresh ---
        baseCutoffHz = params.filterCutoff->load();
        const float baseResonance = params.filterResonance->load();
        const float envDepth      = params.filterEnvDepth->load();
        // OscMix: base APVTS value + velocity mod offset (target 7).
        // velModOscMixOffset is 0.0 when target 7 is not assigned, so this is
        // effectively just params.oscMix->load() when the target is inactive.
        const float mix = juce::jlimit (0.0f, 1.0f,
            params.oscMix->load() + velModOscMixOffset);

        // Filter and LFO modulation range constants.
        constexpr float kModRange       = 10000.0f;
        constexpr float kLFOCutoffRange = 10000.0f;
        constexpr float kLFOResRange    = 0.5f;
        constexpr float kLFOPitchSemi   = 2.0f;

        // --- Unison pan gains (Phase 6.7) ---
        // Recomputed per block from unisonSpread so the spread knob is live.
        // activeUnisonCount is snapshotted at note-on and never changes mid-note.
        const float spread = juce::jlimit (0.0f, 1.0f, params.unisonSpread->load());
        for (int u = 0; u < activeUnisonCount; ++u)
        {
            const float pan = (activeUnisonCount > 1)
                ? ((float)u / (float)(activeUnisonCount - 1) * 2.0f - 1.0f) * spread
                : 0.0f;  // N=1: always centre regardless of spread knob
            // Constant-power pan law: leftGain^2 + rightGain^2 = 1 at any pan position.
            unisonVoices[u].panL = std::sqrt (juce::jlimit (0.0f, 1.0f, 0.5f * (1.0f - pan)));
            unisonVoices[u].panR = std::sqrt (juce::jlimit (0.0f, 1.0f, 0.5f * (1.0f + pan)));
        }

        // --- LFO per-block refresh ---
        lfo.setShape  (static_cast<int> (params.lfoWaveform->load()));
        lfo.setRate   (params.lfoRate->load(), getSampleRate());
        const float lfoAmountKnob = juce::jlimit (0.0f, 1.0f, params.lfoAmount->load());
        // Mod wheel (CC#1, 0–127) adds to the knob value, clamped to [0, 1].
        // This gives classic "mod wheel = vibrato depth" behaviour: at lfoAmount=0
        // the mod wheel still works; at lfoAmount=1 it has no headroom to add.
        const float modWheelNorm = static_cast<float> (params.modWheelValue->load (std::memory_order_relaxed)) / 127.0f;
        const float lfoAmount    = juce::jlimit (0.0f, 1.0f, lfoAmountKnob + modWheelNorm);
        const int   lfoTarget1 = static_cast<int> (params.lfoTarget1->load());
        const int   lfoTarget2 = static_cast<int> (params.lfoTarget2->load());
        const int   lfoTarget3 = static_cast<int> (params.lfoTarget3->load());

        const bool lfoToFilterCutoff = (lfoTarget1 == 1 || lfoTarget2 == 1 || lfoTarget3 == 1);
        const bool lfoToOsc1Pitch    = (lfoTarget1 == 2 || lfoTarget2 == 2 || lfoTarget3 == 2);
        const bool lfoToOsc2Pitch    = (lfoTarget1 == 3 || lfoTarget2 == 3 || lfoTarget3 == 3);
        const bool lfoToOsc1Level    = (lfoTarget1 == 4 || lfoTarget2 == 4 || lfoTarget3 == 4);
        const bool lfoToOsc2Level    = (lfoTarget1 == 5 || lfoTarget2 == 5 || lfoTarget3 == 5);
        const bool lfoToAmpLevel     = (lfoTarget1 == 6 || lfoTarget2 == 6 || lfoTarget3 == 6);
        const bool lfoToFilterRes    = (lfoTarget1 == 7 || lfoTarget2 == 7 || lfoTarget3 == 7);

        // LFO pitch: compute multiplier once (both osc targets share same lfoValue).
        // getCurrentValue() does NOT advance the phase, so the single pow() is correct.
        if (lfoToOsc1Pitch || lfoToOsc2Pitch)
        {
            const double semi = static_cast<double> (lfo.getCurrentValue() * lfoAmount * kLFOPitchSemi);
            const double mult = std::pow (2.0, semi / 12.0);
            for (int u = 0; u < activeUnisonCount; ++u)
            {
                if (lfoToOsc1Pitch) unisonVoices[u].osc1.setLFOPitchMultiplier (mult);
                else                unisonVoices[u].osc1.setLFOPitchMultiplier (1.0);
                if (lfoToOsc2Pitch) unisonVoices[u].osc2.setLFOPitchMultiplier (mult);
                else                unisonVoices[u].osc2.setLFOPitchMultiplier (1.0);
            }
        }
        else
        {
            for (int u = 0; u < activeUnisonCount; ++u)
            {
                unisonVoices[u].osc1.setLFOPitchMultiplier (1.0);
                unisonVoices[u].osc2.setLFOPitchMultiplier (1.0);
            }
        }

        // Hoist channel count -- never changes mid-block.
        const int numChannels = outputBuffer.getNumChannels();

        while (--numSamples >= 0)
        {
            const float lfoSample = lfo.getNextSample() * lfoAmount;

            // --- Per-unison oscillator accumulation ---
            // Each unison voice advances its own detuned osc pair and contributes
            // its sample to L and R *separately* using its individual pan gains.
            // This produces genuinely different pre-filter content per channel,
            // which is what creates real stereo width after filtering.
            // (contrast with the broken previous approach which summed to mono first
            // and then re-applied pan gains, yielding identical L and R channels)
            float preFiltL = 0.0f;
            float preFiltR = 0.0f;
            for (int u = 0; u < activeUnisonCount; ++u)
            {
                float uOsc1s = unisonVoices[u].osc1.getNextSample();
                float uOsc2s = unisonVoices[u].osc2.getNextSample();

                // Osc level LFO (targets 4, 5): applied uniformly to all unison voices.
                if (lfoToOsc1Level) uOsc1s *= juce::jlimit (0.0f, 2.0f, 1.0f + lfoSample);
                if (lfoToOsc2Level) uOsc2s *= juce::jlimit (0.0f, 2.0f, 1.0f + lfoSample);

                const float uMixed = uOsc1s * (1.0f - mix) + uOsc2s * mix;

                preFiltL += uMixed * unisonVoices[u].panL;
                preFiltR += uMixed * unisonVoices[u].panR;
            }

            // Filter envelope + LFO cutoff/resonance modulation.
            // Both filters always receive identical cutoff and resonance.
            // They diverge sonically only because their inputs (preFiltL vs preFiltR)
            // carry different detuned oscillator blends.
            const float filterEnvVal = filterEnvelope.getNextSample();
            float modulatedCutoff    = baseCutoffHz
                                     + filterEnvVal * envDepth * kModRange
                                     + velModCutoffHz;   // Phase 6.8: velocity mod
            if (lfoToFilterCutoff)   modulatedCutoff += lfoSample * kLFOCutoffRange;

            // Resonance: base + velocity mod offset (Phase 6.8) + LFO. Clamped 0-1.
            const float modulatedRes = juce::jlimit (0.0f, 1.0f,
                baseResonance + velModRes + (lfoToFilterRes ? lfoSample * kLFOResRange : 0.0f));

            filterL.setCutoff    (modulatedCutoff); filterR.setCutoff    (modulatedCutoff);
            filterL.setResonance (modulatedRes);    filterR.setResonance (modulatedRes);

            // Per-voice distortion (Phase 6.8b, target 5): velocity -> drive.
            // Applied pre-filter on both channels. velModDistDrive=0.0 when target
            // is inactive, in which case processSample() is bypassed entirely.
            if (velModDistDrive > 0.0f)
            {
                preFiltL = SolaceDistortion::processSample (preFiltL, velModDistDrive);
                preFiltR = SolaceDistortion::processSample (preFiltR, velModDistDrive);
            }

            const float filteredL = filterL.processSample (preFiltL);
            const float filteredR = filterR.processSample (preFiltR);

            // Amp envelope + LFO amp modulation.
            const float ampEnvVal = ampEnvelope.getNextSample();
            const float ampMod    = lfoToAmpLevel
                ? juce::jlimit (0.0f, 2.0f, 1.0f + lfoSample)
                : 1.0f;

            // Final gain: voiceGain provides equal-power unison normalisation.
            // At N=1 spread=0: preFiltL == preFiltR == osc * sqrt(0.5),
            // so filteredL == filteredR, and the output is centred mono --
            // backwards-compatible with Phase 6.6.
            const float gainScalar = voiceGain * velocityScale * ampEnvVal * ampMod;
            const float sampleL    = filteredL * gainScalar;
            const float sampleR    = filteredR * gainScalar;

            // Write to output buffer. Mono hosts (numChannels==1) get an
            // explicit fold-down rather than the left channel only -- important
            // because with spread>0 some unison voices pan mostly right and
            // would be lost if we only wrote sampleL.
            if (numChannels >= 2)
            {
                outputBuffer.addSample (0, startSample, sampleL);
                outputBuffer.addSample (1, startSample, sampleR);
            }
            else if (numChannels == 1)
            {
                outputBuffer.addSample (0, startSample, 0.5f * (sampleL + sampleR));
            }

            ++startSample;

            if (! ampEnvelope.isActive())
            {
                clearCurrentNote();
                break;
            }
        }
    }

    // ========================================================================
    // pitchWheelMoved -- called by JUCE Synthesiser on MIDI pitch wheel events.
    //
    // Converts the 14-bit wheel position (0-16383, centred at 8192) to a
    // frequency multiplier using a ±2 semitone range (standard MIDI default).
    // Applied multiplicatively via setPitchBendMultiplier() on all active unison
    // oscillators. The bend stays applied until the wheel returns to centre;
    // no state reset on note-off (consistent with real hardware behaviour).
    //
    // Bend range: ±2 semitones (kPitchBendRange). Industry standard default.
    //   V2: make this a user-configurable APVTS parameter.
    // ========================================================================
    void pitchWheelMoved (int newValue) override
    {
        _applyPitchBend (newValue);
    }

    // ========================================================================
    // controllerMoved -- called by JUCE Synthesiser on MIDI CC events.
    //
    // CC#1 is the mod wheel (standard MIDI). We store the value on the
    // synthesiser-level atomic (via SolaceVoiceParams pointer) and apply it
    // per-block in renderNextBlock() to the LFO amount.
    //
    // All other CC messages are ignored at the voice level in V1.
    // ========================================================================
    void controllerMoved (int controller, int /*value*/) override
    {
        // The actual mod wheel value is already captured into the synthesiser-level
        // atomic by SolaceSynthesiser::handleMidiEvent() before this call arrives.
        // Nothing more to do here -- renderNextBlock() reads it each block.
        (void) controller;
    }

private:
    const SolaceVoiceParams params;

    // --- Phase 6.1: Amplitude envelope ---
    SolaceADSR ampEnvelope;

    // --- Phase 6.3: Filter ---
    // Two instances (filterL, filterR) process the left and right pre-filter signals
    // independently. This is what makes stereo spread real: each channel receives a
    // different mix of detuned oscillators (per-voice panL/panR weighting applied
    // before summing into preFiltL / preFiltR), so filterL and filterR see genuinely
    // different inputs and produce genuinely different outputs.
    //
    // At N=1 (unisonCount=1), panL == panR == sqrt(0.5), so preFiltL == preFiltR and
    // both filters receive identical inputs -- output is centred mono, fully backward-
    // compatible with Phase 6.6.
    //
    // Both filters are always set to the same mode, cutoff, and resonance; they differ
    // only in their input signal content.
    SolaceFilter filterL;
    SolaceFilter filterR;
    float baseCutoffHz = 20000.0f;

    // --- Phase 6.4: Filter Envelope ---
    SolaceADSR filterEnvelope;

    // --- Phase 6.6: LFO ---
    SolaceLFO lfo;

    // --- Phase 6.7: Unison oscillator array ---
    // Replaces the former standalone osc1 + osc2 members.
    // UnisonVoice holds an osc1/osc2 pair plus pre-computed stereo pan gains.
    // kMaxUnison = 8 matches the APVTS unisonCount parameter range [1, 8].
    // All kMaxUnison slots are always allocated (stack); only activeUnisonCount
    // are rendered each block. At activeUnisonCount=1, voice renders identically
    // to Phase 6.6 with no CPU overhead for unused slots.
    static constexpr int kMaxUnison = 8;

    struct UnisonVoice
    {
        SolaceOscillator osc1;
        SolaceOscillator osc2;
        float panL = 1.0f;  // left channel gain for this unison voice
        float panR = 1.0f;  // right channel gain for this unison voice
    };

    UnisonVoice unisonVoices[kMaxUnison];

    // Number of active unison voices. Snapshotted at note-on from unisonCount param.
    // Never changes mid-note; next note-on picks up any knob changes.
    int activeUnisonCount = 1;

    // Velocity scaling -- set at note-on, multiplied per sample.
    float velocityScale = 0.0f;

    // Phase 6.8 velocity-to-filter offsets. Computed once at note-on and applied
    // every sample in renderNextBlock() alongside the filter envelope and LFO.
    float velModCutoffHz    = 0.0f;  // Hz offset: velocity -> filter cutoff (target 3)
    float velModRes         = 0.0f;  // [0-1] offset: velocity -> resonance (target 4)
    float velModDistDrive   = 0.0f;  // [0-1] drive: velocity -> per-voice distortion (target 5)
    float velModOscMixOffset = 0.0f; // [0-1] offset: velocity -> osc mix toward Osc2 (target 7)

    // Per-voice output gain, updated at note-on.
    // Incorporates equal-power unison normalisation: kBaseVoiceGain / sqrt(unisonCount).
    // At unisonCount=1, voiceGain = kBaseVoiceGain / sqrt(1) = kBaseVoiceGain.
    //
    // Why kBaseVoiceGain = 0.15 * sqrt(2) (same value as before the dual-filter change):
    // With dual-filter stereo, at N=1 spread=0:
    //   panL = panR = sqrt(0.5) = 0.707
    //   preFiltL = osc * 0.707, preFiltR = osc * 0.707
    //   filteredL = filter(osc * 0.707), filteredR = filter(osc * 0.707)
    //   sampleL = filteredL * kBaseVoiceGain = filter(osc) * 0.707 * kBaseVoiceGain
    //
    // To match the old kVoiceGain=0.15 mono output:
    //   0.707 * kBaseVoiceGain = 0.15 -> kBaseVoiceGain = 0.15 / 0.707 = 0.2121 = 0.15 * sqrt(2)
    //
    // kBaseVoiceGain unchanged from before at 0.15 * sqrt(2). Math still holds.
    // ========================================================================
    // _applyPitchBend -- convert MIDI pitch wheel value to frequency multiplier
    // and apply to all active unison oscillators.
    //
    // Called from pitchWheelMoved() (real-time, while note is playing) and
    // from startNote() (primes the multiplier at note onset).
    //
    // wheelValue: 0-16383 (JUCE standard), centred at 8192.
    //   0     = maximum downward bend (-kPitchBendRange semitones)
    //   8192  = centre (no bend),  multiplier = 1.0
    //   16383 = maximum upward bend (+kPitchBendRange semitones)
    // ========================================================================
    void _applyPitchBend (int wheelValue) noexcept
    {
        // Normalise to [-1.0, +1.0]. JUCE uses 0-16383 with centre at 8192.
        const double normalised = (static_cast<double> (wheelValue) - 8192.0) / 8192.0;
        const double semitones  = normalised * static_cast<double> (kPitchBendRange);
        const double multiplier = std::pow (2.0, semitones / 12.0);

        for (int u = 0; u < activeUnisonCount; ++u)
        {
            unisonVoices[u].osc1.setPitchBendMultiplier (multiplier);
            unisonVoices[u].osc2.setPitchBendMultiplier (multiplier);
        }
    }

    // Pitch bend range in semitones (\u00b12 semitones = MIDI default).
    // V2: expose as user-configurable APVTS parameter.
    static constexpr int kPitchBendRange = 2;

    static constexpr float kBaseVoiceGain = 0.15f * 1.4142135f;  // 0.15 * sqrt(2)
    float voiceGain = kBaseVoiceGain;  // updated at startNote()
};
