#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

// ============================================================================
// SolaceADSR — Thin Wrapper Around juce::ADSR
//
// Provides a clean, consistent API for amplitude and filter envelopes.
// All DSP modules in Solace follow the same prepare() / process pattern.
//
// Usage:
//   1. Call prepare(sampleRate) once from SolaceVoice::prepare().
//   2. Call setParameters() at note-on with values read from APVTS atomics.
//   3. Call trigger() on note-on to begin the attack phase.
//   4. Call release() on note-off to begin the release phase.
//   5. Call getNextSample() once per audio sample in renderNextBlock().
//   6. When isActive() returns false, the envelope has fully completed.
//
// Thread safety:
//   Designed to run exclusively on the audio thread.
//   Do NOT share a single SolaceADSR instance across voices.
// ============================================================================

class SolaceADSR
{
public:
    SolaceADSR() = default;

    // ========================================================================
    // prepare — set the sample rate for correct timing calculations.
    // Must be called before any other method. Call from SolaceVoice::prepare().
    // ========================================================================
    void prepare (double sampleRate)
    {
        adsr.setSampleRate (sampleRate);
    }

    // ========================================================================
    // setParameters — update ADSR times and sustain level.
    // Call at note-on with values freshly read from APVTS atomics.
    // All time values are in seconds. Sustain is a linear level (0.0–1.0).
    // ========================================================================
    void setParameters (float attackSec, float decaySec, float sustain, float releaseSec)
    {
        juce::ADSR::Parameters p;
        p.attack  = attackSec;
        p.decay   = decaySec;
        p.sustain = sustain;
        p.release = releaseSec;
        adsr.setParameters (p);
    }

    // ========================================================================
    // trigger — begin the attack phase (call on note-on after setParameters).
    // ========================================================================
    void trigger()  { adsr.noteOn(); }

    // ========================================================================
    // release — begin the release phase (call on note-off).
    // The voice remains active until the release phase completes and
    // isActive() returns false.
    // ========================================================================
    void release()  { adsr.noteOff(); }

    // ========================================================================
    // getNextSample — advance the envelope and return the current level.
    // Returns a value in the range 0.0–1.0.
    // Call exactly once per audio sample in renderNextBlock().
    // ========================================================================
    float getNextSample()  { return adsr.getNextSample(); }

    // ========================================================================
    // isActive — returns true while the envelope is in any non-idle state
    // (attack, decay, sustain, or release). Returns false only after the
    // release phase has fully decayed to zero.
    // ========================================================================
    bool isActive() const  { return adsr.isActive(); }

    // ========================================================================
    // reset — instantly force the envelope to idle, silencing any tail.
    // Use for voice stealing (stopNote with allowTailOff = false).
    // ========================================================================
    void reset()  { adsr.reset(); }

private:
    juce::ADSR adsr;
};
