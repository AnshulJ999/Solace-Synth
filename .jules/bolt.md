## 2026-03-10 - Replace `std::pow(2.0, x)` with `std::exp2(x)` for frequency multipliers
**Learning:** `std::pow(2.0, x)` is slower compared to `std::exp2(x)` for base-2 exponentials. When computing tuning offsets from semitones and cents, replacing `std::pow(2.0, x)` with `std::exp2(x)` resulted in roughly a 2x performance improvement in benchmarks.
**Action:** When computing powers of 2 (e.g., converting pitch offsets like octaves, semitones, and cents to frequency multipliers), use `std::exp2(x)` instead of `std::pow(2.0, x)` to optimize calculations on the audio thread.
