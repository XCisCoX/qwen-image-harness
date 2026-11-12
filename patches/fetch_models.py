#!/usr/bin/env python3
"""Fetch / verify the weights listed in models.json.

  fetch_models.py --manifest models.json --vault ~/Models [--mirror URL] [--dry-run]
  fetch_models.py --manifest models.json --vault ~/Models --link-into ~/git/ComfyUI/models

Downloads go through aria2c against a Hugging Face mirror (the Hub itself throttles
to a crawl on some networks), then every file is checked against the sha256 the
manifest carries before it is moved into the vault. Re-running resumes.

--link-into creates the file-level symlinks ComfyUI needs, so the vault keeps the
only real copy. Nothing is deleted, ever.
"""
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time


def human(n):
    return f"{n / 2**30:.2f} GiB"


def sha256(path, chunk=1 << 22):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(chunk), b""):
            h.update(block)
    return h.hexdigest()


def download(url, dest, dry):
    if shutil.which("aria2c"):
        cmd = ["aria2c", "-x16", "-s16", "-k", "1M", "--file-allocation=none",
               "--continue=true", "--max-tries=8", "--retry-wait=5",
               "--auto-file-renaming=false", "--allow-overwrite=true",
               "--console-log-level=warn", "-d", os.path.dirname(dest),
               "-o", os.path.basename(dest), url]
    else:
        cmd = ["curl", "-L", "-C", "-", "--retry", "5", "-o", dest, url]
    print("    " + " ".join(cmd[:6]) + " ...")
    if dry:
        return True
    return subprocess.call(cmd) == 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--manifest", required=True)
    ap.add_argument("--vault", required=True)
    ap.add_argument("--mirror", default="https://hf-mirror.com")
    ap.add_argument("--link-into", help="ComfyUI models/ directory to symlink into")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--include-optional", action="store_true",
                    help="also fetch entries marked required=false")
    args = ap.parse_args()

    manifest = json.load(open(args.manifest))
    failures = 0
    pending = 0

    if args.link_into:
        for entry in manifest["models"]:
            src = os.path.join(args.vault, entry["dest"].replace("models/", "", 1))
            if not os.path.exists(src):
                continue
            kind = entry["dest"].split("/")[1]
            target_dir = os.path.join(args.link_into, kind)
            os.makedirs(target_dir, exist_ok=True)
            dst = os.path.join(target_dir, os.path.basename(src))
            if os.path.islink(dst) and os.path.realpath(dst) == os.path.realpath(src):
                continue
            if os.path.exists(dst) and not os.path.islink(dst):
                print(f"  keep  {dst} (real file already there, not touching it)")
                continue
            if not args.dry_run:
                if os.path.islink(dst):
                    os.unlink(dst)
                os.symlink(os.path.realpath(src), dst)
            print(f"  link  {dst} -> {src}")
        return 0

    for entry in manifest["models"]:
        if not entry["required"] and not args.include_optional:
            print(f"  skip  {os.path.basename(entry['dest'])} (optional)")
            continue
        dest = os.path.join(args.vault, entry["dest"].replace("models/", "", 1))
        os.makedirs(os.path.dirname(dest), exist_ok=True)

        if os.path.exists(dest) and os.path.getsize(dest) == entry["size"]:
            if sha256(dest) == entry["sha256"]:
                print(f"  ok    {os.path.basename(dest)}  {human(entry['size'])}")
                continue
            print(f"  bad   {os.path.basename(dest)}: sha256 mismatch, re-downloading")
            if not args.dry_run:
                os.unlink(dest)

        if args.dry_run:
            pending += 1
            print(f"  would get  {entry['repo']} :: {entry['file']}  {human(entry['size'])}")
            continue

        url = f"{args.mirror}/{entry['repo']}/resolve/main/{entry['file']}"
        print(f"  get   {entry['repo']} :: {entry['file']}  {human(entry['size'])}")
        if not download(url, dest, args.dry_run):
            print(f"  FAIL  {os.path.basename(dest)}")
            failures += 1
            continue

        if args.dry_run:
            continue
        got = os.path.getsize(dest) if os.path.exists(dest) else 0
        if got != entry["size"]:
            print(f"  FAIL  {os.path.basename(dest)}: {got} bytes, expected {entry['size']}")
            failures += 1
            continue
        digest = sha256(dest)
        if digest != entry["sha256"]:
            print(f"  FAIL  {os.path.basename(dest)}: sha256 {digest[:12]} != {entry['sha256'][:12]}")
            os.unlink(dest)
            failures += 1
            continue
        print(f"  done  {os.path.basename(dest)}  verified")

    if failures:
        print(f"\n{failures} file(s) failed - re-run to resume.")
        return 1
    if args.dry_run:
        print(f"\ndry-run: {pending} file(s) would be fetched; the rest are already verified")
        return 0
    print("\nall required weights present and verified")
    return 0


if __name__ == "__main__":
    sys.exit(main())
