




Can we move the JUCE keyboard to the right side? And add a pitch bend / mod wheel? Both pb and mod. 

Need the rest of the stuff. Presets, settings, etc. 

Also, the settings persist. Where are the settings stored ? 




===


There is NO Filter Env Depth in the design. It should NOT be there. 


---

  - **⚠️ OPEN DESIGN QUESTION — needs Nabeel (2026-03-11):**
    - **Velocity-to-level semantic:** Current behavior: when AmpLevel is NOT in either target slot, `velocityScale = velocity` (level always scales with MIDI velocity). When AmpLevel IS targeted, `velocityScale = lerp(1.0, velocity, velRange)`. This means velocity always affects level unless AmpLevel is targeted AND velocityRange=0. Codex/Claude Code flag this as inconsistent: the documented intent was that AmpLevel is the routing \"gate\" for level sensitivity. Anshul's preference: keep current behavior (feels intuitive). Nabeel should confirm the exact UX intent before V1 release.
    - **Velocity mod target list mismatch:** Vision Document (Nabeel, line 55-63) lists 8 velocity targets: Osc Pitch, Osc Tuning, Amp Attack, Filter Cutoff, Filter Resonance, Distortion, Master Volume, Osc Mix. Current implementation has only 4 (None, AmpLevel, AmpAttack, FilterCutoff, FilterResonance -- 5 enum values total). **The full list from the vision doc needs to be reconciled with the implementation.** This is a V1.x concern -- adding more targets is backward-compatible (APVTS int range extension). Flagged for Nabeel review.


---

Nabeel said: 

[11:32 am, 11/03/2026] Nabeel Main: Let's have it be assigned by default to amp level, and leave the decision to the user if they want to replace amp level with anything else, also there are two options for mod targets, so the user still has a slot available after amp level takes up one slot
[11:33 am, 11/03/2026] Nabeel Main: 2) In the original vision doc you listed 8 velocity modulation targets: Osc Pitch, Osc Tuning, Amp Attack, Filter Cutoff, Filter Resonance, Distortion, Master Volume, and Osc Mix. We currently have 5 implemented: None, Amp Level, Amp Attack, Filter Cutoff, Filter Resonance. Do you need all 8 for V1, or is the subset fine to ship and we add the rest (Distortion, Master Volume, Osc Mix, Osc Tuning) later?
This subset is fine for now, but distortion might be a great addition, the other 3 we can omit for now