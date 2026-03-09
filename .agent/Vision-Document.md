# Solace Soft Synth

# Project Requirements \-

* Simple to use polyphonic (not paraphonic, so the voices will have individual filter envelopes, so that it sounds like a proper instrument) synthesizer, possibly multi-platform  
* Needs at least two oscillators with sin, saw and square waves, basic tuning, and octave shifts  
* An amplifier envelope (this will also work separately per voice, think of it like any normal instrument, when you pluck a string, it always starts with attack and goes through decay sustain and release subsequently depending on the nature of the instrument).  
* Filter types 12 and 24 dB low-pass, maybe high-pass too. The filter types allow for more diverse sound shaping, but 12 and 24 dB low-pass filters are the most common types, along with their High-pass counterparts.  
* Filter envelope. This will allow the effects of the filter to also follow the ADSR pattern as you press a note. For example, if you set the attack on this filter to be really slow, the note will start by sounding like it has no filter and will transition to the way the filter will morph it over the attack period. Need this one to be polyphonic as stated before.  
* LFO that can have targets to modulate. LFOs help modulate target parameters over time using a waveform. We can use sine, saw, and square waves to get different changes in the sound over time. The rate determines the speed of change over time, so the frequency of the filter, and the amount determines the amplitude.  
* Unison. Unison is like a chorus, but you only have to tell it to add more voices, it automatically pans them left and right. So if you choose to have 5 voices in unison, it will play 5 voices for each note, each new note being panned slightly harder than the last, and to the opposite side of the last one.  
* Velocity sensitivity and modulation, I want this to be a player’s synth  
* Computer friendly UI  
* Midi compatibility and virtual keyboard

# Nice to haves \-

* Settings that allow more configuration, such as number of voice (to limit CPU use for mobile devices)  
* Portamento  
* Preset library and some factory presets

# Possible platforms

* JUCE  
* Audio Plugin Coder (APC)

# Roles

**Anshul:**

* Backend vibe-coding  
* Frontend vibe-coding \- importing the Figma design into the code

**Nabeel:**

* Figma UI UX Design  
* JUCE Code Review

# Open Questions / Gaps

1\. \*\*NATS font\*\* — Is this a free font? Where is the font file? Check with Nabeel. \*(Acknowledged — will be resolved)\*  
2\. \*\*Waveform variants\*\* — Design currently has Sine \+ Square only. Will Sawtooth and Triangle be added? \*(Acknowledged — may be added)\*  
3\. \*\*Slider states\*\* — "Big1", "Big2", "Mid", "High", "0", "Small", "Small1", "Small2" — what are the exact pixel track heights for each variant?  
4\. \*\*Filter type options\*\* — The selector shows "LP 24 dB". What are all the filter type options? (LP12, LP24, HP12, HP24, BP?)  
5\. \*\*LFO Target options\*\* — What are all the possible dropdown values? (Full list for JS implementation)  
6\. \*\*Keyboard: CSS vs JUCE native\*\* — See Row 3 section above. Decision pending.  
7\. \*\*Pitch Bend / Mod Wheel\*\* — Confirmed present in design (left of keyboard). Are these MIDI performance controls only, or also mapped to synth parameters?

**Answers \-**

1. We can just use Jura, too many fonts will make it look messy anyways, let’s not take risks with it  
2. We will need at least sawtooth, any synth will be incomplete without it, in fact it’s the most used waveform for lead sounds  
3. There are only three sizes of sliders, Big, normal, and small, the rest are just variations that I was using to illustrate the design and how it might look. That said, the default values for the different parameters will vary. For example Tuning for the oscillators can be varied in both directions, so the default value will be in the middle similarly for pitch-bend and oscillator mix, while sustain might start out at full or else the notes will not sustain forever as expected by default.  
4. Yes, those will do nicely for all the filter type options. They are the usual 5 suspects, often Band Pass is omitted though, if it’s trouble, then we don’t need it.  
5. Okay, for target options, we need to have as many as possible, we can have the same list for velocity modulation targets as well \-  
   1. Oscillator pitch for both  
   2. Oscillator tuning  
   3. Amplifier Attack  
   4. Filter Cutoff  
   5. Filter Resonance  
   6. Distortion  
   7. Master volume  
   8. Oscillator mix  
6. Stick to JUCE native for now and discuss CSS later.  
7. Pitch-bend will work as expected on the synth’s pitch (globally). Modulation however can be another feature to allow one slide to affect multiple things, we can have it be configurable by right-clicking on it, and selecting any parameter to be affected by it’s value as the user drags it.

# UI Tryouts

* UI with knobs did not work out since folks complained about dragging knobs not being intuitive   
* The second iteration with sliders appears more viable. Tried to create some branding and manage the colors better. Not final at all.  
* The third iteration added the keyboard, pitch-bend, modulation, menu button, and preset browser.