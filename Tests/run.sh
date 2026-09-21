#!/bin/bash
set -euo pipefail
repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repo_dir"
build_dir="${BLUESYNTH_TEST_BUILD_DIR:-${TMPDIR:-/tmp}/bluesynth-regression}"
modules_dir="${JUCE_MODULES_DIR:-$repo_dir/../../JUCE/modules}"
mkdir -p "$build_dir"
if ! xcodebuild -project Builds/MacOSX/BlueSynth.xcodeproj \
    -target 'BlueSynth - Shared Code' -configuration Release \
    CONFIGURATION_BUILD_DIR="$build_dir/Release" OBJROOT="$build_dir/obj" \
    build > "$build_dir/build.log" 2>&1; then
    tail -60 "$build_dir/build.log"
    exit 1
fi
clang++ -std=c++17 -O2 -DNDEBUG -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DJUCE_STRICT_REFCOUNTEDPOINTER=1 -I JuceLibraryCode -I "$modules_dir" -I Source \
    Tests/RegressionTests.cpp "$build_dir/Release/libBlueSynth.a" \
    -framework Cocoa -framework IOKit -framework CoreAudio -framework AudioToolbox \
    -framework Accelerate -framework QuartzCore -framework Metal -framework MetalKit \
    -framework WebKit -framework Security -framework CoreMIDI -framework DiscRecording \
    -framework AVFoundation -framework CoreMedia -framework ScreenCaptureKit \
    -framework UniformTypeIdentifiers -o "$build_dir/regression-tests"
"$build_dir/regression-tests"
