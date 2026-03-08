#include "PluginEditor.h"
#include <juce_core/juce_core.h>

// ============================================================================
// Constructor
// ============================================================================
SolaceSynthEditor::SolaceSynthEditor (SolaceSynthProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // --- Fallback label (shown while WebView loads or if it fails) ---
    fallbackLabel.setText ("Loading Solace Synth UI...",
                           juce::dontSendNotification);
    fallbackLabel.setFont (juce::FontOptions (20.0f));
    fallbackLabel.setJustificationType (juce::Justification::centred);
    fallbackLabel.setColour (juce::Label::textColourId, juce::Colours::white);
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
        // WebView not supported — show fallback
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

    // Set window size
    setSize (900, 600);
}

// ============================================================================
// Destructor
// ============================================================================
SolaceSynthEditor::~SolaceSynthEditor()
{
    // Remove APVTS listeners
    auto& apvts = processorRef.getAPVTS();
    auto params = apvts.processor.getParameters();
    for (auto* param : params)
    {
        if (auto* paramWithID = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
            apvts.removeParameterListener (paramWithID->getParameterID(), this);
    }
}

// ============================================================================
// WebView Options — configures the browser and the C++ <-> JS bridge
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

        // setParameter(paramId, value) — JS tells C++ to change a parameter
        .withNativeFunction ("setParameter",
            [this] (const juce::Array<juce::var>& args,
                    juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                handleSetParameter (args, std::move (completion));
            })

        // uiReady() — JS signals that the page has loaded and is ready
        .withNativeFunction ("uiReady",
            [this] (const juce::Array<juce::var>& args,
                    juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                handleUiReady (args, std::move (completion));
            })

        // log(message) — JS can send debug messages to C++ console
        .withNativeFunction ("log",
            [this] (const juce::Array<juce::var>& args,
                    juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                handleLog (args, std::move (completion));
            });
}

// ============================================================================
// Resource Provider — serves UI files from the project's UI/ directory
// ============================================================================
std::optional<juce::WebBrowserComponent::Resource>
SolaceSynthEditor::resourceRequested (const juce::String& path)
{
    // The UI files live in UI/ relative to the executable
    // In a dev build, the exe is deep in build/SolaceSynth_artefacts/...
    // We walk up to find the project root containing the UI/ folder

    auto exeDir = juce::File::getSpecialLocation (
        juce::File::currentExecutableFile).getParentDirectory();

    // Walk up to find UI/ directory (handles nested build output paths)
    juce::File uiDir;
    auto searchDir = exeDir;
    for (int i = 0; i < 8; ++i)
    {
        auto candidate = searchDir.getChildFile ("UI");
        if (candidate.isDirectory())
        {
            uiDir = candidate;
            break;
        }
        searchDir = searchDir.getParentDirectory();
    }

    if (! uiDir.isDirectory())
    {
        DBG ("Solace Synth: UI/ directory not found! Searched from: " + exeDir.getFullPathName());
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

    if (ext == ".html") mimeType = "text/html";
    else if (ext == ".css")  mimeType = "text/css";
    else if (ext == ".js")   mimeType = "application/javascript";
    else if (ext == ".json") mimeType = "application/json";
    else if (ext == ".png")  mimeType = "image/png";
    else if (ext == ".jpg" || ext == ".jpeg") mimeType = "image/jpeg";
    else if (ext == ".svg")  mimeType = "image/svg+xml";
    else if (ext == ".woff") mimeType = "font/woff";
    else if (ext == ".woff2") mimeType = "font/woff2";
    else if (ext == ".ttf")  mimeType = "font/ttf";

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
            // setValueNotifyingHost normalizes the value to 0-1 range
            // But our parameters are already in their natural range,
            // so we need to convert to normalized
            param->setValueNotifyingHost (
                param->convertTo0to1 (value));

            completion (juce::var (true));
        }
        else
        {
            DBG ("Solace Synth: Unknown parameter: " + paramId);
            completion (juce::var (false));
        }
    }
    else
    {
        DBG ("Solace Synth: setParameter requires 2 args (paramId, value)");
        completion (juce::var (false));
    }
}

// Called when JS signals that the page has loaded
void SolaceSynthEditor::handleUiReady (
    const juce::Array<juce::var>& /*args*/,
    juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    DBG ("Solace Synth: UI is ready");
    webViewReady = true;

    // Hide fallback, show WebView
    fallbackLabel.setVisible (false);

    // Send the current state of all parameters to JS
    sendAllParametersToJS();

    completion (juce::var (true));
}

// Called when JS sends a debug log message
void SolaceSynthEditor::handleLog (
    const juce::Array<juce::var>& args,
    juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    if (! args.isEmpty())
        DBG ("Solace Synth [JS]: " + args[0].toString());

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

    // Create a JSON object: { paramId: "masterVolume", value: 0.8 }
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("paramId", paramId);
    obj->setProperty ("value", value);

    webView->emitEventIfBrowserIsVisible ("parameterChanged", juce::var (obj));
}

// Push ALL parameter values to JS (called on uiReady and after preset load)
void SolaceSynthEditor::sendAllParametersToJS()
{
    if (webView == nullptr || ! webViewReady)
        return;

    auto& apvts = processorRef.getAPVTS();

    // Build an array of { paramId, value } objects
    auto* paramsArray = new juce::Array<juce::var>();

    for (auto* param : apvts.processor.getParameters())
    {
        if (auto* paramWithID = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
        {
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("paramId", paramWithID->getParameterID());

            // Get the denormalized (real-world) value
            auto* rangedParam = apvts.getParameter (paramWithID->getParameterID());
            if (rangedParam != nullptr)
                obj->setProperty ("value", rangedParam->convertFrom0to1 (rangedParam->getValue()));
            else
                obj->setProperty ("value", param->getValue());

            paramsArray->add (juce::var (obj));
        }
    }

    webView->emitEventIfBrowserIsVisible ("syncAllParameters", juce::var (*paramsArray));

    delete paramsArray;
}

// ============================================================================
// APVTS Listener — fires when parameters change from automation/presets/host
// ============================================================================
void SolaceSynthEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    // This is called from the audio thread — we must bounce to the message thread
    juce::MessageManager::callAsync ([this, parameterID, newValue]()
    {
        sendParameterToJS (parameterID, newValue);
    });
}

// ============================================================================
// Paint — background behind WebView (visible during load)
// ============================================================================
void SolaceSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFFf5f5f5));  // Light background matching the UI design
}

// ============================================================================
// Resized — fill the entire window with the WebView
// ============================================================================
void SolaceSynthEditor::resized()
{
    auto bounds = getLocalBounds();

    fallbackLabel.setBounds (bounds);

    if (webView != nullptr)
        webView->setBounds (bounds);
}
