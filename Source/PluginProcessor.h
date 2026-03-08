#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "SolaceLogger.h"

// ============================================================================
// Solace Synth — Audio Processor (DSP Engine)
//
// This is the heart of the plugin. It handles:
// - Audio processing (oscillators, filters, envelopes, etc.)
// - MIDI input
// - Parameter management via AudioProcessorValueTreeState
// - State save/load for DAW recall
//
// Currently: skeleton that passes silence. DSP will be added in later phases.
// ============================================================================

class SolaceSynthProcessor : public juce::AudioProcessor
{
public:
    SolaceSynthProcessor();
    ~SolaceSynthProcessor() override;

    // --- Audio Processing ---
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // --- Editor ---
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    // --- Plugin Info ---
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    // --- Programs (presets) ---
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    // --- State Persistence ---
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- Parameter Tree ---
    // All plugin parameters live here. This is the contract between DSP and UI.
    // The UI reads/writes parameters through this tree.
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

private:
    // Parameter tree — defines all automatable parameters
    juce::AudioProcessorValueTreeState apvts;

    // File-based logger — output goes to %TEMP%/SolaceSynth/ (trace.log, debug.log, info.log)
    std::unique_ptr<SolaceLogger> solaceLogger;

    // Creates the parameter layout (called once in constructor)
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SolaceSynthProcessor)
};
