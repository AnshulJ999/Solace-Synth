#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

// ============================================================================
// Solace Synth — Plugin Editor (UI Host)
//
// This is the plugin's GUI window. Currently shows a simple label.
// In Phase 4, this will be replaced with a WebView component that loads
// the HTML/CSS/JS frontend from UI/index.html.
//
// The editor communicates with the processor via the APVTS parameter tree.
// ============================================================================

class SolaceSynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit SolaceSynthEditor (SolaceSynthProcessor&);
    ~SolaceSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SolaceSynthProcessor& processorRef;

    // Placeholder label — will be replaced by WebView in Phase 4
    juce::Label titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SolaceSynthEditor)
};
