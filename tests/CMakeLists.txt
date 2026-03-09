include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(Tests
  PluginEditorTest.cpp
)

# We link directly to the SolaceSynth library to reuse the compiled objects.
target_link_libraries(Tests PRIVATE
    Catch2::Catch2WithMain
    SolaceSynth
)

target_compile_definitions(Tests
    PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    SOLACE_DEV_UI_PATH="${CMAKE_SOURCE_DIR}/UI"
)

target_link_libraries(Tests PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Since we don't compile JUCE here, we just need the headers
target_include_directories(Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source
    ${CMAKE_BINARY_DIR}/SolaceSynth_artefacts/JuceLibraryCode
    ${CMAKE_SOURCE_DIR}/build/_deps/juce-src/modules
)

include(CTest)
include(Catch)
catch_discover_tests(Tests)
