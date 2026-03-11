


## 🐛 Distribution Debugging (2026-03-11)

### Problem
Standalone EXE exits silently on test machines with no visible window. Logs from `%TEMP%\SolaceSynth\` showed the processor starting and `prepareToPlay` completing, but then nothing — no editor or WebView logs.

### Root Cause Diagnosis (all 3 AIs in agreement)
In JUCE Standalone, `prepareToPlay` fires **before** the editor is created. Log ending there means `SolaceSynthEditor` constructor is crashing. Most likely: `std::make_unique<juce::WebBrowserComponent>(options)` fails during WebView2 controller creation due to:
1. WebView2 Runtime too old (JUCE 8 requires ≥ 1.0.1774)
2. Wrong installer architecture (x86 vs x64)
3. COM/GPU init failure

### Changes Made (this session)

**`PluginEditor.cpp`** — Wrapped `std::make_unique<WebBrowserComponent>` and `goToURL` in try-catch. Instead of crashing the process, a WebView2 failure now shows a graceful fallback label and logs an error to the file. Both `catch (std::exception&)` and `catch (...)` handlers added.

**`PluginProcessor.cpp` + `PluginEditor.cpp`** — Granular startup logging added by Codex, reviewed and kept. Every stage of the editor constructor now logs, so the next test run will show exactly which line is reached before failure.

**`CMakeLists.txt` line 135** — `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` added by Gemini. Correct — tells JUCE to statically link `WebView2LoaderStatic.lib` (already in our NuGet package) instead of needing `WebView2Loader.dll` at runtime. Does **not** remove the WebView2 Runtime requirement.

### Next Steps
1. Build Release, send EXE to test machine
2. If it still fails: collect `%TEMP%\SolaceSynth\info.log` — will now show exactly which stage failed
3. Also check **Windows Event Viewer → Application** at the failure timestamp for faulting module name
4. Ask tester if ANY window appeared (even grey/blank)
5. If Runtime is culprit: direct tester to `edge://components` → update WebView2, or download x64 Evergreen Runtime from Microsoft

### Key Distinction
- `WebView2LoaderStatic.lib` = small loader shim — now statically linked into our EXE ✅
- WebView2 Runtime = Chromium browser engine — **still must be installed on the target machine**


