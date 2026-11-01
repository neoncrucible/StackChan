#!/usr/bin/env python3
"""Package verified IDF build outputs without writing a padded image over NVS."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-sha", help="Published source commit when packaging outside its checkout")
    args = parser.parse_args()
    revision = args.source_sha or subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    build = ROOT / "firmware/build"
    flash = json.loads((build / "flasher_args.json").read_text())
    output = ROOT / "dist/PHOSPHOR-CoreS3"
    output.mkdir(parents=True, exist_ok=True)
    segments = []
    for offset, relative in sorted(flash["flash_files"].items(), key=lambda x: int(x[0], 0)):
        source = build / relative
        target = output / source.name
        shutil.copy2(source, target)
        data = target.read_bytes()
        segments.append({"offset": offset, "file": target.name, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()})
    manifest = {"name": "PHOSPHOR", "version": "1.1.0-phosphor", "chip": "esp32s3",
                "hardware": "M5Stack StackChan K151 / CoreS3", "flash_bytes": 16 * 1024 * 1024,
                "source_commit": revision, "source_url": f"https://github.com/neoncrucible/StackChan/tree/{revision}",
                "idf": "v5.5.4", "hardware_tested": False, "segments": segments}
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (output / "SHA256SUMS").write_text("".join(f"{x['sha256']}  {x['file']}\n" for x in segments))
    shutil.copy2(ROOT / "tools/flash.py", output / "flash.py")
    shutil.copy2(ROOT / "docs/FLASH.md", output / "FLASH.md")
    shutil.copy2(ROOT / "THIRD_PARTY.md", output / "THIRD_PARTY.md")
    shutil.copy2(ROOT / "docs/COMPANION.md", output / "COMPANION.md")
    shutil.copy2(ROOT / "docs/PERSONALITY.md", output / "PERSONALITY.md")
    # Licence texts travel with the binary distribution. Source stays in Git.
    licence_roots = [("registry", ROOT / "firmware/managed_components"),
                     ("components", ROOT / "firmware/components"),
                     ("xiaozhi", ROOT / "firmware/xiaozhi-esp32"),
                     ("board", ROOT / "firmware/main")]
    if os.environ.get("IDF_PATH"):
        licence_roots.append(("esp-idf", Path(os.environ["IDF_PATH"])))
    for label, folder in licence_roots:
        for path in folder.rglob("*"):
            if not path.is_file() or ".git" in path.parts:
                continue
            name = path.name.lower()
            if name.startswith(("license", "licence")) or name in ("copying", "notice", "copyright"):
                destination = output / "licenses" / label / path.relative_to(folder)
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(path, destination)
    subprocess.run(["python3", str(output / "flash.py"), "--port", "DRY_RUN", "--dry-run"], check=True)
    archive = ROOT / "dist/PHOSPHOR-CoreS3.zip"
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as target:
        for path in sorted(output.rglob("*")):
            if path.is_file():
                target.write(path, path.relative_to(output))
    print(f"Created {archive} ({archive.stat().st_size:,} bytes)")

if __name__ == "__main__":
    main()
