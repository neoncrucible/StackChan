#!/usr/bin/env python3
"""Verify a PHOSPHOR package, back up a 16 MiB CoreS3, then flash segments."""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import re
import subprocess
import sys

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="Robot serial port, e.g. COM7 or /dev/ttyACM0")
    parser.add_argument("--backup-dir", type=Path, default=Path("backups"))
    parser.add_argument("--dry-run", action="store_true", help="Verify files and print commands; no serial access")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    manifest = json.loads((root / "manifest.json").read_text())
    if manifest["chip"] != "esp32s3" or manifest["flash_bytes"] != 16 * 1024 * 1024:
        raise SystemExit("Unexpected firmware target")
    segments = sorted(manifest["segments"], key=lambda x: int(x["offset"], 0))
    previous_end = 0
    for entry in segments:
        path = (root / entry["file"]).resolve()
        if root not in path.parents:
            raise SystemExit("Invalid package path")
        data = path.read_bytes()
        if len(data) != entry["bytes"] or hashlib.sha256(data).hexdigest() != entry["sha256"]:
            raise SystemExit(f"Checksum/size mismatch: {entry['file']}")
        start = int(entry["offset"], 0)
        if start < previous_end or start + len(data) > manifest["flash_bytes"]:
            raise SystemExit("Overlapping or oversized flash segments")
        # Stock NVS contains credentials and calibration. Do not write it.
        if start < 0xd000 and start + len(data) > 0x9000:
            raise SystemExit("Package would overwrite the stock NVS region")
        previous_end = start + len(data)
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    port_name = re.sub(r"[^A-Za-z0-9_-]", "_", args.port)
    backup = args.backup_dir.resolve() / f"stackchan-before-phosphor-{port_name}-{stamp}.bin"
    common = [sys.executable, "-m", "esptool", "--chip", "esp32s3", "--port", args.port, "--baud", "460800"]
    identity = common + ["flash_id"]
    read = common + ["read_flash", "0x0", "0x1000000", str(backup)]
    write = common + ["write_flash", "--flash_mode", "dio", "--flash_size", "16MB", "--flash_freq", "80m"]
    for entry in segments:
        write.extend([entry["offset"], str(root / entry["file"])])
    if args.dry_run:
        for command in (identity, read, write):
            print(subprocess.list2cmdline(command))
        print("Package verified. Dry run: no device was accessed.")
        return
    result = subprocess.run(identity, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
    print(result.stdout)
    if "Detected flash size: 16MB" not in result.stdout:
        raise SystemExit("Expected 16MB flash; refusing to continue")
    backup.parent.mkdir(parents=True, exist_ok=True)
    if backup.exists():
        raise SystemExit("Backup already exists; refusing to overwrite it")
    subprocess.run(read, check=True)
    if backup.stat().st_size != manifest["flash_bytes"]:
        raise SystemExit("Incomplete backup; firmware was not written")
    print(f"Full backup saved: {backup}")
    subprocess.run(write, check=True)
    print("PHOSPHOR written. Keep the backup; complete the single acceptance pass in FLASH.md.")

if __name__ == "__main__":
    main()
