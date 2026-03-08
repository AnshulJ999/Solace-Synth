#include "PluginEditor.h"

// ============================================================================
// Constructor
// ============================================================================
SolaceSynthEditor::SolaceSynthEditor (SolaceSynthProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // Title label — placeholder UI for Phase 3
    titleLabel.setText ("Solace Synth - WebView UI coming soon",
                        juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (24.0f));
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel);

    // Set initial window size
    // This will match the final UI dimensions later
    setSize (900, 600);
}

SolaceSynthEditor::~SolaceSynthEditor()
{
}

// ============================================================================
// Paint — draws the background
// ============================================================================
void SolaceSynthEditor::paint (juce::Graphics& g)
{
    // Dark background — similar to the final UI's dark mode option
    g.fillAll (juce::Colour (0xFF1a1a2e));

    // Version info in bottom-right corner
    g.setColour (juce::Colour (0xFF666666));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("v" VERSION, getLocalBounds().removeFromBottom (25).removeFromRight (80),
                juce::Justification::centredRight, false);
}

// ============================================================================
// Resized — positions child components
// ============================================================================
void SolaceSynthEditor::resized()
{
    titleLabel.setBounds (getLocalBounds());
}
