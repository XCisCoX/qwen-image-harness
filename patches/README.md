# patches/

## `comfyui-gguf-qwen-image-2.1.patch`

Applies to [city96/ComfyUI-GGUF](https://github.com/city96/ComfyUI-GGUF) (GPL-3.0).
Adds a `ModelQwenImage21` template to `tools/convert.py` so GGUF files without a
`general.architecture` field are recognised as Qwen-Image DiTs.

Without it, `UnetLoaderGGUF` fails with `Unknown model architecture` for both community
Qwen-Image-2.1 GGUFs, because their converters write no metadata.

```bash
git -C "$COMFY_DIR/custom_nodes/ComfyUI-GGUF" apply patches/comfyui-gguf-qwen-image-2.1.patch
```

If it stops applying, re-add the equivalent block: a template with

```python
arch = "qwen_image"
keys_detect = [("txt_in.text_norm.weight", "modulation.1.weight",
                "transformer_blocks.0.attn.norm_q.weight")]
```

appended to `arch_list`. Reporting arch `qwen_image` is deliberate: ComfyUI's own
`model_detection.py` then distinguishes 2.1 from 2.0 by tensor keys.

## `fetch_models.py`

Downloads and verifies the weights in `models.json`, then creates the file-level symlinks
ComfyUI needs. Uses `aria2c -x16` against a mirror (the Hub throttles hard on some
networks), checks every file's sha256, and never overwrites or deletes an existing file
that already verifies.

```bash
python3 patches/fetch_models.py --manifest models.json --vault ~/Models
python3 patches/fetch_models.py --manifest models.json --vault ~/Models --link-into ~/git/ComfyUI/models
```

## Not patched, but worth knowing

* **cuDNN** must match the version torch was built against, or VAE decode dies with
  `CUDNN_STATUS_SUBLIBRARY_VERSION_MISMATCH`. `setup.sh` queries torch's own requirement
  and pins it; do the same if you install by hand (AGENT.md §3).
* **`requirements.txt`** references `comfyui-workflow-templates-media-assets-02`, which
  does not exist on PyPI. Filter that line out before installing (AGENT.md §3).
* **`--fp16-unet`** is not optional on Turing: without it the DiT runs bf16 with an fp32
  cast and is ~3.8× slower. The app always passes it.
