#include "PluginEditor.h"
#include <juce_core/juce_core.h>

// ============================================================================
// Constructor
// ============================================================================
SolaceSynthEditor::SolaceSynthEditor (SolaceSynthProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      // MidiKeyboardComponent needs the MidiKeyboardState (from Processor)
      // and the layout orientation.
      midiKeyboard (p.getMidiKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    // --- Fallback label (shown while WebView loads or if it fails) ---
    fallbackLabel.setText ("Loading Solace Synth UI...",
                           juce::dontSendNotification);
    fallbackLabel.setFont (juce::FontOptions (20.0f));
    fallbackLabel.setJustificationType (juce::Justification::centred);
    fallbackLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (fallbackLabel);

    // --- Create WebView with our bridge configuration ---
    auto options = createWebViewOptions();

    if (juce::WebBrowserComponent::areOptionsSupported (options))
    {
        webView = std::make_unique<juce::WebBrowserComponent> (options);
        addAndMakeVisible (*webView);

        // Navigate to the resource provider root (serves our UI/index.html)
        webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    }
    else
    {
        // WebView not supported - show fallback
        fallbackLabel.setText ("WebView is not available on this system.\n"
                               "Please install Microsoft Edge WebView2 Runtime.",
                               juce::dontSendNotification);
    }

    // --- Register as APVTS listener for all parameters ---
    // This fires parameterChanged() when automation or presets change values
    auto& apvts = processorRef.getAPVTS();
    auto params = apvts.processor.getParameters();
    for (auto* param : params)
    {
        if (auto* paramWithID = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
            apvts.addParameterListener (paramWithID->getParameterID(), this);
    }

    // --- On-screen MIDI keyboard ---
    // Scroll to show octave 3-5 (middle range) by default
    midiKeyboard.setAvailableRange (36, 96);   // C2 to C6
    midiKeyboard.setScrollButtonsVisible (true);
    addAndMakeVisible (midiKeyboard);
    // Grab keyboard focus after a short delay so the user can play immediately.
    // We use a lambda timer so we don't block the constructor.
    juce::Timer::callAfterDelay (500, [this]() {
        midiKeyboard.grabKeyboardFocus();
    });

    // Set window size
    setSize (900, 600);
}

// ============================================================================
// Destructor
// ============================================================================
SolaceSynthEditor::~SolaceSynthEditor()
{
    // Remove APVTS listeners FIRST to prevent any new async callbacks
    auto& apvts = processorRef.getAPVTS();
    auto params = apvts.processor.getParameters();
    for (auto* param : params)
    {
        if (auto* paramWithID = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
            apvts.removeParameterListener (paramWithID->getParameterID(), this);
    }
}

// ============================================================================
// WebView Options - configures the browser and the C++ <-> JS bridge
// ============================================================================
juce::WebBrowserComponent::Options SolaceSynthEditor::createWebViewOptions()
{
    using Options = juce::WebBrowserComponent::Options;

    return Options()
        .withBackend (Options::Backend::webview2)
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withNativeIntegrationEnabled()

        // WebView2-specific: set user data folder to temp to avoid permission
        // issues in VST3 hosts that restrict filesystem access
        .withWinWebView2Options (
            Options::WinWebView2()
                .withUserDataFolder (juce::File::getSpecialLocation (
                    juce::File::tempDirectory).getChildFile ("SolaceSynth_WebView2"))
                .withStatusBarDisabled()
                .withBackgroundColour (juce::Colour (0xFFf5f5f5))
        )

        // Resource provider: serves files from our UI/ directory
        .withResourceProvider ([this] (const juce::String& path)
        {
            return resourceRequested (path);
        })

        // --- Native functions (JS -> C++) ---

        // setParameter(paramId, value) - JS tells C++ to change a parameter
        .withNativeFunction ("setParameter",
            [this] (const juce::Array<juce::var>& args,
                    juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                handleSetParameter (args, std::move (completion));
            })

        // uiReady() - JS signals that the page has loaded and is ready
        .withNativeFunction ("uiReady",
            [this] (const juce::Array<juce::var>& args,
                    juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                handleUiReady (args, std::move (completion));
            })

        // log(message) - JS can send debug messages to C++ console
        .withNativeFunction ("log",
            [this] (const juce::Array<juce::var>& args,
                    juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                handleLog (args, std::move (completion));
            });
}

// ============================================================================
// Resource Provider - serves UI files
//
// DEV MODE:  Uses SOLACE_DEV_UI_PATH (compile-time absolute path to UI/)
//            Works in both standalone and VST3 hosts (path doesn't depend
//            on the running executable's location).
//
// PRODUCTION: Will eventually use juce_add_binary_data() to embed files.
//             The ResourceProvider lambda stays the same, just the data
//             source changes from disk to BinaryData::.
// ============================================================================
std::optional<juce::WebBrowserComponent::Resource>
SolaceSynthEditor::resourceRequested (const juce::String& path)
{
    // Use the compile-time dev path (defined in CMakeLists.txt)
    // This is an absolute path like "G:/GitHub/Solace-Synth/UI"
    juce::File uiDir (SOLACE_DEV_UI_PATH);

    if (! uiDir.isDirectory())
    {
        DBG ("Solace Synth: UI/ directory not found at: " + uiDir.getFullPathName());
        return std::nullopt;
    }

    // Map the request path to a file
    juce::String filePath = path;
    if (filePath == "/" || filePath.isEmpty())
        filePath = "/index.html";

    // Remove leading slash
    if (filePath.startsWith ("/"))
        filePath = filePath.substring (1);

    auto file = uiDir.getChildFile (filePath);

    if (! file.existsAsFile())
    {
        DBG ("Solace Synth: Resource not found: " + filePath);
        return std::nullopt;
    }

    return loadFileAsResource (file);
}

// ============================================================================
// Load a file from disk and return it as a WebView Resource
// ============================================================================
std::optional<juce::WebBrowserComponent::Resource>
SolaceSynthEditor::loadFileAsResource (const juce::File& file)
{
    // Determine MIME type from extension
    auto ext = file.getFileExtension().toLowerCase();
    juce::String mimeType = "application/octet-stream";

    if      (ext == ".html")                    mimeType = "text/html";
    else if (ext == ".css")                     mimeType = "text/css";
    else if (ext == ".js")                      mimeType = "application/javascript";
    else if (ext == ".json")                    mimeType = "application/json";
    else if (ext == ".png")                     mimeType = "image/png";
    else if (ext == ".jpg" || ext == ".jpeg")   mimeType = "image/jpeg";
    else if (ext == ".svg")                     mimeType = "image/svg+xml";
    else if (ext == ".woff")                    mimeType = "font/woff";
    else if (ext == ".woff2")                   mimeType = "font/woff2";
    else if (ext == ".ttf")                     mimeType = "font/ttf";

    // Read file contents
    juce::MemoryBlock mb;
    if (! file.loadFileAsData (mb))
    {
        DBG ("Solace Synth: Failed to read file: " + file.getFullPathName());
        return std::nullopt;
    }

    // Convert to std::vector<std::byte>
    std::vector<std::byte> data (mb.getSize());
    std::memcpy (data.data(), mb.getData(), mb.getSize());

    return juce::WebBrowserComponent::Resource { std::move (data), mimeType };
}

// ============================================================================
// Bridge: JS -> C++ handlers
// ============================================================================

// Called when JS sends: setParameter("masterVolume", 0.75)
void SolaceSynthEditor::handleSetParameter (
    const juce::Array<juce::var>& args,
    juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    if (args.size() >= 2)
    {
        auto paramId = args[0].toString();
        auto value = static_cast<float> (args[1]);

        auto* param = processorRef.getAPVTS().getParameter (paramId);
        if (param != nullptr)
        {
            auto normalized = param->convertTo0to1 (value);
#if SOLACE_LOGGING_ENABLED || JUCE_DEBUG
            SolaceLog::trace ("setParameter: " + paramId
                + " rawValue=" + juce::String (value, 4)
                + " normalized=" + juce::String (normalized, 4));
#endif

            param->setValueNotifyingHost (normalized);
            completion (juce::var (true));
        }
        else
        {
#if SOLACE_LOGGING_ENABLED || JUCE_DEBUG
            SolaceLog::error ("setParameter: unknown param '" + paramId + "'");
#endif
            completion (juce::var (false));
        }
    }
    else
    {
#if SOLACE_LOGGING_ENABLED || JUCE_DEBUG
        SolaceLog::error ("setParameter: wrong arg count (" + juce::String (args.size()) + ")");
#endif
        completion (juce::var (false));
    }
}

// Called when JS signals that the page has loaded
void SolaceSynthEditor::handleUiReady (
    const juce::Array<juce::var>& /*args*/,
    juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
#if SOLACE_LOGGING_ENABLED || JUCE_DEBUG
    SolaceLog::info ("UI ready signal received from JS");
#endif
    webViewReady = true;

    // Hide fallback, show WebView
    fallbackLabel.setVisible (false);

    // Send the current state of all parameters to JS
    sendAllParametersToJS();

#if SOLACE_LOGGING_ENABLED || JUCE_DEBUG
    SolaceLog::info ("UI ready complete - all parameters synced to JS");
#endif
    completion (juce::var (true));
}

// Called when JS sends a debug log message
void SolaceSynthEditor::handleLog (
    const juce::Array<juce::var>& args,
    juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
#if SOLACE_LOGGING_ENABLED || JUCE_DEBUG
    if (! args.isEmpty())
    {
        SolaceLog::debug ("[JS] " + args[0].toString());
    }
#else
    juce::ignoreUnused (args);
#endif

    completion (juce::var (true));
}

// ============================================================================
// Bridge: C++ -> JS
// ============================================================================

// Push a single parameter value to JS
void SolaceSynthEditor::sendParameterToJS (const juce::String& paramId, float value)
{
    if (webView == nullptr || ! webViewReady)
        return;

#if SOLACE_LOGGING_ENABLED || JUCE_DEBUG
    SolaceLog::trace ("C++->JS parameterChanged: " + paramId
        + " value=" + juce::String (value, 4));
#endif

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("paramId", paramId);
    obj->setProperty ("value", value);

    webView->emitEventIfBrowserIsVisible ("parameterChanged", juce::var (obj));
}

// Push ALL parameter values to JS (called on uiReady, preset load, and visibility regain)
void SolaceSynthEditor::sendAllParametersToJS()
{
    if (webView == nullptr || ! webViewReady)
        return;

    auto& apvts = processorRef.getAPVTS();

    // Build an array of { paramId, value } objects on the stack (no heap allocation)
    juce::Array<juce::var> paramsArray;

    for (auto* param : apvts.processor.getParameters())
    {
        if (auto* paramWithID = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
        {
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("paramId", paramWithID->getParameterID());

            // Get the denormalized (real-world) value
            auto* rangedParam = apvts.getParameter (paramWithID->getParameterID());
            if (rangedParam != nullptr)
            {
                obj->setProperty ("value", rangedParam->convertFrom0to1 (rangedParam->getValue()));
            }
            else
            {
                obj->setProperty ("value", param->getValue());
            }

            paramsArray.add (juce::var (obj));
        }
    }

    webView->emitEventIfBrowserIsVisible ("syncAllParameters", juce::var (paramsArray));
}

// ============================================================================
// APVTS Listener - fires when parameters change from automation/presets/host
// ============================================================================
void SolaceSynthEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    // IMPORTANT: This callback runs on the AUDIO THREAD.
    // No disk I/O, no allocations, no locks here.
    // Bounce everything to the message thread via callAsync.
    auto safeThis = juce::Component::SafePointer<SolaceSynthEditor> (this);

    juce::MessageManager::callAsync ([safeThis, parameterID, newValue]()
    {
        if (auto* self = safeThis.getComponent())
        {
#if SOLACE_LOGGING_ENABLED || JUCE_DEBUG
            SolaceLog::trace ("APVTS parameterChanged: " + parameterID
                + " newValue=" + juce::String (newValue, 4));
#endif
            self->sendParameterToJS (parameterID, newValue);
        }
    });
}

// ============================================================================
// Visibility Changed - resync parameters when editor becomes visible again
//
// Why: emitEventIfBrowserIsVisible() drops events when hidden, so if
// automation/presets change parameters while the editor is hidden, the JS
// UI misses those updates. We fix this by pushing all current values when
// the editor becomes visible again.
// ============================================================================
void SolaceSynthEditor::visibilityChanged()
{
    if (isVisible() && webViewReady)
        sendAllParametersToJS();
}

// ============================================================================
// Paint - background behind WebView (visible during load)
// ============================================================================
void SolaceSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFFf5f5f5));  // Light background matching the UI design
}

// ============================================================================
// Resized - fill the entire window with the WebView
// ============================================================================
void SolaceSynthEditor::resized()
{
    auto bounds = getLocalBounds();

    // Reserve the bottom strip for the MIDI keyboard
    auto keyboardBounds = bounds.removeFromBottom (keyboardHeight);
    midiKeyboard.setBounds (keyboardBounds);

    // Remaining area goes to the WebView / fallback
    fallbackLabel.setBounds (bounds);

    if (webView != nullptr)
        webView->setBounds (bounds);
}
