## 2024-05-24 - Optimizing base-2 exponentials
**Learning:** `std::exp2(x)` is significantly faster than `std::pow(2.0, x)` for base-2 exponential calculations in the audio DSP thread. This provides measurable performance benefits, up to ~2x faster.
**Action:** Always prefer `std::exp2(x)` over `std::pow(2.0, x)` for base-2 exponential calculations such as frequency multipliers from pitch offsets in audio code.
