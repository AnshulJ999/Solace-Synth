#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

// ============================================================================
// SolaceOscillator — Per-Voice Waveform Oscillator
//
// Generates one of four waveforms via phase accumulation. One instance lives
// inside each SolaceVoice (Osc1). Phase 6.5 adds a second instance (Osc2).
// Phase 6.6 adds LFO pitch modulation via setLFOPitchMultiplier().
//
// Waveform index mapping (matches APVTS "osc1Waveform" / "osc2Waveform"):
//   0 = Sine      — std::sin(angle)
//   1 = Sawtooth  — naïve, linear ramp [-1, +1]; aliasing acceptable in V1
//   2 = Square    — naïve ±1 pulse; aliasing acceptable in V1
//   3 = Triangle  — folded ramp; band-limited enough to be usable at V1 stage
//
// V2 note: Replace the naïve waveforms with PolyBLEP or wavetable LUTs.
//          The module boundary is clean — only getNextSample() changes.
//
// Tuning offset:
//   setTuningOffset(octave, transpose, cents) converts discrete tuning controls
//   to a frequency multiplier applied inside setFrequency(). This keeps the
//   full equal-temperament formula in one place.
//
// Usage per voice:
//   1. Call setWaveform(index) at note-on (read from APVTS).
//   2. Call setTuningOffset(octave, transpose, cents) at note-on.
//          MUST be called before setFrequency() — setFrequency() reads the
//          tuning multiplier computed here.
//   3. Call setFrequency(hz, sampleRate) at note-on.
//   4. Call reset() at note-on to restart phase from zero (click-free attack).
//   5. Call getNextSample() once per sample in renderNextBlock().
//
// Thread safety:
//   Designed to run exclusively on the audio thread.
//   Do NOT share a single SolaceOscillator instance across voices.
// ============================================================================

class SolaceOscillator
{
public:
    // Waveform indices — kept as an enum for readability inside SolaceVoice.
    // The int values match the APVTS AudioParameterInt range [0, 3].
    enum Waveform
    {
        Sine     = 0,
        Sawtooth = 1,
        Square   = 2,
        Triangle = 3
    };

    SolaceOscillator() = default;

    // ========================================================================
    // setWaveform — select the output waveform.
    // Call at note-on after reading osc1Waveform (or osc2Waveform) from APVTS.
    // Values outside [0, 3] fall back to Sine (defensive guard).
    // ========================================================================
    void setWaveform (int waveformIndex) noexcept
    {
        switch (waveformIndex)
        {
            case Sawtooth: waveform = Sawtooth; break;
            case Square:   waveform = Square;   break;
            case Triangle: waveform = Triangle; break;
            default:       waveform = Sine;     break;  // 0 and any invalid index
        }
    }

    // ========================================================================
    // setTuningOffset — store the discrete tuning controls.
    //
    // These are converted to a frequency multiplier inside setFrequency().
    // Must be called before setFrequency() so the multiplier is up to date.
    //
    //   octave    — integer octave shift (-3 to +3); doubles/halves frequency
    //   transpose — integer semitone shift (-12 to +12); equal temperament steps
    //   cents     — fine tuning (-100.0 to +100.0 cents); 100 cents = 1 semitone
    //
    // Formula (equal temperament):
    //   multiplier = 2^octave * 2^(semitones/12) * 2^(cents/1200)
    // ========================================================================
    void setTuningOffset (int octave, int transpose, float cents) noexcept
    {
        tuningMultiplier = std::exp2 (static_cast<double> (octave))
                         * std::exp2 (static_cast<double> (transpose) / 12.0)
                         * std::exp2 (static_cast<double> (cents)     / 1200.0);
    }

    // ========================================================================
    // setFrequency — compute the per-sample phase increment.
    //
    // Call after setTuningOffset() so the tuning multiplier is current.
    // hz is the base MIDI note frequency (getMidiNoteInHertz). sampleRate is
    // the current audio engine sample rate (from getSampleRate() on the voice).
    // ========================================================================
    void setFrequency (double hz, double sampleRate) noexcept
    {
        jassert (sampleRate > 0.0);
        const double tunedHz       = hz * tuningMultiplier;
        const double cyclesPerSample = tunedHz / sampleRate;
        angleDelta = cyclesPerSample * juce::MathConstants<double>::twoPi;
    }

    // ========================================================================
    // reset — restart the phase accumulator from zero.
    // Call at every note-on to ensure a phase-coherent, click-free attack.
    // ========================================================================
    void reset() noexcept
    {
        currentAngle = 0.0;
    }

    // ========================================================================
    // setLFOPitchMultiplier -- apply LFO vibrato as a frequency multiplier.
    //
    // Called once per render block (not per sample) before the sample loop.
    // The multiplier is applied inside getNextSample() by scaling angleDelta.
    //
    //   multiplier = 2^(lfoSemitones / 12)  (equal-temperament pitch shift)
    //   multiplier = 1.0                     (no LFO effect -- default)
    //
    // This is cheaper than recalling setFrequency() per block because it avoids
    // the exp2() and division; the oscillator already has the base angleDelta set.
    // The compiler will see the 1.0 case in the hot loop and may elide the
    // multiply when the target is not a pitch target.
    // ========================================================================
    void setLFOPitchMultiplier (double multiplier) noexcept
    {
        lfoMultiplier = multiplier;
    }

    // ========================================================================
    // getNextSample — advance the oscillator and return the current sample.
    //
    // Returns a value in [-1.0, +1.0] for all waveforms.
    // Must be called exactly once per audio sample in renderNextBlock().
    //
    // Phase wrapping: currentAngle is kept in [0, 2π]. Wrapping every sample
    // prevents floating-point precision drift at high sample counts (long notes).
    // ========================================================================
    float getNextSample() noexcept
    {
        // Compute sample for the current phase.
        float sample = computeSample (currentAngle);

        // Advance phase accumulator. Apply LFO pitch multiplier (default 1.0 = no effect).
        currentAngle += angleDelta * lfoMultiplier;

        // Wrap to [0, 2π]. Using a while loop rather than fmod (fmod is more
        // expensive on the audio thread). A single subtraction is insufficient
        // when angleDelta > 2π — which is reachable for high MIDI notes with
        // octave/transpose/cents offsets (e.g. MIDI 127 + osc1Octave=+3 gives
        // angleDelta ≈ 30 radians). The loop runs at most a handful of
        // iterations (ceil(angleDelta / 2π) - 1), so it is audio-thread safe.
        while (currentAngle >= juce::MathConstants<double>::twoPi)
            currentAngle -= juce::MathConstants<double>::twoPi;

        return sample;
    }

private:
    // ========================================================================
    // computeSample — maps currentAngle to the selected waveform output.
    //
    // All formulas produce output in [-1.0, +1.0], matching sine's range.
    //
    // Naïve waveforms (Saw, Square) alias at high frequencies. This is
    // intentional for V1. PolyBLEP replacement is tracked in the V2 plan.
    // Triangle is naturally band-limited (gentle slope) — alias is minimal.
    // ========================================================================
    float computeSample (double angle) const noexcept
    {
        const double twoPi = juce::MathConstants<double>::twoPi;
        const double pi    = juce::MathConstants<double>::pi;

        switch (waveform)
        {
            case Sine:
                // Standard sine. Exact JUCE tutorial waveform — no alias.
                return static_cast<float> (std::sin (angle));

            case Sawtooth:
                // Linear ramp from -1 to +1 over one cycle (rising sawtooth).
                // angle / twoPi maps [0, 2π] → [0, 1); *2 - 1 → [-1, +1).
                // Wraps discontinuously at 2π → 0 (generates alias harmonics).
                // Note: rising and falling saws have identical harmonic content
                // (just phase-inverted), so this is acoustically equivalent.
                return static_cast<float> (2.0 * (angle / twoPi) - 1.0);

            case Square:
                // +1 for first half-cycle, -1 for second.
                // Naive but correct. Alias from sharp edges — fine in V1.
                return (angle < pi) ? 1.0f : -1.0f;

            case Triangle:
                // Triangle via folded sawtooth: y = 1 - 2|2*(t) - 1|
                // where t = angle / twoPi maps to [0, 1].
                // Equivalent to: rise from -1 → +1 then fall +1 → -1 per cycle.
                {
                    const double t = angle / twoPi;  // [0, 1)
                    return static_cast<float> (2.0 * std::abs (2.0 * t - 1.0) - 1.0);
                }

            default:
                return 0.0f;  // Unreachable — all enum cases handled above.
        }
    }

    // Current waveform selection.
    Waveform waveform = Sine;

    // Phase accumulator — kept in [0, 2π].
    double currentAngle = 0.0;

    // Per-sample phase increment = (frequency / sampleRate) * 2π.
    double angleDelta = 0.0;

    // Frequency multiplier from tuning controls (octave + semitones + cents).
    // Default 1.0 = no offset. Recomputed by setTuningOffset() at each note-on.
    double tuningMultiplier = 1.0;

    // LFO pitch multiplier = 2^(lfoSemitones/12). Default 1.0 = no modulation.
    // Set once per render block by setLFOPitchMultiplier() when a pitch LFO
    // target is active; reset to 1.0 when the target is None.
    double lfoMultiplier = 1.0;
};
