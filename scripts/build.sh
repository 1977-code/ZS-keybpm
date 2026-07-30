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
