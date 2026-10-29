#!/usr/bin/env python3
"""Fetch exact dependency commits; a missing patch is a build failure."""
from pathlib import Path
import json
import subprocess

ROOT = Path(__file__).resolve().parent

def git(path, *args, check=True):
    return subprocess.run(["git", "-C", str(path), *args], check=check,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

def main():
    for dependency in json.loads((ROOT / "repos.json").read_text()):
        path = ROOT / dependency["path"]
        revision = dependency["branch"]
        if not path.exists():
            path.mkdir(parents=True)
            git(path, "init")
            git(path, "remote", "add", "origin", dependency["url"])
        if git(path, "cat-file", "-e", revision + "^{commit}", check=False).returncode:
            git(path, "fetch", "--depth=1", "origin", revision)
        current = git(path, "rev-parse", "HEAD", check=False)
        if current.returncode or current.stdout.strip() != revision:
            # Do not discard a developer's edits when moving a dependency.
            if git(path, "status", "--porcelain").stdout.strip():
                raise RuntimeError(f"{path}: local edits block the pinned checkout")
            git(path, "checkout", "--detach", revision)
        if dependency.get("with_submodules"):
            git(path, "submodule", "update", "--init", "--recursive")
        if dependency.get("patch"):
            patch = str(ROOT / dependency["patch"])
            if git(path, "apply", "--check", patch, check=False).returncode == 0:
                git(path, "apply", patch)
            elif git(path, "apply", "--reverse", "--check", patch, check=False).returncode:
                raise RuntimeError(f"{path}: required patch neither applies nor is already applied")
        print(f"Ready: {dependency['path']} @ {revision[:12]}")

if __name__ == "__main__":
    main()
