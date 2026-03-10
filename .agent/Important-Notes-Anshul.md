

There is NO Filter Env Depth in the design. It should NOT be there. 


---

  - **⚠️ OPEN DESIGN QUESTION — needs Nabeel (2026-03-11):**
    - **Velocity-to-level semantic:** Current behavior: when AmpLevel is NOT in either target slot, `velocityScale = velocity` (level always scales with MIDI velocity). When AmpLevel IS targeted, `velocityScale = lerp(1.0, velocity, velRange)`. This means velocity always affects level unless AmpLevel is targeted AND velocityRange=0. Codex/Claude Code flag this as inconsistent: the documented intent was that AmpLevel is the routing \"gate\" for level sensitivity. Anshul's preference: keep current behavior (feels intuitive). Nabeel should confirm the exact UX intent before V1 release.
    - **Velocity mod target list mismatch:** Vision Document (Nabeel, line 55-63) lists 8 velocity targets: Osc Pitch, Osc Tuning, Amp Attack, Filter Cutoff, Filter Resonance, Distortion, Master Volume, Osc Mix. Current implementation has only 4 (None, AmpLevel, AmpAttack, FilterCutoff, FilterResonance -- 5 enum values total). **The full list from the vision doc needs to be reconciled with the implementation.** This is a V1.x concern -- adding more targets is backward-compatible (APVTS int range extension). Flagged for Nabeel review.


---

