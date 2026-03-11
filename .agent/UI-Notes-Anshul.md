

These are UI notes/observations by Anshul. Based on comparison of Figma design and actual UI, and also personal preference.

1) The slider on the faders is too big and not aligned. It seems offset to the right; it is not center-aligned. Needs to be a little smaller and center-aligned. True for all faders. --- RESOLVED

2) RESOLVED - Waveform, Octave, Transpose, and Tuning, are all in one row, which is why they don't fit. It should be more like

Waveform                              Tuning

Octave                                   (still tuning)

Transpose                               (still tuning)

Like one column on left has waveform, octave, and tranpsoe, and the right column has the tuning fader. 

This problem is for both Osc1 and Osc2. 

3) RESOLVED - The ruler-type tick marks on the faders should be to the right-side of the faders. Currently they are on the faders themselves. They should be aligned to the right side. 

4) In the figma design, none of the faders have the value underneath them. However this is a serious design consideration, so for now we will keep the values. In future this may be a setting where we can optionally hide/show it. 

Also, we may opt to show the values as a tooltip when hovering over a fader.

5) All the stuff that was supposed to be a dropdown is a click to cycle instead. Everything under LFO and Voicing was supposed to be dropdowns naturally.

6) The sizing and placement of the faders is awkward and not perfect. Needs refinement. Some faders need to be bigger. 

7) We need precise control over values of faders. Right now it's a bit finicky. Ideas: 

A modifier key like Ctrl + Dragging the fader should allow precise control over values. Slower value adjustment basically. 

Being able to double-click the value and becomes a text box where we can just type the actual value. 

Keyboard arrow keys to control value when the fader was touched/focused. This might be complex so have to discuss.

8) There is no way to go back to the 'Default' value of a setting. Need a gesture for that. Perhaps double-clicking on a fader thumb puts it back to default.

For example, It would be nice if we can just double-click on OscMix and get back to 50-50 mix of both Oscillators

There's other such small UI UX things that professional plugins have and we need to think about and add.

9) The tick marks are not properly aligned to the faders. They look offset. Like they are starting below the fader. It's not vertically aligned. 

Also the fader size is not good.