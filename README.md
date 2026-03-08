# Solace Synth

A free, open-source polyphonic soft synthesizer — VST3 plugin and standalone application.

> 🚧 **Status:** Early development — project skeleton being set up.

## Features (Planned V1)

- 2 Oscillators (Sine, Saw, Square, Triangle, Noise) with tuning and octave controls
- Oscillator Mix (level sliders)
- Amplifier ADSR envelope + Master Level
- Filter (LP12, LP24, HP12) with cutoff, resonance, and dedicated ADSR
- LFO with assignable modulation targets
- Polyphony (1–16 configurable voices)
- Unison (1–4 voices with detune)
- Velocity sensitivity with assignable modulation
- Modern WebView-based UI (HTML/CSS/JS)

## Tech Stack

- **Audio Engine:** C++ with [JUCE 8](https://juce.com/)
- **UI:** HTML/CSS/JS via JUCE WebView (WebView2 on Windows)
- **Build System:** CMake
- **Formats:** VST3, Standalone

## Building

### Prerequisites

- Visual Studio 2022/2026 with "Desktop development with C++" workload
- CMake 3.25+
- Git

### Build Steps

```bash
# Configure (first time — downloads JUCE automatically)
cmake -B build

# Build
cmake --build build --config Release
```

The built plugin and standalone app will be in `build/SolaceSynth_artefacts/`.

## Project Structure

```
Source/          — C++ audio engine and plugin code
UI/              — HTML/CSS/JS frontend (WebView UI)
.agent/          — Project memory and AI context
```

## License

TBD — License will be decided before public release.

## Authors

Built by Anshul and Nabeel.
