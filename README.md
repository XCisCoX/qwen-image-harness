# Qwen Image Harness

A desktop frontend for **Qwen-Image-2.1** on a consumer GPU: generate, edit, face-swap
and mask-infill, browse results, watch the engine's log. Qt/C++, no web stack, one binary.

The interesting part is not the UI — it is the **seam**. The app talks to an abstract
`Backend`; a reference client for a local ComfyUI ships with it, and swapping in your own
service is a one-line change in `harness/BackendFactory.cpp`. If you have your own
harness or API subscription, wire it there and keep the entire interface.

![rendered with this app, 8 steps at 1024x1024](docs/sample.png)

## What it does

| Page | What it is for |
|---|---|
| Generate | prompt → image, 1–16 reference images |
| Edit | change a photo with instructions, optional extra references |
| Face swap | identity transfer, same request shape as edit |
| Mask infill | paint a mask, re-render only that area — from scratch or keeping the original |
| Gallery | everything in the output folder, newest first |
| Terminal | the engine's log and this app's own backend messages |
| Settings | paths, server command, defaults, retention, accent colour |

Also: **on-the-fly scaling** of the primary image (defaults to ~1.0 MP, keeps aspect),
**a prompt queue** (submit as many as you like, they run in order), **optional live
preview** of the sampler, batch, seed lock,
per-run JSON sidecars, and a headless mode for scripting.

Unofficial community project. Not affiliated with, endorsed by, or supported by Alibaba or
the Qwen team; "Qwen" is their trademark and is used here only to say what the app runs.

## Requirements

* Linux with a CUDA GPU and a recent NVIDIA driver (validated: RTX 2060 SUPER 8 GB,
  Turing, driver 595, Ubuntu 24.04). 8 GB is the floor for the quantised stack.
* Qt 5.15 (or Qt 6) + a C++11 compiler — `qtbase5-dev qttools5-dev-tools build-essential`.
* Python 3.10+ with `python3-venv`.
* ~18 GB disk without weights, ~40 GB with them.

The model weights are **not** included. They are downloaded from Hugging Face under the
Qwen Research License (research/evaluation only) — see `models.json`.

## Install

Deterministic path:

```bash
./setup.sh            # venv, ComfyUI v0.39.2, GGUF patch, weights, UI build, launcher
qwen-image-harness
```

Or hand the repository to an agent and point it at **AGENT.md** — it is written as a
gated procedure (probe → install → verify) with the failure table this stack earned the
hard way.

Headless smoke test:

```bash
qwen-image-harness --run "a brass key on dark slate" --steps 8 --out /tmp/out
qwen-image-harness --run "add a small flower here" --mode mask \
    --image photo.png --mask mask.png --out /tmp/out
```

## Backend seam (the "soldering pad")

```
harness/Backend.h              the contract: startServer / stopServer / submit / cancel
                               + stateChanged / progress / jobFinished / jobFailed
harness/BackendFactory.cpp     return your implementation here
harness/ComfyBackend.cpp       reference client: local ComfyUI over HTTP
harness/MockBackend.cpp        simulator: full UI, no GPU, no model
harness/docs/BACKEND.md        the request/result shapes and the rules
```

`GenerationRequest` carries prompt, negative, size, steps, cfg, batch, sampler, seed,
an ordered image list (`images[0]` is the primary) and an optional mask path. That is
everything the four workflows collect — a new backend needs nothing else.

## Mask infill options

Two controls on the Mask infill page decide what the model knows and what it sees:

* **Regenerate the masked area from scratch** (default on). The masked pixels are replaced
  with the average colour of the surrounding ring in the reference the model receives, so
  it cannot copy the shape or detail that was there - the Fooocus behaviour, minus the halo
  a flat grey fill causes against a hard-edged background. Unchecked, the original content stays visible and the model
  tends to recolour or nudge it instead of replacing it.
* **Fill: Matched colour / Noise.** What the masked pixels become before the model sees
  them. Matched colour is a flat fill sampled from the ring around the mask; Noise is the
  Fooocus behaviour, noise around that tone, which gives the model a texture to start
  from (fuzzier results, sometimes better fur/skin).
* **Context: Area + surroundings / Whole image.** Cropping a window around the mask keeps
  detail high (the crop is rendered at ~1024 px); whole-image mode hands over the entire
  frame at native scale, at the cost of detail on a small mask.

The sampler is also allowed to repaint ~12 px beyond the painted area (the way Fooocus
does it) while only the painted pixels are pasted back, so the model can blend its own
edge into the surroundings.

Headless equivalents: `--inpaint-keep`, `--inpaint-fill colour|noise`, `--inpaint-grow N`,
`--inpaint-context full|area`.

## Mask infill, briefly

A hard cut-and-paste leaves a seam and lets existing geometry drift. The reference client
instead crops around the mask with context, upscales the crop, re-samples **only** the
masked latents, then blends back with a few pixels of feather along the painted edge.
Measured on a pole with hard edges: 0 changed pixels outside `mask + 14 px`, pole edges
bit-identical above and below the edit. `AGENT.md` §9 has the pipeline and §7 the test.

## Status

Working, verified on the hardware above: generate (42 s), base 25-step (88 s), edit (56 s),
multi-reference composition, RGBA output, mask infill with the invariants above. Known
gaps: 2048² does not fit (VAE decode needs ~7 GB), Settings → Apply does not re-apply
defaults to already-open pages, and batch results arrive as separate frames.

## Licence

Code: MIT (see `LICENSE`), with one carve-out — `patches/comfyui-gguf-qwen-image-2.1.patch`
is a modification of GPL-3.0 code (ComfyUI-GGUF) and stays GPL-3.0. Everything else in
`harness/`, `setup.sh`, `AGENT.md` and the docs is MIT. Third-party components are **not**
bundled — see `NOTICE`.
Model weights are downloaded by the user and remain under the Qwen Research License.
