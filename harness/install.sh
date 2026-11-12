#!/usr/bin/env bash
# Install Qwen Image Harness for the current user.
#
#   ./install.sh            # build + install launcher, icon and .desktop entry
#   ./install.sh --no-build # reuse the existing Build-Linux/QwenImageHarness
#
# Installs into ~/.local (no sudo):
#   ~/.local/bin/qwen-image-harness                          launcher on PATH
#   ~/.local/share/applications/qwen-image-harness.desktop   menu entry
#   ~/.local/share/icons/hicolor/256x256/apps/qwen-image-harness.png
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="${ROOT}/Build-Linux/QwenImageHarness"
BIN_DIR="${HOME}/.local/bin"
APP_DIR="${HOME}/.local/share/applications"
ICON_DIR="${HOME}/.local/share/icons/hicolor/256x256/apps"
LAUNCHER="${BIN_DIR}/qwen-image-harness"

if [ "${1:-}" != "--no-build" ]; then
    "${ROOT}/build-linux.sh"
fi

[ -x "${BIN}" ] || { echo "[ERROR] ${BIN} missing - run build-linux.sh first" >&2; exit 1; }

mkdir -p "${BIN_DIR}" "${APP_DIR}" "${ICON_DIR}"

# Launcher delegates to the built binary, so a rebuild is picked up instantly.
cat > "${LAUNCHER}" <<EOF
#!/bin/sh
# Qwen Image Harness launcher (installed by ${ROOT}/install.sh)
exec "${BIN}" "\$@"
EOF
chmod +x "${LAUNCHER}"

# Icon: the app paints its own mark procedurally; render one with the same shape.
python3 - "${ICON_DIR}/qwen-image-harness.png" <<'PY'
import sys
try:
    from PIL import Image, ImageDraw
except ImportError:
    sys.exit(0)
size = 256
img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
d = ImageDraw.Draw(img)
d.rounded_rectangle((8, 8, size - 8, size - 8), radius=58, fill=(108, 98, 240, 255))
cx, cy, r = 128, 128, 62
d.polygon([(cx, cy - r), (cx + r * 0.28, cy - r * 0.28), (cx + r, cy),
           (cx + r * 0.28, cy + r * 0.28), (cx, cy + r),
           (cx - r * 0.28, cy + r * 0.28), (cx - r, cy),
           (cx - r * 0.28, cy - r * 0.28)], fill=(255, 255, 255, 235))
img.save(sys.argv[1])
PY

cat > "${APP_DIR}/qwen-image-harness.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Qwen Image Harness
GenericName=Image generation
Comment=Generate, edit and mask-infill images with Qwen-Image-2.1 (local ComfyUI)
Exec=${LAUNCHER}
Icon=${ICON_DIR}/qwen-image-harness.png
Terminal=false
Categories=Graphics;2DGraphics;
Keywords=qwen;image;generate;edit;diffusion;
StartupWMClass=QwenImageHarness
EOF

chmod +x "${APP_DIR}/qwen-image-harness.desktop"
command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "${APP_DIR}" >/dev/null 2>&1 || true
command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -f -t "${HOME}/.local/share/icons/hicolor" >/dev/null 2>&1 || true

echo "[ok] launcher : ${LAUNCHER}"
echo "[ok] desktop  : ${APP_DIR}/qwen-image-harness.desktop"
echo "[ok] icon     : ${ICON_DIR}/qwen-image-harness.png"
echo
echo "Run it with:   qwen-image-harness"
echo "Headless:      qwen-image-harness --run \"a brass key on slate\" --out ~/Pictures/qwen"
