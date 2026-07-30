#!/usr/bin/env bash
#
# Builds ZS-keybpm as a universal (arm64 + x86_64) release and copies the
# results into the user plug-in folders.
#
#   ./scripts/build.sh            release build + install
#   ./scripts/build.sh --tests    also build and run the offline DSP checks
#
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

source "$root/scripts/fetch-juce.sh"
zs_fetch_juce

want_tests=0
[[ "${1:-}" == "--tests" ]] && want_tests=1

# CMAKE_OSX_DEPLOYMENT_TARGET is a cache variable, so an existing build directory
# keeps whatever it was first configured with — raise the minimum in CMakeLists.txt
# and the binary silently keeps the old one, which is exactly the kind of bug that
# only shows up on someone else's older Mac. Reconfigure from scratch when they
# disagree.
wanted="$(sed -n 's/.*CMAKE_OSX_DEPLOYMENT_TARGET "\([0-9.]*\)".*/\1/p' CMakeLists.txt | head -1)"
cached="$(sed -n 's/^CMAKE_OSX_DEPLOYMENT_TARGET:STRING=//p' build/CMakeCache.txt 2>/dev/null | head -1)"

if [[ -n "$wanted" && -n "$cached" && "$wanted" != "$cached" ]]; then
    echo "deployment target changed ($cached → $wanted) — reconfiguring from scratch"
    rm -rf build
fi

cmake -B build -G "Unix Makefiles" \
      -DCMAKE_BUILD_TYPE=Release \
      -DZSKEYBPM_COPY_AFTER_BUILD=ON \
      -DZSKEYBPM_BUILD_TESTS=$want_tests

cmake --build build -j "$(sysctl -n hw.ncpu)"

if [[ $want_tests -eq 1 ]]; then
    echo
    ./build/ZSkeybpmTests_artefacts/Release/ZSkeybpmTests
fi

echo
echo "Built:"
find build/ZSkeybpm_artefacts/Release -maxdepth 2 -name "ZS-keybpm.*" -o -maxdepth 2 -name "ZS-keybpm.app" | sed 's/^/  /'
echo
echo "Installed to:"
echo "  ~/Library/Audio/Plug-Ins/VST3/ZS-keybpm.vst3"
echo "  ~/Library/Audio/Plug-Ins/Components/ZS-keybpm.component"
