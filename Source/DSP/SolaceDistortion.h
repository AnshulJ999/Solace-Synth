// ============================================================================
// SolaceDistortion.h — Master distortion (soft-clipper)
//
// Phase 6.9: Global post-process applied per sample to the final stereo mix,
// after synth.renderNextBlock() and before the master volume gain stage.
// Phase 6.8b: Also used for per-voice velocity-to-distortion modulation target.
//
// Algorithm: hyperbolic tangent soft-clip, gain-normalised so the formula is
// naturally transparent at drive=0 and gradually saturates as drive increases:
//
//   k = drive * 10.0         (maps drive 0.0→1.0 to k 0.0→10.0)
//   y = tanh(k * x) / tanh(k)
//
//   As k→0, tanh(k*x)/tanh(k) → x (L'Hôpital's rule). This means drive=0
//   is *mathematically* unity gain -- no discontinuous jump when the knob first
//   moves off zero, and no crossfade hack needed.
//
//   Previous formula: k = 1 + drive * 9 (k range 1→10). The old mapping
//   created a +31% volume jump the moment drive exceeded 0.0f, because the
//   formula was already applying gain at k=1. The new mapping fixes this by
//   making k=0 the natural transparent point.
//
//   At drive=1: k=10 -- same maximum saturation intensity as before.
//   At drive=0.5: k=5 (previously k=5.5). Midrange character shifts slightly
//   but is irrelevant pre-release.
//
//   For inputs within [-1.0, +1.0], output is within ±1.0. For inputs outside
//   that range (dense chords + high resonance), output may exceed ±1.0 but
//   remains finite -- no NaN or inf at any valid drive setting.
//
// Design decisions:
//   - No state — purely sample-in sample-out. No prepare() needed.
//   - Static method — instantiated as a value type in PluginProcessor.
//   - std::tanh used for float version (avoids implicit double promotion).
//   - Applied per channel independently (L and R) for correct stereo handling.
//   - Outer bypass guards in PluginProcessor.cpp and SolaceVoice.h skip the
//     function entirely when drive == 0.0f (free CPU win for the common case).
//
// ⚠️ Performance note (V1.1): For the velocity-to-distortion target, k and
// tanhK are constant for a note's lifetime but are recomputed every sample.
// If CPU headroom is tight at high polyphony, precompute k and tanhK at
// note-on and store as SolaceVoice members. Tracked for V1.1.
// ============================================================================

#pragma once

#include <cmath>

class SolaceDistortion
{
public:
    // ========================================================================
    // processSample — apply tanh soft-clip to a single sample.
    //
    // Parameters:
    //   x     — input sample (expected range ≈ ±1.0 but handles > 1.0 safely)
    //   drive — 0.0 (transparent) to 1.0 (heavy clip), mapped to k=0..10
    //
    // Returns:
    //   Soft-clipped sample. At drive=0, returns x exactly (unity gain).
    //   At drive=1, returns maximum tanh saturation (k=10).
    //   Callers may additionally guard with `if (drive > 0.0f)` to skip the
    //   function call entirely in the common no-distortion case.
    //
    // Audio thread safe: no allocation, no I/O, no state mutated.
    // ========================================================================
    static float processSample (float x, float drive) noexcept
    {
        // Fast bypass: drive=0 is mathematically unity, but skipping tanhf
        // entirely is cheaper. Outer guards in PluginProcessor/SolaceVoice
        // also catch this, so this inner guard is a safety net.
        if (drive <= 0.0f)
            return x;

        const float k     = drive * 10.0f;   // drive=0→k=0 (transparent), drive=1→k=10 (heavy)
        const float tanhK = std::tanh (k);

        // Guard: tanhK near zero means k≈0 -- return x (mathematically correct limit).
        if (tanhK < 1e-6f)
            return x;

        return std::tanh (k * x) / tanhK;
    }
};
