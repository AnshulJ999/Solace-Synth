#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

// ============================================================================
// SolaceLFO -- Per-Voice Low Frequency Oscillator
//
// Free-running oscillator used for slow modulation (vibrato, filter wah,
// tremolo, resonance sweep). One instance lives inside each SolaceVoice.
//
// Free-running design (Phase 6.6):
//   Each instance is seeded with a unique random initial phase at construction,
//   so chord voices have independent LFO positions from the very first note.
//   The phase is NEVER reset at note-on -- voices retain their divergent phases
//   throughout their lifetimes, creating natural shimmer on pads and sweeps.
//   Note: the LFO advances only during active voice rendering (not while idle).
//   "Free-running" here means "random initial phase + never key-synced" --
//   the perceptual goal of per-voice phase diversity is fully achieved without
//   any idle-voice CPU overhead.
//   A synchronised LFO (reset at note-on) is a different character and is
//   out of scope for V1.
//
// Waveform index mapping (matches APVTS "lfoWaveform" int range 0-4):
//   0 = Sine      -- smooth periodic sweep (classic vibrato, filter wah)
//   1 = Triangle  -- linear up/down ramp (sharper transitions than sine)
//   2 = Sawtooth  -- rising ramp, hard reset at cycle end
//   3 = Square    -- binary +1/-1 (hard gating tremolo)
//   4 = S&H       -- sample-and-hold: latches a new random value each cycle
//
// Output range: [-1.0, +1.0] for all waveforms.
//
// API:
//   setShape(int)              -- select waveform (valid range 0-4; others fall back to Sine)
//   setRate(float hz, double sampleRate) -- set LFO frequency, call once per render block
//   getCurrentValue()          -- returns current value WITHOUT advancing phase
//                                 (use for per-block pitch target setup)
//   getNextSample()            -- advances phase by one sample and returns value
//                                 (call once per sample inside renderNextBlock())
//   reset()                    -- exists for completeness; do NOT call in startNote()
//
// Thread safety:
//   Designed to run exclusively on the audio thread.
//   Do NOT share a SolaceLFO instance across voices.
// ============================================================================

class SolaceLFO
{
public:
    // Waveform indices -- int values match APVTS AudioParameterInt range [0, 4].
    enum Shape
    {
        Sine     = 0,
        Triangle = 1,
        Sawtooth = 2,
        Square   = 3,
        SandH    = 4
    };

    // Constructor: seed each instance with a unique random initial phase and
    // S&H hold value, and re-seed the per-instance juce::Random.
    //
    // Why this is necessary:
    //   (a) Initial phase -- All 16 voice LFOs would otherwise start at
    //       currentAngle = 0.0. A simultaneous chord gives all voices the same
    //       LFO phase -- no shimmer. The random seed gives immediate per-voice
    //       phase diversity from the very first note.
    //
    //   (b) S&H initial value -- sAndHValue = 0.0 by default and only receives
    //       a real random value after the first phase wrap. At slow rates this
    //       means 1+ seconds of silent S&H modulation on first use. Seeding
    //       here makes S&H effective immediately.
    //
    //   (c) juce::Random seed -- The default juce::Random() constructor uses
    //       seed = 1 for every instance. Without re-seeding, all voices produce
    //       the same S&H random sequence and latch identical values at the same
    //       time -- defeating per-voice independence entirely.
    //
    // Thread safety: all SolaceVoice instances are constructed sequentially
    // on the message thread in PluginProcessor's constructor, so sequential
    // calls to getSystemRandom() do not race.
    SolaceLFO()
    {
        auto& sys    = juce::Random::getSystemRandom();
        currentAngle = sys.nextDouble() * juce::MathConstants<double>::twoPi;
        sAndHValue   = sys.nextFloat() * 2.0f - 1.0f;
        random.setSeed (sys.nextInt64());
    }

    // ========================================================================
    // setShape -- select the LFO waveform.
    // Values outside [0, 4] fall back to Sine (defensive guard).
    // ========================================================================
    void setShape (int index) noexcept
    {
        switch (index)
        {
            case Triangle: shape = Triangle; break;
            case Sawtooth: shape = Sawtooth; break;
            case Square:   shape = Square;   break;
            case SandH:    shape = SandH;    break;
            default:       shape = Sine;     break;  // 0 and any invalid index
        }
    }

    // ========================================================================
    // setRate -- convert LFO rate in Hz to a per-sample phase increment.
    //
    // Call once per render block (not per sample) for live rate knob response.
    // Internally clamps to [0.01, 50.0] Hz to prevent degenerate states.
    // ========================================================================
    void setRate (float hz, double sampleRate) noexcept
    {
        jassert (sampleRate > 0.0);
        const double clamped = juce::jlimit (0.01, 50.0, static_cast<double> (hz));
        angleDelta = (clamped / sampleRate) * juce::MathConstants<double>::twoPi;
    }

    // ========================================================================
    // getCurrentValue -- read the current LFO output WITHOUT advancing phase.
    //
    // Use this to compute per-block pitch multipliers BEFORE the sample loop
    // begins. The per-sample getNextSample() loop then remains the sole source
    // of LFO phase advancement, keeping the LFO rate correct.
    // ========================================================================
    float getCurrentValue() const noexcept
    {
        return computeSample (currentAngle);
    }

    // ========================================================================
    // getNextSample -- advance by one sample and return the new LFO value.
    //
    // Returns [-1.0, +1.0]. Call exactly once per audio sample inside
    // renderNextBlock(). For S&H, a new random value is latched at each cycle
    // wrap (detected when currentAngle passes 2*pi).
    // ========================================================================
    float getNextSample() noexcept
    {
        const float sample = computeSample (currentAngle);

        currentAngle += angleDelta;

        // Phase wrap [0, 2pi]. Using while loop in case angleDelta > 2pi
        // (would only happen at extremely high LFO rates near audio rate, but
        // the guard makes the class safe under any setRate() call).
        const bool wrapped = currentAngle >= juce::MathConstants<double>::twoPi;
        while (currentAngle >= juce::MathConstants<double>::twoPi)
            currentAngle -= juce::MathConstants<double>::twoPi;

        // S&H: latch a new random value on each cycle boundary.
        // The hold value is returned during the next cycle via computeSample().
        if (shape == SandH && wrapped)
            sAndHValue = random.nextFloat() * 2.0f - 1.0f;

        return sample;
    }

    // ========================================================================
    // reset -- restart the phase accumulator from zero.
    //
    // Exists for completeness. Do NOT call from SolaceVoice::startNote() --
    // each voice has a unique random initial phase from construction, giving
    // organic chord shimmer. Calling reset() in startNote() would destroy that
    // independence and produce key-synced behaviour (all voices back to 0).
    // ========================================================================
    void reset() noexcept
    {
        currentAngle = 0.0;
    }

private:
    // ========================================================================
    // computeSample -- maps angle to the selected waveform output.
    //
    // All formulas produce output in [-1.0, +1.0].
    // angle is assumed to be in [0, 2pi].
    // ========================================================================
    float computeSample (double angle) const noexcept
    {
        const double twoPi = juce::MathConstants<double>::twoPi;
        const double pi    = juce::MathConstants<double>::pi;

        switch (shape)
        {
            case Sine:
                return static_cast<float> (std::sin (angle));

            case Triangle:
            {
                // Folded sawtooth: rises from -1 to +1 in first half-cycle,
                // falls from +1 to -1 in second half. Same formula as
                // SolaceOscillator's triangle, applied at LFO rates.
                const double t = angle / twoPi;  // [0, 1)
                return static_cast<float> (2.0 * std::abs (2.0 * t - 1.0) - 1.0);
            }

            case Sawtooth:
                // Rising ramp: -1 at angle=0, approaching +1 just before 2pi,
                // then hard reset. Classic LFO saw for filter sweeps.
                return static_cast<float> (2.0 * (angle / twoPi) - 1.0);

            case Square:
                // +1 for first half-cycle, -1 for second.
                // Hard gating effect for tremolo and rhythmic modulation.
                return (angle < pi) ? 1.0f : -1.0f;

            case SandH:
                // Hold value is updated once per cycle in getNextSample().
                // getCurrentValue() returns the same held value between updates.
                return sAndHValue;

            default:
                return 0.0f;  // Unreachable -- all enum cases handled above.
        }
    }

    // Current waveform shape.
    Shape shape = Sine;

    // Phase accumulator -- kept in [0, 2pi].
    // Seeded with a unique random value at construction (see constructor).
    // Never reset at note-on -- retains its phase across notes for organic shimmer.
    double currentAngle = 0.0;

    // Per-sample phase increment = (hz / sampleRate) * 2pi.
    // Default 0.0 keeps LFO static until setRate() is called in the first block.
    double angleDelta = 0.0;

    // S&H hold value -- updated once per LFO cycle on phase wrap.
    // Seeded with a random value at construction so S&H is effective immediately,
    // not silent for the entire first cycle at slow LFO rates.
    float sAndHValue = 0.0f;

    // Per-LFO-instance RNG for S&H. Re-seeded at construction (see constructor)
    // because juce::Random() default constructor uses seed = 1 for all instances --
    // without re-seeding, all voices would produce the same S&H sequence and
    // latch identical values simultaneously, breaking per-voice independence.
    juce::Random random;
};
