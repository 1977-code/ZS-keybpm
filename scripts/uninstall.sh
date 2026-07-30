#!/usr/bin/env bash
#
# Removes ZS-keybpm from the user plug-in folders. Nothing else is touched.
#
set -euo pipefail

# Both places it can live: the user folders a local build installs into, and the
# system folders the .pkg installs into.
targets=(
    "$HOME/Library/Audio/Plug-Ins/VST3/ZS-keybpm.vst3"
    "$HOME/Library/Audio/Plug-Ins/Components/ZS-keybpm.component"
    "/Library/Audio/Plug-Ins/VST3/ZS-keybpm.vst3"
    "/Library/Audio/Plug-Ins/Components/ZS-keybpm.component"
    "/Applications/ZS-keybpm.app"
    "/Library/Application Support/ZS Records/ZS-keybpm"
)

for target in "${targets[@]}"; do
    if [[ -e "$target" ]]; then
        if rm -rf "$target" 2>/dev/null; then
            echo "removed  $target"
        else
            echo "needs sudo to remove  $target"
        fi
    else
        echo "absent   $target"
    fi
done
