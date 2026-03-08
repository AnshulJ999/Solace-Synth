#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "SolaceSound.h"

// ============================================================================
// SolaceVoice — One Polyphonic Synthesiser Voice
//
// A SynthesiserVoice renders one note at a time. The Synthesiser manages
// a pool of these voices and assigns them to incoming MIDI note-on events.
//
// Phase 5 Implementation:
//   - Single sine wave oscillator (std::sin based, standard for clarity)
//   - Velocity-scaled amplitude
//   - Simple exponential tail-off on note release (avoids audible click)
//
// Architecture Notes:
//   - This runs on the AUDIO THREAD. No allocations, no locks, no logging.
//   - angleDelta == 0.0 is used as the "silent / not playing" flag.
//     The Synthesiser itself tracks voice state; we just render or don't.
//   - addSample() is used (not setSample()) so voices mix correctly when
//     multiple voices are sounding simultaneously (polyphony).
//   - startSample offset MUST be respected — Synthesiser can call
//     renderNextBlock() mid-buffer when a note arrives mid-block.
//
// Future: Replace std::sin with a wavetable for efficiency and waveform choice.
// ============================================================================
class SolaceVoice : public juce::SynthesiserVoice
{
public:
    SolaceVoice() = default;

    // ========================================================================
    // canPlaySound — tell the Synthesiser which sound types we handle
    // ========================================================================
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        // This voice only handles SolaceSound — not any other sound type.
        // Using dynamic_cast is the idiomatic JUCE pattern here.
        return dynamic_cast<SolaceSound*> (sound) != nullptr;
    }

    // ========================================================================
    // startNote — called by Synthesiser when a MIDI note-on arrives
    //
    // Receives the MIDI note number (0-127), velocity (0.0-1.0),
    // the sound descriptor, and the current pitch wheel position.
    // ========================================================================
    void startNote (int midiNoteNumber,
                    float velocity,
                    juce::SynthesiserSound* /*sound*/,
                    int /*currentPitchWheelPosition*/) override
    {
        // Reset phase on each note-on (clean attack, no phase accumulation)
        currentAngle = 0.0;

        // Scale level by velocity. 0.15 cap prevents clipping with many voices.
        // A full-scale sine wave at ±1.0 is extremely loud — 0.15 is safe.
        level = velocity * 0.15;

        // Reset tail-off — 0.0 means "key is held down"
        tailOff = 0.0;

        // Convert MIDI note number → frequency in Hz using equal temperament
        // A4 (MIDI 69) = 440 Hz. Each semitone = factor of 2^(1/12).
        double frequencyHz = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

        // Convert frequency to phase increment per sample
        double cyclesPerSample = frequencyHz / getSampleRate();
        angleDelta = cyclesPerSample * juce::MathConstants<double>::twoPi;
    }

    // ========================================================================
    // stopNote — called by Synthesiser when a MIDI note-off arrives
    //
    // allowTailOff: true = let the note fade naturally (normal)
    //               false = cut immediately (e.g. voice stolen)
    // ========================================================================
    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            // Begin exponential tail-off from current level
            // tailOff starts at 1.0 and decays per-sample in renderNextBlock
            if (tailOff == 0.0)
                tailOff = 1.0;
        }
        else
        {
            // Immediate cut (voice stealing, monophonic mode, noteOff with legato off)
            clearCurrentNote();
            angleDelta = 0.0;
        }
    }

    // ========================================================================
    // renderNextBlock — called each audio block to fill output buffer
    //
    // CRITICAL: Must use addSample() not setSample() — multiple voices mix.
    // CRITICAL: Must respect startSample offset — note may start mid-block.
    // CRITICAL: No allocations, no I/O, no locks on this thread.
    // ========================================================================
    void renderNextBlock (juce::AudioSampleBuffer& outputBuffer,
                          int startSample,
                          int numSamples) override
    {
        if (angleDelta == 0.0)
            return; // Voice is silent — nothing to render

        if (tailOff > 0.0)
        {
            // --- Release phase: exponential decay ---
            while (--numSamples >= 0)
            {
                auto currentSample = static_cast<float> (std::sin (currentAngle) * level * tailOff);

                // Mix into all output channels
                for (int ch = outputBuffer.getNumChannels(); --ch >= 0;)
                    outputBuffer.addSample (ch, startSample, currentSample);

                currentAngle += angleDelta;
                ++startSample;

                // Decay factor: 0.99 gives ~0.5s tail at 44.1kHz
                // Tunable later with an APVTS "release" parameter
                tailOff *= 0.99;

                if (tailOff <= 0.005)
                {
                    // Tail has decayed to inaudible — mark voice as done
                    clearCurrentNote();
                    angleDelta = 0.0;
                    break;
                }
            }
        }
        else
        {
            // --- Sustain phase: note is held, render at full level ---
            while (--numSamples >= 0)
            {
                auto currentSample = static_cast<float> (std::sin (currentAngle) * level);

                for (int ch = outputBuffer.getNumChannels(); --ch >= 0;)
                    outputBuffer.addSample (ch, startSample, currentSample);

                currentAngle += angleDelta;
                ++startSample;
            }
        }
    }

    // ========================================================================
    // Pitch wheel — not implemented in Phase 5, required override
    // ========================================================================
    void pitchWheelMoved (int /*newPitchWheelValue*/) override {}

    // ========================================================================
    // Controller — not implemented in Phase 5, required override
    // ========================================================================
    void controllerMoved (int /*controllerNumber*/, int /*newControllerValue*/) override {}

private:
    // Sine oscillator state — all doubles for precision at low frequencies
    double currentAngle  = 0.0;   // Current phase (radians, not bounded to 2pi)
    double angleDelta    = 0.0;   // Phase increment per sample (0.0 = silent)
    double level         = 0.0;   // Amplitude (0.0-0.15, velocity scaled)
    double tailOff       = 0.0;   // Release envelope (0.0=held, 1.0→0.0=releasing)
};
