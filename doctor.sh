#!/usr/bin/env bash
# One-screen health check for the Qwen-Image-2.1 stack.
#   ./doctor.sh
# Exit 0 = everything the API can see; 1 = something is missing.
set -uo pipefail

COMFY="${COMFY_DIR:-$HOME/git/ComfyUI}"
SERVER="${COMFY:-http://127.0.0.1:8188}"
SERVER="${COMFY_SERVER:-http://127.0.0.1:8188}"
VAULT="$HOME/Models"
fail=0
ok()   { printf '[ok]   %s\n' "$*"; }
bad()  { printf '[FAIL] %s\n' "$*"; fail=$((fail+1)); }
warn() { printf '[warn] %s\n' "$*"; }

echo "== files"
for f in \
  "$VAULT/diffusion_models/Qwen-Image-2.1/qwen-image-2.1-turbo-Q4_K_M.gguf" \
  "$VAULT/diffusion_models/Qwen-Image-2.1/qwen_image_2.1-Q4_K_M.gguf" \
  "$VAULT/text_encoders/Qwen-Image-2.1/qwen3vl_8b_w4a8.safetensors" \
  "$VAULT/text_encoders/Qwen-Image-2.1/qwen3vl_8b_w4a8_heretic.safetensors" \
  "$VAULT/vae/Qwen-Image-2.1/qwen_image_2.1_vae_bf16.safetensors" ; do
    [ -f "$f" ] && ok "$(basename "$f") $(du -h "$f" | cut -f1)" || bad "missing $f"
done

echo "== symlinks (ComfyUI -> vault)"
broken=$(find "$COMFY/models" -type l ! -exec test -e {} \; -print 2>/dev/null)
[ -z "$broken" ] && ok "no broken links" || { echo "$broken"; bad "broken symlinks"; }
for f in diffusion_models/qwen-image-2.1-turbo-Q4_K_M.gguf text_encoders/qwen3vl_8b_w4a8.safetensors vae/qwen_image_2.1_vae_bf16.safetensors; do
    [ -e "$COMFY/models/$f" ] && ok "linked $f" || bad "not linked: $f"
done

echo "== install"
v=$(git -C "$COMFY" describe --tags 2>/dev/null || echo unknown)
case "$v" in v0.3[7-9]*|v0.[4-9]*) ok "ComfyUI $v (>= v0.37 needed for Qwen-Image-2.1)";; *) bad "ComfyUI $v is too old (need >= v0.37)";; esac
"$COMFY/venv/bin/python" -c "import torch,sys; sys.exit(0 if torch.cuda.is_available() else 1)" 2>/dev/null \
    && ok "torch $(  "$COMFY/venv/bin/python" -c 'import torch;print(torch.__version__, torch.version.cuda)') CUDA visible" \
    || bad "torch cannot see CUDA"
grep -q "class ModelQwenImage21" "$COMFY/custom_nodes/ComfyUI-GGUF/tools/convert.py" 2>/dev/null \
    && ok "GGUF arch patch present" || bad "GGUF arch patch missing (UnetLoaderGGUF will fail)"

echo "== server"
if ! curl -sf "$SERVER/system_stats" -o /tmp/_ss.json; then
    warn "server not reachable at $SERVER - start it with ./run-server.sh"
else
    python3 - "$SERVER" <<'PY' || fail=$((fail+1))
import json, sys, urllib.request
s = json.load(urllib.request.urlopen(sys.argv[1] + "/system_stats"))
d = s["devices"][0]
print(f"[ok]   {d['name']}  {d['vram_total']/2**20:.0f} MiB  torch {s['system']['pytorch_version']}")
try:
    inf = json.load(urllib.request.urlopen(sys.argv[1] + "/object_info/UnetLoaderGGUF"))
    names = inf["UnetLoaderGGUF"]["input"]["required"]["unet_name"][0]
    q = [n for n in names if n.startswith("qwen")]
    print("[ok]   GGUF loader sees: " + (", ".join(q) if q else "NO qwen GGUFs"))
    if not q: raise SystemExit(1)
except Exception as e:
    print("[FAIL] GGUF loader check:", e); raise SystemExit(1)
for kind in ("text_encoders", "vae"):
    lst = json.load(urllib.request.urlopen(sys.argv[1] + f"/models/{kind}"))
    q = [n for n in lst if "qwen" in n.lower()]
    print(f"[ok]   {kind}: " + (", ".join(q) if q else "none"))
PY
fi

echo
[ "$fail" -eq 0 ] && echo "all good" || echo "$fail problem(s)"
exit $([ "$fail" -eq 0 ] && echo 0 || echo 1)
