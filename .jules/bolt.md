## 2024-05-24 - Faster Exponentials with std::exp2
**Learning:** In the audio DSP thread, `std::exp2(x)` is significantly faster (~2x) than `std::pow(2.0, x)` for base-2 exponential calculations like frequency multipliers from pitch offsets.
**Action:** Always prefer `std::exp2(x)` over `std::pow(2.0, x)` when working with pitch offsets and frequency multipliers in DSP code.
