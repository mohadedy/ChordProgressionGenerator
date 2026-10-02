#!/bin/bash
# Build and install the Chord Progression Generator plugin on macOS.
# Run this from inside the ChordProgressionGenerator folder (where CMakeLists.txt lives):
#   chmod +x install_mac.sh
#   ./install_mac.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "== Checking for Xcode Command Line Tools =="
if ! xcode-select -p >/dev/null 2>&1; then
    echo "Xcode Command Line Tools not found. A system dialog will pop up - click Install and wait for it to finish, then re-run this script."
    xcode-select --install
    exit 1
fi
echo "OK."

echo "== Checking for CMake =="
if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake not found."
    if command -v brew >/dev/null 2>&1; then
        echo "Installing CMake via Homebrew..."
        brew install cmake
    else
        echo "Homebrew not found either. Install CMake manually from https://cmake.org/download/ (get the macOS .dmg), then re-run this script."
        exit 1
    fi
fi
echo "CMake: $(cmake --version | head -n1)"

echo "== Configuring (this downloads JUCE on first run, needs internet) =="
cmake -B build -DCMAKE_BUILD_TYPE=Release

echo "== Building (first build can take 15-30+ minutes) =="
cmake --build build --parallel

echo "== Running unit tests =="
ctest --test-dir build --output-on-failure || echo "WARNING: tests failed, continuing anyway"

ARTEFACTS="build/ChordGen_artefacts/Release"
VST3_SRC="$ARTEFACTS/VST3/Chord Progression Generator.vst3"
AU_SRC="$ARTEFACTS/AU/Chord Progression Generator.component"

VST3_DEST="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DEST="$HOME/Library/Audio/Plug-Ins/Components"

mkdir -p "$VST3_DEST" "$AU_DEST"

if [ -d "$VST3_SRC" ]; then
    echo "== Installing VST3 to $VST3_DEST =="
    rm -rf "$VST3_DEST/Chord Progression Generator.vst3"
    cp -R "$VST3_SRC" "$VST3_DEST/"
    xattr -cr "$VST3_DEST/Chord Progression Generator.vst3"
    codesign --force --deep -s - "$VST3_DEST/Chord Progression Generator.vst3"
    echo "VST3 installed."
else
    echo "VST3 build not found at $VST3_SRC"
fi

if [ -d "$AU_SRC" ]; then
    echo "== Installing AU to $AU_DEST =="
    rm -rf "$AU_DEST/Chord Progression Generator.component"
    cp -R "$AU_SRC" "$AU_DEST/"
    xattr -cr "$AU_DEST/Chord Progression Generator.component"
    codesign --force --deep -s - "$AU_DEST/Chord Progression Generator.component"
    echo "AU installed."
else
    echo "AU build not found at $AU_SRC"
fi

echo ""
echo "Done. Rescan plugins in your DAW (Logic/GarageBand will see the AU; Ableton/others will see the VST3)."
