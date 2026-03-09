# ============================================================================
# Solace Synth Tests — CMakeLists.txt
# ============================================================================

juce_add_console_app(SolaceSynthTests
    PRODUCT_NAME "Solace Synth Tests"
)

# Source files
file(GLOB_RECURSE TestSourceFiles
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

target_sources(SolaceSynthTests PRIVATE ${TestSourceFiles})

# Preprocessor definitions
target_compile_definitions(SolaceSynthTests
    PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

# Include directories
target_include_directories(SolaceSynthTests PRIVATE "${CMAKE_SOURCE_DIR}/Source")

# Link JUCE modules
target_link_libraries(SolaceSynthTests
    PRIVATE
    juce_audio_utils
    juce_dsp
    juce_gui_basics

    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# C++ standard
target_compile_features(SolaceSynthTests PRIVATE cxx_std_20)

# Add to CTest
add_test(NAME SolaceSynthTests COMMAND SolaceSynthTests)
