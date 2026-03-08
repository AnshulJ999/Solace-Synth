# ============================================================================
# Solace Synth — CMakeLists.txt
#
# JUCE 8 plugin project using CMake FetchContent.
# Based on pamplejuce patterns, configured for Solace Synth.
# ============================================================================

cmake_minimum_required(VERSION 3.25)

# Project version
set(PROJECT_VERSION "0.1.0")

# Plugin metadata — these configure the plugin identity
set(PROJECT_NAME "SolaceSynth")               # Internal name (no spaces)
set(PRODUCT_NAME "Solace Synth")              # Display name in DAWs (spaces OK)
set(COMPANY_NAME "Solace")                    # Manufacturer name
set(BUNDLE_ID "com.solace.solacesynth")       # macOS bundle identifier
set(PLUGIN_MANUFACTURER_CODE Solc)            # 4-char manufacturer code (first char uppercase)
set(PLUGIN_CODE Ss01)                         # 4-char unique plugin code (needs one uppercase)

# Plugin formats to build
set(FORMATS Standalone VST3)

# Force MSVC to embed debug info (for sccache/ccache compatibility)
set(CMAKE_POLICY_DEFAULT_CMP0141        NEW      CACHE STRING "" FORCE)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded CACHE STRING "" FORCE)

project(${PROJECT_NAME} VERSION ${PROJECT_VERSION})

# ============================================================================
# Fetch JUCE 8 via FetchContent (no submodule needed)
# ============================================================================
include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE    # Don't download full git history — faster
)

FetchContent_MakeAvailable(JUCE)

# ============================================================================
# Define the plugin target
# ============================================================================
juce_add_plugin("${PROJECT_NAME}"
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"

    # Copy built plugin to system plugin folder after build (convenient for testing)
    COPY_PLUGIN_AFTER_BUILD TRUE

    PLUGIN_MANUFACTURER_CODE ${PLUGIN_MANUFACTURER_CODE}
    PLUGIN_CODE ${PLUGIN_CODE}
    FORMATS ${FORMATS}

    # This is a synthesizer — it produces audio from MIDI, no audio input
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE

    # WebView support — required for our HTML/CSS/JS UI
    NEEDS_WEB_BROWSER TRUE

    PRODUCT_NAME "${PRODUCT_NAME}"
)

# ============================================================================
# Source files
# ============================================================================
# Glob all .cpp and .h files from Source/ directory
file(GLOB_RECURSE SourceFiles
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
)

target_sources("${PROJECT_NAME}" PRIVATE ${SourceFiles})

# ============================================================================
# Preprocessor definitions
# ============================================================================
target_compile_definitions("${PROJECT_NAME}"
    PUBLIC
    JUCE_WEB_BROWSER=1          # Enable WebView support
    JUCE_USE_CURL=0             # No need for CURL
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0  # Using JUCE Starter commercial license

    # Build info available to code
    CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    VERSION="${PROJECT_VERSION}"
    PRODUCT_NAME_WITHOUT_VERSION="${PROJECT_NAME}"
)

# ============================================================================
# Include directories — so Source/ files can find each other
# ============================================================================
target_include_directories("${PROJECT_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")

# ============================================================================
# Link JUCE modules
# ============================================================================
target_link_libraries("${PROJECT_NAME}"
    PRIVATE
    juce_audio_utils         # Includes audio_basics, audio_devices, audio_formats, audio_processors
    juce_dsp                 # DSP classes (oscillators, filters, etc.)
    juce_gui_basics          # Core GUI
    juce_gui_extra           # WebBrowserComponent lives here

    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ============================================================================
# C++ standard
# ============================================================================
target_compile_features("${PROJECT_NAME}" PRIVATE cxx_std_20)
