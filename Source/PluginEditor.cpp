#include "PluginEditor.h"
#include <SolaceUIBinaryData.h>
#include <juce_core/juce_core.h>

namespace
{
juce::String normaliseResourcePath (juce::String path)
{
    if (path == "/" || path.isEmpty())
        path = "/index.html";

    if (path.startsWith ("/"))
        path = path.substring (1);

    return juce::URL::removeEscapeChars (path);
}

juce::String getMimeTypeForPath (const juce::String& path)
{
    auto ext = juce::File (path).getFileExtension().toLowerCase();

    if      (ext == ".html")                    return "text/html";
    else if (ext == ".css")                     return "text/css";
    else if (ext == ".js")                      return "application/javascript";
    else if (ext == ".json")                    return "application/json";
    else if (ext == ".png")                     return "image/png";
    else if (ext == ".jpg" || ext == ".jpeg")   return "image/jpeg";
    else if (ext == ".svg")                     return "image/svg+xml";
    else if (ext == ".woff")                    return "font/woff";
    else if (ext == ".woff2")                   return "font/woff2";
    else if (ext == ".ttf")                     return "font/ttf";

    return "application/octet-stream";
}

std::optional<juce::WebBrowserComponent::Resource> makeResourceFromMemory (
    const void* data,
    size_t size,
    const juce::String& mimeType)
{
    if (data == nullptr || size == 0)
        return std::nullopt;

    std::vector<std::byte> bytes (size);
    std::memcpy (bytes.data(), data, size);

    return juce::WebBrowserComponent::Resource { std::move (bytes), mimeType };
}

std::optional<juce::WebBrowserComponent::Resource> loadEmbeddedResource (const juce::String& requestedPath)
{
    const auto fileName = juce::File (requestedPath).getFileName();

    for (int i = 0; i < SolaceUIBinaryData::namedResourceListSize; ++i)
    {
        const auto originalFileName = juce::String (SolaceUIBinaryData::originalFilenames[i]);

        if (! originalFileName.equalsIgnoreCase (fileName))
            continue;

        int dataSize = 0;
        const auto* data = SolaceUIBinaryData::getNamedResource (SolaceUIBinaryData::namedResourceList[i], dataSize);

        return makeResourceFromMemory (data,
                                       static_cast<size_t> (dataSize),
                                       getMimeTypeForPath (fileName));
    }

    return std::nullopt;
}
} // namespace

// ============================================================================
// Constructor
// ============================================================================
SolaceSynthEditor::SolaceSynthEditor (SolaceSynthProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      // MidiKeyboardComponent needs the MidiKeyboardState (from Processor)
      // and the layout orientation.
      midiKeyboard (p.getMidiKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    SolaceLog::info ("Editor ctor: entered");

    // --- Fallback label (shown while WebView loads or if it fails) ---
    fallbackLabel.setText ("Loading Solace Synth UI...",
                           juce::dontSendNotification);
    fallbackLabel.setFont (juce::FontOptions (20.0f));
    fallbackLabel.setJustificationType (juce::Justification::centred);
    fallbackLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (fallbackLabel);

    // --- Create WebView with our bridge configuration ---
    SolaceLog::debug ("Editor ctor: creating WebView options");
    auto options = createWebViewOptions();
    SolaceLog::debug ("Editor ctor: WebView options created");

    const auto optionsSupported = juce::WebBrowserComponent::areOptionsSupported (options);
    SolaceLog::info ("Editor ctor: WebView options supported = " + juce::String (optionsSupported ? "true" : "false"));

    if (optionsSupported)
    {
        SolaceLog::debug ("Editor ctor: constructing WebView component");
        webView = std::make_unique<juce::WebBrowserComponent> (options);
        SolaceLog::debug ("Editor ctor: WebView component constructed");
        addAndMakeVisible (*webView);
        SolaceLog::debug ("Editor ctor: WebView made visible");

        // Navigate to the resource provider root (serves our UI/index.html)
        SolaceLog::info ("Editor ctor: navigating WebView to resource provider root");
        webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
        SolaceLog::debug ("Editor ctor: goToURL returned");
    }
    else
    {
        // WebView not supported - show fallback
        SolaceLog::error ("Editor ctor: WebView options not supported on this system");
        fallbackLabel.setText ("WebView is not available on this system.\n"
                               "Please install Microsoft Edge WebView2 Runtime.",
                               juce::dontSendNotification);
    }

    // --- Register as APVTS listener for all parameters ---
    // This fires parameterChanged() when automation or presets change values
    auto& apvts = processorRef.getAPVTS();
    auto params = apvts.processor.getParameters();
    int listenerCount = 0;
    for (auto* param : params)
    {
        if (auto* paramWithID = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
        {
            apvts.addParameterListener (paramWithID->getParameterID(), this);
            ++listenerCount;
        }
    }
    SolaceLog::debug ("Editor ctor: APVTS listeners registered = " + juce::String (listenerCount));

    // --- On-screen MIDI keyboard ---
    // Scroll to show octave 3-5 (middle range) by default
    midiKeyboard.setAvailableRange (36, 96);   // C2 to C6
    midiKeyboard.setScrollButtonsVisible (true);
    addAndMakeVisible (midiKeyboard);
    SolaceLog::debug ("Editor ctor: MIDI keyboard created and visible");
    // Grab keyboard focus after a short delay so the user can play immediately.
    // We use a lambda timer so we don't block the constructor.
    //
    // IMPORTANT: use SafePointer, not raw [this]. The lambda fires on the message
    // thread 500ms later — if the user closes the plugin window in that window,
    // the editor is destroyed before the lambda fires. A raw 'this' would be a
    // dangling pointer → undefined behaviour. SafePointer nulls itself on
    // component deletion, making the check below safe.
    juce::Timer::callAfterDelay (500,
        [safeThis = juce::Component::SafePointer<SolaceSynthEditor> (this)]()
        {
            if (auto* self = safeThis.getComponent())
                self->midiKeyboard.grabKeyboardFocus();
        });

    // Set window size
    setSize (900, 600);
    SolaceLog::info ("Editor ctor: finished, window size set to 900x600");
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
    SolaceLog::debug ("createWebViewOptions: building options");

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
    const auto requestedPath = normaliseResourcePath (path);
    SolaceLog::debug ("resourceRequested: path='" + path + "' normalized='" + requestedPath + "'");

   #if SOLACE_ENABLE_DEV_UI_FALLBACK
    // Debug/dev builds can still load directly from disk for fast UI iteration.
    juce::File uiDir (SOLACE_DEV_UI_PATH);

    if (uiDir.isDirectory())
    {
        auto file = uiDir.getChildFile (requestedPath);

        if (file.isAChildOf (uiDir) && file.existsAsFile())
        {
            SolaceLog::debug ("resourceRequested: serving from dev disk path '" + file.getFullPathName() + "'");
            return loadFileAsResource (file);
        }
    }
   #endif

    if (auto embedded = loadEmbeddedResource (requestedPath))
    {
        SolaceLog::debug ("resourceRequested: serving embedded resource '" + requestedPath + "'");
        return embedded;
    }

    SolaceLog::error ("resourceRequested: embedded resource not found '" + requestedPath + "'");
    DBG ("Solace Synth: Embedded resource not found: " + requestedPath);
    return std::nullopt;
}

// ============================================================================
// Load a file from disk and return it as a WebView Resource
// ============================================================================
std::optional<juce::WebBrowserComponent::Resource>
SolaceSynthEditor::loadFileAsResource (const juce::File& file)
{
    SolaceLog::trace ("loadFileAsResource: reading '" + file.getFullPathName() + "'");

    // Read file contents
    juce::MemoryBlock mb;
    if (! file.loadFileAsData (mb))
    {
        SolaceLog::error ("loadFileAsResource: failed to read '" + file.getFullPathName() + "'");
        DBG ("Solace Synth: Failed to read file: " + file.getFullPathName());
        return std::nullopt;
    }

    SolaceLog::trace ("loadFileAsResource: read OK bytes=" + juce::String (static_cast<int> (mb.getSize())));
    return makeResourceFromMemory (mb.getData(),
                                   mb.getSize(),
                                   getMimeTypeForPath (file.getFullPathName()));
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
            SolaceLog::trace ("setParameter: " + paramId
                + " rawValue=" + juce::String (value, 4)
                + " normalized=" + juce::String (normalized, 4));

            param->setValueNotifyingHost (normalized);
            completion (juce::var (true));
        }
        else
        {
            SolaceLog::error ("setParameter: unknown param '" + paramId + "'");
            completion (juce::var (false));
        }
    }
    else
    {
        SolaceLog::error ("setParameter: wrong arg count (" + juce::String (args.size()) + ")");
        completion (juce::var (false));
    }
}

// Called when JS signals that the page has loaded
void SolaceSynthEditor::handleUiReady (
    const juce::Array<juce::var>& /*args*/,
    juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    SolaceLog::info ("UI ready signal received from JS");
    webViewReady = true;

    // Hide fallback, show WebView
    fallbackLabel.setVisible (false);

    // Send the current state of all parameters to JS
    sendAllParametersToJS();

    SolaceLog::info ("UI ready complete - all parameters synced to JS");
    completion (juce::var (true));
}

// Called when JS sends a debug log message
void SolaceSynthEditor::handleLog (
    const juce::Array<juce::var>& args,
    juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    if (! args.isEmpty())
    {
        SolaceLog::debug ("[JS] " + args[0].toString());
    }

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

    SolaceLog::trace ("C++->JS parameterChanged: " + paramId
        + " value=" + juce::String (value, 4));

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

    // Build an array of { paramId, value } objects. paramsArray itself is
    // stack-allocated; the per-entry DynamicObject instances are heap-allocated
    // (via juce::var ownership — freed when paramsArray goes out of scope).
    // This is fine: we're on the message thread, not the audio thread.
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
            SolaceLog::trace ("APVTS parameterChanged: " + parameterID
                + " newValue=" + juce::String (newValue, 4));
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
