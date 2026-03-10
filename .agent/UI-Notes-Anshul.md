

These are UI notes/observations by Anshul. Based on comparison of Figma design and actual UI, and also personal preference.

1) The slider on the faders is too big and not aligned. It seems offset to the right; it is not center-aligned. Needs to be a little smaller and center-aligned. True for all faders. --- STILL NOT RESOLVED

2) Waveform, Octave, Transpose, and Tuning, are all in one row, which is why they don't fit. It should be more like

Waveform                              Tuning

Octave                                   (still tuning)

Transpose                               (still tuning)

Like one column on left has waveform, octave, and tranpsoe, and the right column has the tuning fader. 

This problem is for both Osc1 and Osc2. 

3) The ruler-type tick marks on the faders should be to the right-side of the faders. Currently they are on the faders themselves. They should be aligned to the right side. 

4) In the figma design, none of the faders have the value underneath them. However this is a serious design consideration, so for now we will keep the values. In future this may be a setting where we can optionally hide/show it. 

Also, we may opt to show the values as a tooltip when hovering over a fader.

5) All the stuff that was supposed to be a dropdown is a click to cycle instead. Everything under LFO and Voicing was supposed to be dropdowns naturally.