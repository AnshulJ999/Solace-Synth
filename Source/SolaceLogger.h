#pragma once

#include <juce_core/juce_core.h>

// ============================================================================
// Solace Synth - Multi-Level File Logger
//
// Writes to 3 separate log files with cascading levels:
//   info.log  — INFO, WARN, ERROR
//   debug.log — DEBUG + everything in info.log
//   trace.log — TRACE + everything (every event, every slider move)
//
// Log files are in: %TEMP%/SolaceSynth/
//
// Usage:
//   SolaceLog::info("Bridge handshake complete");
//   SolaceLog::debug("setParameter: masterVolume = 0.85");
//   SolaceLog::trace("JS input event: value=0.8500");
//   SolaceLog::warn("WebView not ready, skipping event");
//   SolaceLog::error("Unknown parameter: foo");
//
// IMPORTANT: Do NOT call any SolaceLog:: method from the audio thread.
// Audio thread callbacks must bounce to the message thread first.
// ============================================================================

enum class LogLevel
{
    Trace = 0,  // Every event (slider moves, echoes)
    Debug = 1,  // Bridge calls, parameter changes
    Info  = 2,  // Key lifecycle events (startup, handshake, shutdown)
    Warn  = 3,  // Suspicious but non-fatal
    Error = 4   // Something broke
};

class SolaceLogger : public juce::Logger
{
public:
    SolaceLogger()
    {
        auto logDir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                          .getChildFile ("SolaceSynth");

        // Create 3 log files with 512KB max each
        traceLogger = std::make_unique<juce::FileLogger> (
            logDir.getChildFile ("trace.log"), "Solace Synth - TRACE", 1024 * 512);
        debugLogger = std::make_unique<juce::FileLogger> (
            logDir.getChildFile ("debug.log"), "Solace Synth - DEBUG", 1024 * 512);
        infoLogger = std::make_unique<juce::FileLogger> (
            logDir.getChildFile ("info.log"), "Solace Synth - INFO", 1024 * 512);
    }

    // Write a message at a specific level — cascades to appropriate files
    void write (LogLevel level, const juce::String& message)
    {
        auto prefix = getLevelPrefix (level);
        auto formatted = prefix + " " + message;

        // Cascade: higher-verbosity files include lower-verbosity messages
        switch (level)
        {
            case LogLevel::Trace:
                traceLogger->logMessage (formatted);
                break;

            case LogLevel::Debug:
                traceLogger->logMessage (formatted);
                debugLogger->logMessage (formatted);
                break;

            case LogLevel::Info:
            case LogLevel::Warn:
            case LogLevel::Error:
                traceLogger->logMessage (formatted);
                debugLogger->logMessage (formatted);
                infoLogger->logMessage (formatted);
                break;
        }
    }

    // juce::Logger override — route untagged messages to DEBUG
    void logMessage (const juce::String& message) override
    {
        write (LogLevel::Debug, message);
    }

    // Get the log directory path (for display/reference)
    juce::String getLogDirectory() const
    {
        return juce::File::getSpecialLocation (juce::File::tempDirectory)
                   .getChildFile ("SolaceSynth")
                   .getFullPathName();
    }

private:
    std::unique_ptr<juce::FileLogger> traceLogger;
    std::unique_ptr<juce::FileLogger> debugLogger;
    std::unique_ptr<juce::FileLogger> infoLogger;

    static juce::String getLevelPrefix (LogLevel level)
    {
        switch (level)
        {
            case LogLevel::Trace: return "[TRACE]";
            case LogLevel::Debug: return "[DEBUG]";
            case LogLevel::Info:  return "[INFO] ";
            case LogLevel::Warn:  return "[WARN] ";
            case LogLevel::Error: return "[ERROR]";
            default:              return "[?????]";
        }
    }
};

// ============================================================================
// Convenience functions — use these everywhere in the codebase
//
// These get the current Logger instance (set in PluginProcessor) and
// write at the specified level. Safe to call even if no logger is set.
// ============================================================================
namespace SolaceLog
{
    inline SolaceLogger* getLogger()
    {
        return dynamic_cast<SolaceLogger*> (juce::Logger::getCurrentLogger());
    }

    inline void trace (const juce::String& msg)
    {
        if (auto* logger = getLogger())
            logger->write (LogLevel::Trace, msg);
    }

    inline void debug (const juce::String& msg)
    {
        if (auto* logger = getLogger())
            logger->write (LogLevel::Debug, msg);
    }

    inline void info (const juce::String& msg)
    {
        if (auto* logger = getLogger())
            logger->write (LogLevel::Info, msg);
    }

    inline void warn (const juce::String& msg)
    {
        if (auto* logger = getLogger())
            logger->write (LogLevel::Warn, msg);
    }

    inline void error (const juce::String& msg)
    {
        if (auto* logger = getLogger())
            logger->write (LogLevel::Error, msg);
    }
}
