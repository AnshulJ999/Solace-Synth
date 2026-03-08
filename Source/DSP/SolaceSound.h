#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

// ============================================================================
// SolaceSound — Synthesiser Sound Descriptor
//
// A SynthesiserSound describes *what kind of sounds* the synth can play.
// It doesn't hold audio data itself — it's a tag that tells the Synthesiser
// which voices can play which sounds.
//
// For Phase 5: we play all notes on all MIDI channels. One sound type.
// Future: multiple SolaceSounds could represent different oscillator types,
// sample zones, or keyboard splits.
// ============================================================================
struct SolaceSound : public juce::SynthesiserSound
{
    SolaceSound() = default;

    // This sound plays on every MIDI note number
    bool appliesToNote (int /*midiNoteNumber*/) override { return true; }

    // This sound plays on every MIDI channel
    bool appliesToChannel (int /*midiChannel*/) override { return true; }
};
