#!/usr/bin/env bash
# Nested X display for exercising the GUI without touching the real desktop.
#
# Why a window manager is required: a bare Xephyr has no WM, so Qt maps dialogs
# (file pickers, message boxes) at 0x0 or off-screen and they take keyboard focus
# while being invisible - the UI looks alive but stops responding to clicks.
# matchbox maps them properly. Install it once with:
#   sudo apt-get install -y matchbox-window-manager
#
# Usage:
#   ./harness/gui-check.sh            # start display :99, WM and the app
#   ./harness/gui-check.sh --no-app   # display + WM only (bring your own binary)
#   GUI_DISPLAY=:98 ./harness/gui-check.sh
#
# Then: DISPLAY=:99 import -window root shot.png   (imagemagick) or xwd.
# Stop with: pkill -f "Xephyr $GUI_DISPLAY"; pkill -f matchbox-window-manager
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
display="${GUI_DISPLAY:-:99}"
screen="${GUI_SCREEN:-1400x900}"
binary="${GUI_BINARY:-$here/Build-Linux/QwenImageHarness}"
start_app=1

for arg in "$@"; do
    case "$arg" in
        --no-app) start_app=0 ;;
        -h|--help) sed -n '2,20p' "$0"; exit 0 ;;
        *) echo "unknown argument: $arg" >&2; exit 2 ;;
    esac
done

command -v Xephyr >/dev/null || { echo "Xephyr missing: sudo apt-get install -y xserver-xephyr" >&2; exit 1; }
command -v matchbox-window-manager >/dev/null || {
    echo "matchbox-window-manager missing - dialogs will not map:" >&2
    echo "  sudo apt-get install -y matchbox-window-manager" >&2
    exit 1
}

if xdpyinfo -display "$display" >/dev/null 2>&1; then
    echo "display $display already in use" >&2
    exit 1
fi

# Nested server, sized to the screen we want to inspect.
Xephyr "$display" -screen "$screen" -noreset -ac >/tmp/gui-check-xephyr.log 2>&1 &
xpid=$!
trap 'kill $xpid 2>/dev/null || true' EXIT

for _ in $(seq 1 50); do
    xdpyinfo -display "$display" >/dev/null 2>&1 && break
    sleep 0.2
done

# The WM has to be up before the app maps its first window.
DISPLAY="$display" matchbox-window-manager -use_titlebar no >/tmp/gui-check-wm.log 2>&1 &
wmpid=$!
trap 'kill $wmpid $xpid 2>/dev/null || true' EXIT
sleep 0.5

echo "display $display ready ($screen), WM running"

if [ "$start_app" -eq 1 ]; then
    [ -x "$binary" ] || { echo "no binary at $binary (run ./harness/build-linux.sh)" >&2; exit 1; }
    DISPLAY="$display" "$binary" >/tmp/gui-check-app.log 2>&1 &
    echo "app started: $binary"
    echo "stderr: /tmp/gui-check-app.log"
fi

echo "Ctrl-C (or kill this shell) tears the nested display down."
wait $wmpid
