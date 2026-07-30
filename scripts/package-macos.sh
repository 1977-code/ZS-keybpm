#!/usr/bin/env bash
#
# Builds the branded macOS installer: dist/ZS-keybpm-<version>.pkg
#
#   ./scripts/package-macos.sh              build universal Release, then package
#   ./scripts/package-macos.sh --no-build   package whatever is already in build/
#
# Signing is optional and driven by the environment. Without it the installer
# still works, but Gatekeeper will warn on first open (right-click → Открыть):
#
#   ZS_APP_ID="Developer ID Application: … (TEAMID)"     signs the plug-ins
#   ZS_INSTALLER_ID="Developer ID Installer: … (TEAMID)" signs the .pkg
#
# pkgbuild prints a handful of "write: Permission denied" lines on macOS 14 and
# later. That is it failing to archive the com.apple.provenance extended attribute
# the system stamps on every new file; it is not about the payload, which comes out
# complete either way.
#
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

version="$(sed -n 's/^project(ZSkeybpm VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
[[ -n "$version" ]] || { echo "could not read the version out of CMakeLists.txt" >&2; exit 1; }

artefacts="build/ZSkeybpm_artefacts/Release"
stage="build/package"
out="dist"

if [[ "${1:-}" != "--no-build" ]]; then
    ./scripts/build.sh
fi

for item in "$artefacts/VST3/ZS-keybpm.vst3" \
            "$artefacts/AU/ZS-keybpm.component" \
            "$artefacts/Standalone/ZS-keybpm.app"; do
    [[ -d "$item" ]] || { echo "missing $item — build first" >&2; exit 1; }
done

rm -rf "$stage"
mkdir -p "$stage"/{vst3,au,standalone,pkg} "$out"
mkdir -p "$stage/support/ZS-keybpm"

# ditto, not cp: it is the one copy on macOS that treats a bundle as a bundle.
ditto "$artefacts/VST3/ZS-keybpm.vst3"      "$stage/vst3/ZS-keybpm.vst3"
ditto "$artefacts/AU/ZS-keybpm.component"   "$stage/au/ZS-keybpm.component"
ditto "$artefacts/Standalone/ZS-keybpm.app" "$stage/standalone/ZS-keybpm.app"

# The licences travel with the binaries rather than being promised by a link:
# AGPLv3 requires a copy of itself to accompany the object code, and the OFL
# requires the same of the two font families compiled into the plug-in.
cp LICENSE                  "$stage/support/ZS-keybpm/LICENSE.txt"
cp Resources/Fonts/OFL.txt  "$stage/support/ZS-keybpm/LICENSE-fonts.txt"

# ─── optional code signing ──────────────────────────────────────────────────
if [[ -n "${ZS_APP_ID:-}" ]]; then
    echo "signing the bundles as $ZS_APP_ID"

    for bundle in "$stage/vst3/ZS-keybpm.vst3" \
                  "$stage/au/ZS-keybpm.component" \
                  "$stage/standalone/ZS-keybpm.app"; do
        codesign --force --timestamp --options runtime \
                 --sign "$ZS_APP_ID" "$bundle"
    done
else
    echo "ZS_APP_ID is not set — shipping unsigned bundles"
fi

# ─── component packages, one per format ─────────────────────────────────────
pkgbuild --quiet --identifier ru.zsrecords.zskeybpm.vst3 --version "$version" \
         --root "$stage/vst3"       --install-location "/Library/Audio/Plug-Ins/VST3" \
         "$stage/pkg/vst3.pkg"

pkgbuild --quiet --identifier ru.zsrecords.zskeybpm.au --version "$version" \
         --root "$stage/au"         --install-location "/Library/Audio/Plug-Ins/Components" \
         "$stage/pkg/au.pkg"

pkgbuild --quiet --identifier ru.zsrecords.zskeybpm.standalone --version "$version" \
         --root "$stage/standalone" --install-location "/Applications" \
         "$stage/pkg/standalone.pkg"

pkgbuild --quiet --identifier ru.zsrecords.zskeybpm.support --version "$version" \
         --root "$stage/support" --install-location "/Library/Application Support/ZS Records" \
         "$stage/pkg/support.pkg"

# ─── the branded installer around them ──────────────────────────────────────
installer="$out/ZS-keybpm-$version.pkg"

productbuild --distribution Installer/macos/distribution.xml \
             --resources    Installer/macos/resources \
             --package-path "$stage/pkg" \
             "$stage/unsigned.pkg"

if [[ -n "${ZS_INSTALLER_ID:-}" ]]; then
    echo "signing the installer as $ZS_INSTALLER_ID"
    productsign --sign "$ZS_INSTALLER_ID" "$stage/unsigned.pkg" "$installer"
else
    echo "ZS_INSTALLER_ID is not set — shipping an unsigned installer"
    mv "$stage/unsigned.pkg" "$installer"
fi

echo
echo "Built: $installer"
du -h "$installer" | awk '{ print "  " $1 }'
echo
echo "Installs:"
echo "  /Library/Audio/Plug-Ins/VST3/ZS-keybpm.vst3"
echo "  /Library/Audio/Plug-Ins/Components/ZS-keybpm.component"
echo "  /Applications/ZS-keybpm.app"
echo "  /Library/Application Support/ZS Records/ZS-keybpm   (licences, always)"
