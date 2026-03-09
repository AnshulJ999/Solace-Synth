#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

class SolaceSynthEditorTest {
public:
    static bool testProcessSetParameter(SolaceSynthEditor& editor, const juce::Array<juce::var>& args) {
        return editor.processSetParameter(args);
    }
};

TEST_CASE("PluginEditor handles setParameter", "[editor]") {
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    SolaceSynthProcessor processor;
    SolaceSynthEditor editor(processor);

    auto& apvts = processor.getAPVTS();
    auto* masterVolumeParam = apvts.getParameter("masterVolume");
    REQUIRE(masterVolumeParam != nullptr);

    SECTION("Valid parameter and value updates APVTS and returns true") {
        juce::Array<juce::var> args;
        args.add(juce::var("masterVolume"));
        args.add(juce::var(0.5));

        bool result = SolaceSynthEditorTest::testProcessSetParameter(editor, args);

        REQUIRE(result == true);
        REQUIRE(masterVolumeParam->getValue() == Catch::Approx(0.5f));
    }

    SECTION("Unknown parameter returns false and leaves params unchanged") {
        float originalValue = masterVolumeParam->getValue();
        juce::Array<juce::var> args;
        args.add(juce::var("unknownParam"));
        args.add(juce::var(0.75));

        bool result = SolaceSynthEditorTest::testProcessSetParameter(editor, args);

        REQUIRE(result == false);
        REQUIRE(masterVolumeParam->getValue() == Catch::Approx(originalValue));
    }

    SECTION("Missing arguments returns false") {
        juce::Array<juce::var> args;
        args.add(juce::var("masterVolume"));

        bool result = SolaceSynthEditorTest::testProcessSetParameter(editor, args);

        REQUIRE(result == false);
    }
}
