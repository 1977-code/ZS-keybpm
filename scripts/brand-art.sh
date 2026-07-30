#!/usr/bin/env bash
#
# Regenerates every branded image in the repository from Source/gui/Theme, so the
# installers can never drift from the interface:
#
#   docs/ui.png                              the editor, populated with real analysis
#   Installer/macos/resources/watermark.png  corner mark for the macOS installer
#   Installer/windows/wizard-*.{png,bmp}     panels for the Windows installer
#
# Inno Setup only reads BMP, so the PNG is kept as the lossless original and the
# BMP is derived from it here.
#
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

source "$root/scripts/fetch-juce.sh"
zs_fetch_juce

cmake -B build-dev -G "Unix Makefiles" \
      -DCMAKE_BUILD_TYPE=Release \
      -DZSKEYBPM_COPY_AFTER_BUILD=OFF \
      -DZSKEYBPM_BUILD_ART=ON > /dev/null

cmake --build build-dev --target ZSkeybpmArt -j "$(sysctl -n hw.ncpu)" > /dev/null

art="$root/build-dev/ZSkeybpmArt_artefacts/Release/ZSkeybpmArt"

mkdir -p docs Installer/macos/resources Installer/windows

"$art" ui           "docs/ui.png"
"$art" watermark    "Installer/macos/resources/watermark.png"
"$art" wizard-large "Installer/windows/wizard-large.png"
"$art" wizard-small "Installer/windows/wizard-small.png"
"$art" icon         "Installer/windows/icon.png"

# Not `sips`: it writes a 32-bpp BI_BITFIELDS BITMAPV5 top-down BMP, and Inno Setup
# loads wizard images through Delphi's TBitmap, which wants the plain 24-bpp BI_RGB
# bottom-up form. Pillow writes exactly that.
python3 - <<'PY'
import pathlib

try:
    from PIL import Image
except ImportError:
    raise SystemExit("Pillow is needed for the wizard BMPs: python3 -m pip install pillow")

def flatten(path):
    # Flattened onto the brand near-black: BMP and .ico carry no alpha here.
    art = Image.open(path).convert("RGBA")
    flat = Image.new("RGB", art.size, (5, 5, 5))
    flat.paste(art, mask=art.split()[3])
    return flat

for name in ("wizard-large", "wizard-small"):
    source = pathlib.Path("Installer/windows") / f"{name}.png"
    target = source.with_suffix(".bmp")

    flat = flatten(source)
    flat.save(target, format="BMP")

    print(f"wrote {target.resolve()}  ({flat.width} x {flat.height}, 24-bit)")

# The setup executable's icon, every size Explorer and the taskbar ask for.
icon = pathlib.Path("Installer/windows/zskeybpm.ico")
flatten("Installer/windows/icon.png").save(
    icon, format="ICO",
    sizes=[(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])

print(f"wrote {icon.resolve()}")
PY
