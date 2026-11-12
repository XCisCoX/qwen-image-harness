#!/usr/bin/env bash
# Start ComfyUI for Qwen-Image-2.1 on an 8 GB card, headless-friendly.
#
#   ./run-server.sh            # foreground
#   ./run-server.sh --bg       # background, logs to server.log
#
# The card is 8 GB: keep --listen on loopback and reach the UI through
#   ssh -L 8188:127.0.0.1:8188 <user>@<host>
set -euo pipefail

COMFY="${COMFY_DIR:-$HOME/git/ComfyUI}"
PORT="${PORT:-8188}"

cd "$COMFY"
export PYTORCH_CUDA_ALLOC_CONF=expandable_segments:True
export HF_HOME="${HF_HOME:-$HOME/AI/hf}"

CMD=(venv/bin/python main.py --listen 127.0.0.1 --port "$PORT" --reserve-vram 0.8 --fp16-unet)

if [ "${1:-}" = "--bg" ]; then
    exec nohup "${CMD[@]}" > ${HOME}/.local/state/qwen-image-harness/server.log 2>&1 &
    echo "started pid $! -- log: ${HOME}/.local/state/qwen-image-harness/server.log"
else
    exec "${CMD[@]}"
fi
