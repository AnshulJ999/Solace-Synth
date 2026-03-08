#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

// ============================================================================
// Solace Synth - Plugin Editor (WebView UI Host)
//
// Phase 4: Embeds a WebBrowserComponent that loads UI/index.html via
// a ResourceProvider. Implements a bidirectional C++ <-> JS bridge:
//
//   JS -> C++: Native functions (setParameter, uiReady, log)
//   C++ -> JS: Events (parameterChanged, syncAllParameters)
//
// Architecture:
//   - APVTS is the single source of truth for all parameters
//   - JS never "owns" state - it reflects and edits host state
//   - C++ validates and applies parameter changes from JS
//   - C++ pushes parameter changes (from automation/presets) to JS
// ============================================================================

class SolaceSynthEditor : public juce::AudioProcessorEditor,
                          private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit SolaceSynthEditor (SolaceSynthProcessor&);
    ~SolaceSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

private:
    SolaceSynthProcessor& processorRef;

    // --- WebView ---
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // --- Fallback UI (shown if WebView fails) ---
    juce::Label fallbackLabel;
    bool webViewReady = false;

    // --- Bridge: C++ -> JS ---
    void sendParameterToJS (const juce::String& paramId, float value);
    void sendAllParametersToJS();

    // --- Bridge: JS -> C++ (registered as native functions) ---
    void handleSetParameter (const juce::Array<juce::var>& args,
                             juce::WebBrowserComponent::NativeFunctionCompletion completion);
    void handleUiReady (const juce::Array<juce::var>& args,
                        juce::WebBrowserComponent::NativeFunctionCompletion completion);
    void handleLog (const juce::Array<juce::var>& args,
                    juce::WebBrowserComponent::NativeFunctionCompletion completion);

    // --- APVTS Listener (for automation/preset changes) ---
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // --- Resource Provider ---
    std::optional<juce::WebBrowserComponent::Resource> resourceRequested (const juce::String& path);

    // --- Helpers ---
    juce::WebBrowserComponent::Options createWebViewOptions();
    static std::optional<juce::WebBrowserComponent::Resource> loadFileAsResource (const juce::File& file);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SolaceSynthEditor)
};
