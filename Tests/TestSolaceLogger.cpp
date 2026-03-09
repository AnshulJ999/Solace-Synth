#include <juce_core/juce_core.h>
#include "SolaceLogger.h"

class SolaceLoggerTests : public juce::UnitTest
{
public:
    SolaceLoggerTests() : juce::UnitTest ("SolaceLogger") {}

    void runTest() override
    {
        auto logDir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("SolaceSynthTests");
        auto infoFile = logDir.getChildFile ("info.log");
        auto debugFile = logDir.getChildFile ("debug.log");
        auto traceFile = logDir.getChildFile ("trace.log");

        // Start fresh
        logDir.deleteRecursively();
        logDir.createDirectory();

        beginTest ("Log files are created successfully");
        {
            // Clean up any existing logs
            infoFile.deleteFile();
            debugFile.deleteFile();
            traceFile.deleteFile();

            SolaceLogger logger (logDir);

            expect (infoFile.existsAsFile());
            expect (debugFile.existsAsFile());
            expect (traceFile.existsAsFile());
        }

        beginTest ("Info level cascades to all logs");
        {
            infoFile.deleteFile();
            debugFile.deleteFile();
            traceFile.deleteFile();

            {
                SolaceLogger logger (logDir);
                logger.write (LogLevel::Info, "Test Info Message");
            } // Destructor flushes the files

            auto infoContent = infoFile.loadFileAsString();
            auto debugContent = debugFile.loadFileAsString();
            auto traceContent = traceFile.loadFileAsString();

            expect (infoContent.contains ("Test Info Message"));
            expect (debugContent.contains ("Test Info Message"));
            expect (traceContent.contains ("Test Info Message"));
        }

        beginTest ("Debug level cascades to trace and debug logs");
        {
            infoFile.deleteFile();
            debugFile.deleteFile();
            traceFile.deleteFile();

            {
                SolaceLogger logger (logDir);
                logger.write (LogLevel::Debug, "Test Debug Message");
            } // Destructor flushes the files

            auto infoContent = infoFile.loadFileAsString();
            auto debugContent = debugFile.loadFileAsString();
            auto traceContent = traceFile.loadFileAsString();

            expect (! infoContent.contains ("Test Debug Message"));
            expect (debugContent.contains ("Test Debug Message"));
            expect (traceContent.contains ("Test Debug Message"));
        }

        beginTest ("Trace level cascades only to trace log");
        {
            infoFile.deleteFile();
            debugFile.deleteFile();
            traceFile.deleteFile();

            {
                SolaceLogger logger (logDir);
                logger.write (LogLevel::Trace, "Test Trace Message");
            } // Destructor flushes the files

            auto infoContent = infoFile.loadFileAsString();
            auto debugContent = debugFile.loadFileAsString();
            auto traceContent = traceFile.loadFileAsString();

            expect (! infoContent.contains ("Test Trace Message"));
            expect (! debugContent.contains ("Test Trace Message"));
            expect (traceContent.contains ("Test Trace Message"));
        }

        beginTest ("Error/Warn level cascades to all logs");
        {
            infoFile.deleteFile();
            debugFile.deleteFile();
            traceFile.deleteFile();

            {
                SolaceLogger logger (logDir);
                logger.write (LogLevel::Error, "Test Error Message");
                logger.write (LogLevel::Warn, "Test Warn Message");
            } // Destructor flushes the files

            auto infoContent = infoFile.loadFileAsString();
            auto debugContent = debugFile.loadFileAsString();
            auto traceContent = traceFile.loadFileAsString();

            expect (infoContent.contains ("Test Error Message"));
            expect (debugContent.contains ("Test Error Message"));
            expect (traceContent.contains ("Test Error Message"));

            expect (infoContent.contains ("Test Warn Message"));
            expect (debugContent.contains ("Test Warn Message"));
            expect (traceContent.contains ("Test Warn Message"));
        }

        // Clean up at the very end
        logDir.deleteRecursively();
    }
};

static SolaceLoggerTests solaceLoggerTests;
