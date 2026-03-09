#pragma once

#include <juce_dsp/juce_dsp.h>

// ============================================================================
// SolaceFilter — Per-Voice Subtractive Filter
//
// Wraps juce::dsp::LadderFilter<float> — a TPT (Topology-Preserving Transform)
// Moog-style ladder filter implementation. Chosen over StateVariableTPTFilter
// because LP24 (24 dB/oct) is required as the default: it is the signature
// sound of subtractive synthesis and is not available in StateVariableTPTFilter.
//
// Filter type mapping (matches APVTS "filterType" AudioParameterInt index):
//   0 = LP12 — Low-pass, 12 dB/oct, lighter slope
//   1 = LP24 — Low-pass, 24 dB/oct (default) — Moog-style, classic sound
//   2 = HP12 — High-pass, 12 dB/oct, thins bass
//
// Per-sample design:
//   LadderFilter exposes a public processSample(value, channelIndex) method.
//   We use channel 0 since each SolaceVoice processes mono internally (output
//   is then added to all buffer channels in renderNextBlock).
//   This allows per-sample cutoff modulation in Phase 6.4 (filter envelope)
//   by calling setCutoff() inside the renderNextBlock() sample loop.
//
// Phase 6.3 usage (static cutoff per note):
//   1. Call prepare(spec) from SolaceVoice::prepare().
//   2. Call setMode(), setResonance(), setCutoff() at note-on (startNote()).
//   3. Call processSample(input) per sample in renderNextBlock().
//   4. Call reset() on hard voice cut (stopNote with allowTailOff = false).
//
// Phase 6.4 usage (dynamic cutoff from filter envelope):
//   In the renderNextBlock() sample loop, compute modulatedCutoff from the
//   filter envelope value + depth + baseCutoff, then call setCutoff() before
//   processSample(). SolaceFilter is opaque to the modulation math — it just
//   forwards the cutoff to LadderFilter each sample.
//
// Thread safety:
//   Designed to run exclusively on the audio thread.
//   Do NOT share a single SolaceFilter instance across voices.
// ============================================================================

class SolaceFilter
{
public:
    SolaceFilter() = default;

    // ========================================================================
    // prepare — initialise the filter for the given sample rate and block size.
    //
    // Must be called before any other method. Call from SolaceVoice::prepare().
    // The spec is passed with numChannels overridden to 1 — each voice is mono.
    // Using the actual sampleRate and maximumBlockSize from the host spec
    // ensures correct internal LadderFilter allocation.
    // ========================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        juce::dsp::ProcessSpec monoSpec = spec;
        monoSpec.numChannels = 1;  // Per-voice filter processes mono
        ladder.prepare (monoSpec);
        ladder.reset();
    }

    // ========================================================================
    // setMode — select the filter type.
    //
    // Call at note-on after reading filterType from APVTS (AudioParameterInt,
    // stored as float, cast to int here). Invalid indices fall back to LP24.
    //
    //   0 → LPF12, 1 → LPF24 (default), 2 → HPF12
    // ========================================================================
    void setMode (int typeIndex) noexcept
    {
        switch (typeIndex)
        {
            case 0:  ladder.setMode (juce::dsp::LadderFilterMode::LPF12); break;
            case 2:  ladder.setMode (juce::dsp::LadderFilterMode::HPF12); break;
            default: ladder.setMode (juce::dsp::LadderFilterMode::LPF24); break;  // 1 + invalid
        }
    }

    // ========================================================================
    // setCutoff — set the filter cutoff frequency in Hz.
    //
    // Valid range [20, 20000] Hz. Input is clamped before forwarding to
    // LadderFilter to prevent undefined behaviour outside the valid range
    // (especially important when the filter envelope modulates the cutoff,
    // potentially pushing it past the edges in Phase 6.4).
    //
    // In Phase 6.3, called once at note-on.
    // In Phase 6.4, called per-sample with the modulated cutoff value.
    // ========================================================================
    void setCutoff (float hz) noexcept
    {
        ladder.setCutoffFrequencyHz (juce::jlimit (20.0f, 20000.0f, hz));
    }

    // ========================================================================
    // setResonance — set the filter resonance.
    //
    // Range [0.0, 1.0]. Values approaching 1.0 can cause self-oscillation
    // in the LadderFilter (that is intentional and musically useful). Clamped
    // to prevent undefined behaviour from out-of-range values.
    // ========================================================================
    void setResonance (float r) noexcept
    {
        ladder.setResonance (juce::jlimit (0.0f, 1.0f, r));
    }

    // ========================================================================
    // processSample — filter one sample and return the output.
    //
    // Uses LadderFilter's native per-sample path (channel 0, mono voice).
    // This avoids AudioBlock construction overhead while giving us the
    // per-sample control needed for filter envelope modulation in Phase 6.4.
    //
    // Call once per audio sample in renderNextBlock().
    // ========================================================================
    float processSample (float input) noexcept
    {
        return ladder.processSample (input, 0);  // channel 0 — mono per voice
    }

    // ========================================================================
    // reset — flush all internal filter state to zero.
    //
    // Call from stopNote(allowTailOff=false) to prevent transient pops when
    // a voice is stolen and immediately reused for a new note.
    // ========================================================================
    void reset() noexcept
    {
        ladder.reset();
    }

private:
    juce::dsp::LadderFilter<float> ladder;
};
