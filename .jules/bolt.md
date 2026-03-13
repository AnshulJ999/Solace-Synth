## 2024-05-18 - Faster base-2 exponentiation using std::exp2
**Learning:** `std::exp2(x)` is significantly faster (~2x) than `std::pow(2.0, x)` for base-2 exponential calculations in C++ DSP code, especially in audio threads where every cycle counts.
**Action:** Use `std::exp2(x)` instead of `std::pow(2.0, x)` when computing frequency multipliers from pitch offsets or semitones.