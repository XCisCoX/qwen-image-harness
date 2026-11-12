#!/usr/bin/env bash
# Deterministic installer for the Qwen-Image-2.1 desktop frontend.
#
#   ./setup.sh                  # everything: venv, ComfyUI, patch, weights, UI build
#   ./setup.sh --no-models      # skip the ~14 GB weight download
#   ./setup.sh --dry-run        # print what would happen
#
# Idempotent: re-running finishes what is missing and skips what is already good.
# Every phase prints a PASS/FAIL gate; a FAIL stops the script.
#
# Nothing here needs sudo. Nothing touches an existing ComfyUI installation or an
# existing model vault: it creates its own when the paths are free, and reuses
# what is already there otherwise.
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMFY_DIR="${COMFY_DIR:-$HOME/git/ComfyUI}"
MODEL_DIR="${MODEL_DIR:-$HOME/Models}"
COMFY_TAG="${COMFY_TAG:-v0.39.2}"
GGUF_NODE_TAG="${GGUF_NODE_TAG:-main}"
MIRROR="${HF_ENDPOINT:-https://hf-mirror.com}"
WITH_MODELS=1
DRY=0

for arg in "$@"; do
    case "$arg" in
        --no-models) WITH_MODELS=0 ;;
        --dry-run)   DRY=1 ;;
        -h|--help)   sed -n '2,12p' "$0"; exit 0 ;;
        *) echo "unknown argument: $arg" >&2; exit 2 ;;
    esac
done

ok()   { printf '\n[PASS] %s\n' "$*"; }
gap()  { printf '\n[FAIL] %s\n' "$*" >&2; exit 1; }
step() { printf '\n=== %s\n' "$*"; }
run()  { if [ "$DRY" = 1 ]; then printf '  would run: %s\n' "$*"; else "$@"; fi; }

# ---------------------------------------------------------------- phase 0 -----
step "Phase 0 - probes"
for tool in git python3; do
    command -v "$tool" >/dev/null 2>&1 || gap "$tool is missing (apt install git python3 python3-venv)"
done
python3 -c 'import venv' 2>/dev/null || gap "python3-venv missing (apt install python3-venv)"
command -v aria2c >/dev/null 2>&1 || echo "  note: aria2c missing; downloads will be ~10x slower (apt install aria2c)"

FREE_GB=$(df -Pk "$HOME" | awk 'NR==2 {printf "%.0f", $4/1024/1024}')
echo "  free disk: ${FREE_GB} GB   (need ~40 GB with models, ~18 GB without)"

DRIVER=""
if command -v nvidia-smi >/dev/null 2>&1; then
    DRIVER=$(nvidia-smi --query-gpu=driver_version --format=csv,noheader | head -1 | tr -d ' ')
    VRAM=$(nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits | head -1 | tr -d ' ')
    CC=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader | head -1 | tr -d ' ')
    echo "  GPU: driver ${DRIVER}, ${VRAM} MiB, compute capability ${CC}"
    case "${DRIVER%%.*}" in
        5[8-9][0-9]|6[0-9][0-9]) CUDA_INDEX=cu130 ;;
        57[0-9])                  CUDA_INDEX=cu128 ;;
        5[5-6][0-9])              CUDA_INDEX=cu126 ;;
        *)                        gap "driver ${DRIVER} is too old for the current wheels; install a 570+ driver first" ;;
    esac
    echo "  torch wheel index: ${CUDA_INDEX}"
else
    gap "nvidia-smi not found - install the NVIDIA driver before running setup"
fi

if [ "$WITH_MODELS" = 1 ] && [ "$FREE_GB" -lt 40 ]; then
    gap "not enough free disk for models (~40 GB); re-run with --no-models to skip them"
fi
ok "probes"

# ---------------------------------------------------------------- phase 1 -----
step "Phase 1 - ComfyUI ${COMFY_TAG} in ${COMFY_DIR}"
if [ -f "$COMFY_DIR/main.py" ]; then
    echo "  existing checkout kept: $(git -C "$COMFY_DIR" describe --tags 2>/dev/null || echo unknown)"
else
    run mkdir -p "$(dirname "$COMFY_DIR")"
    run git clone --quiet https://github.com/comfyanonymous/ComfyUI.git "$COMFY_DIR"
    run git -C "$COMFY_DIR" checkout --quiet "$COMFY_TAG"
fi

PY="$COMFY_DIR/venv/bin/python"
PIP="$COMFY_DIR/venv/bin/pip"
if [ ! -x "$PY" ]; then
    step "Phase 1b - virtualenv"
    run python3 -m venv "$COMFY_DIR/venv"
fi
[ "$DRY" = 1 ] || { [ -x "$PY" ] || gap "venv was not created"; }

step "Phase 1c - python packages"
if [ "$DRY" = 1 ]; then
    echo "  would install requirements (minus comfyui-workflow-templates-media-assets-02,"
    echo "  which does not exist on PyPI), then torch for ${CUDA_INDEX}, then pin cudnn"
else
    grep -v '^comfyui-workflow-templates' "$COMFY_DIR/requirements.txt" > /tmp/req-nomedia.txt
    "$PIP" install -q --upgrade pip wheel
    "$PIP" install -q -r /tmp/req-nomedia.txt || gap "pip install of ComfyUI requirements failed"
    "$PIP" install -q --force-reinstall --index-url "https://download.pytorch.org/whl/${CUDA_INDEX}" \
        torch torchvision torchaudio || gap "torch install failed for ${CUDA_INDEX}"

    # cuDNN must match what torch was built against or VAE decode dies with
    # CUDNN_STATUS_SUBLIBRARY_VERSION_MISMATCH.
    WANT_CUDNN=$(python3 - "$PY" <<'PY'
import re, subprocess, sys
py = sys.argv[1]
out = subprocess.run([py, "-m", "pip", "show", "torch"], capture_output=True, text=True).stdout
m = re.search(r"Requires:.*", out)
meta = subprocess.run([py, "-c", "import importlib.metadata as m;print([d for d in m.requires('torch') if 'cudnn' in d][0])"],
                      capture_output=True, text=True).stdout.strip()
print(meta.split("==")[-1].split(";")[0].strip())
PY
)
    if [ -n "${WANT_CUDNN:-}" ]; then
        "$PIP" install -q "nvidia-cudnn-cu13==${WANT_CUDNN}" || gap "could not pin cudnn ${WANT_CUDNN}"
        echo "  cudnn pinned to ${WANT_CUDNN}"
    fi

    "$PY" -c "import torch;print('  torch', torch.__version__, 'cuda', torch.version.cuda, 'available', torch.cuda.is_available())" \
        || gap "torch cannot import"
fi
ok "python environment"

# ---------------------------------------------------------------- phase 2 -----
step "Phase 2 - ComfyUI-GGUF + Qwen-Image-2.1 architecture patch"
NODE_DIR="$COMFY_DIR/custom_nodes/ComfyUI-GGUF"
if [ ! -d "$NODE_DIR" ]; then
    run git clone --quiet https://github.com/city96/ComfyUI-GGUF "$NODE_DIR"
fi
if [ "$DRY" = 0 ]; then
    "$PIP" install -q -r "$NODE_DIR/requirements.txt" || gap "ComfyUI-GGUF requirements failed"
    if grep -q "class ModelQwenImage21" "$NODE_DIR/tools/convert.py"; then
        echo "  arch patch already applied"
    else
        git -C "$NODE_DIR" apply "$ROOT/patches/comfyui-gguf-qwen-image-2.1.patch" \
            || gap "could not apply the GGUF architecture patch - see patches/README.md"
        echo "  arch patch applied"
    fi
fi
ok "GGUF node"

# ---------------------------------------------------------------- phase 3 -----
step "Phase 3 - model weights (${MODEL_DIR})"
if [ "$WITH_MODELS" = 0 ]; then
    echo "  skipped (--no-models)"
else
    mkdir -p "$MODEL_DIR"/diffusion_models "$MODEL_DIR"/text_encoders "$MODEL_DIR"/vae
    python3 "$ROOT/patches/fetch_models.py" --manifest "$ROOT/models.json" \
        --vault "$MODEL_DIR" --mirror "$MIRROR" $( [ "$DRY" = 1 ] && echo --dry-run ) \
        || gap "model download/verification failed - re-run to resume"
    mkdir -p "$COMFY_DIR"/models/{diffusion_models,text_encoders,vae}
    python3 "$ROOT/patches/fetch_models.py" --manifest "$ROOT/models.json" \
        --vault "$MODEL_DIR" --link-into "$COMFY_DIR/models" \
        || gap "could not create the model symlinks"
fi
ok "weights"

# ---------------------------------------------------------------- phase 4 -----
step "Phase 4 - build the UI"
if [ "$DRY" = 1 ]; then
    echo "  would run harness/build-linux.sh"
elif ! command -v qmake >/dev/null 2>&1 && ! command -v qmake6 >/dev/null 2>&1; then
    gap "qmake not found (apt install qtbase5-dev qttools5-dev-tools build-essential)"
else
    ( cd "$ROOT/harness" && ./build-linux.sh ) || gap "UI build failed"
    ( cd "$ROOT/harness" && ./install.sh --no-build ) || gap "launcher install failed"
fi
ok "UI built and installed"

# ---------------------------------------------------------------- phase 5 -----
cat <<EOF

=== Next

  qwen-image-studio                       # the app
  qwen-image-studio --run "a brass key"   # headless smoke test

  Settings defaults already point at ${COMFY_DIR} and port 8188; change them in
  the app if you moved things. Outputs land in ~/Pictures/Qwen-Image-Studio.

  Not done for you: the licence. Qwen-Image-2.1 weights are research/evaluation
  only until you obtain a commercial licence from Qwen.
EOF
