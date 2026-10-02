# Chord Progression Generator (VST3 / AU)

Generates a fresh chord progression every time you press **Generate**, in **minor** or **phrygian**
(or random between the two). Export it as a `.mid` file or drag it straight into FL Studio, Logic or Ableton.

- Key (or random), scale, triads / 7ths / mixed (with 9ths), 1 or 2 chords per bar or varied rhythm, 1-16 bars
- Variety slider: how adventurous the harmony is
- Smooth voice leading with inversions, optional bass note
- Never repeats a recent progression
- Prev / next arrows to go back through earlier results
- Built-in preview sound (Play) so you can audition before exporting
- **Drag MIDI into your DAW** handle + **Export MIDI...** button

Formats: VST3 (Windows + macOS), AU (macOS, for Logic/GarageBand), Standalone.

## Build

Requirements: CMake 3.22+, a C++17 compiler.

**Windows:** Visual Studio 2022 (Desktop C++ workload)

```
cmake -B build
cmake --build build --config Release
```

**macOS:** Xcode command line tools (`xcode-select --install`)

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

JUCE is downloaded automatically on first configure. Output is in `build/ChordGen_artefacts/Release/`.

No Mac/PC for one of the platforms? Push this folder to GitHub: `.github/workflows/build.yml` builds both
and uploads the plugins as downloadable artifacts.

## Install

| Platform | Format | Copy to |
|---|---|---|
| Windows | VST3 | `C:\Program Files\Common Files\VST3` |
| macOS | VST3 | `/Library/Audio/Plug-Ins/VST3` |
| macOS | AU | `/Library/Audio/Plug-Ins/Components` |

macOS (unsigned build): run `xattr -cr "<plugin>"` and `codesign --force --deep -s - "<plugin>"` once on the copied plugin.

Or configure with `-DCOPY_AFTER_BUILD=ON` to copy automatically (Windows needs an admin terminal).

## Use in your DAW

- **FL Studio:** Options > Manage plugins > *Find installed plugins*, then add it as a generator. Drag the MIDI handle onto the Playlist or Piano roll.
- **Logic Pro:** load as an Instrument (AU), drag the handle onto a software instrument track.
- **Ableton Live:** rescan plugins, put it on a MIDI track, drag the handle onto any MIDI track.

If a DAW refuses the drag, use **Export MIDI...** and drag the saved file in manually.

## Layout

- `Source/ChordEngine.*` pure C++ engine (no JUCE), unit tested in `tests/`
- `Source/PluginProcessor.*` parameters, preview playback, state
- `Source/PluginEditor.*` UI and MIDI drag-and-drop

## License note

JUCE is AGPLv3 or commercial. To distribute this plugin closed-source you need a JUCE commercial license.
