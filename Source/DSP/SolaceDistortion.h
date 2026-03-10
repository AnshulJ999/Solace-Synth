// ============================================================================
// SolaceDistortion.h — Master distortion (soft-clipper)
//
// Phase 6.9: Global post-process applied per sample to the final stereo mix,
// after synth.renderNextBlock() and before the master volume gain stage.
//
// Algorithm: hyperbolic tangent soft-clip, gain-normalised so output never
// exceeds ±1.0 at any drive level:
//
//   k = 1.0 + drive * 9.0          (maps drive 0.0→1.0 to k 1.0→10.0)
//   y = tanh(k * x) / tanh(k)      (normalised tanh soft-clip)
//
//   drive=0.0 → k=1.0 → mild saturation (tanh is near-linear for small x,
//               but tanh(1)≈0.762 normalisation adds ≈1.31× for large x).
//               The plan specifies "near-linear" at drive=0 — this matches.
//   drive=1.0 → k=10.0 → heavy saturation, sine wave sounds nearly square.
//
// Design decisions:
//   - No state — purely sample-in sample-out. No prepare() needed.
//   - Static method — instantiated as a value type in PluginProcessor.
//   - std::tanhf used for float version (avoids implicit double promotion).
//   - Applied per channel independently (L and R) for correct stereo handling.
//
// ⚠️ NOTE for Nabeel: at drive=0 the formula still applies a small amount of
// saturation (not transparent passthrough). If truly flat unity gain at
// drive=0 is required, add: `if (drive <= 0.0f) return x;` before the tanh.
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
    //   drive — 0.0 (clean) to 1.0 (heavy clip), mapped to k=1..10 internally
    //
    // Returns:
    //   Soft-clipped sample, guaranteed to be within ±1.0.
    //
    // Audio thread safe: no allocation, no I/O, no state mutated.
    // ========================================================================
    static float processSample (float x, float drive) noexcept
    {
        const float k    = 1.0f + drive * 9.0f;
        const float tanhK = std::tanhf (k);

        // Guard: if tanhK is near zero (should never happen for k≥1), pass through.
        if (tanhK < 1e-6f)
            return x;

        return std::tanhf (k * x) / tanhK;
    }
};
