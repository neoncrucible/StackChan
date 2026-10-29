# PHOSPHOR — one candidate, one acceptance pass

Target: **M5Stack StackChan K151 with CoreS3, ESP32-S3, 16 MB flash**. This is not for Core2, AtomS3R or a bare CoreS3 without the StackChan servo base. Candidate version: **1.0.0-phosphor**.

The firmware compiled successfully and passed its focused host checks. It has **not been physically tested on your robot**. No device is attached to the build environment.

## Flash

Extract the ZIP, connect the robot by USB, and open a terminal in the extracted folder. Close any serial monitor. Replace `COM7` with the robot's port (`/dev/ttyACM0` on Linux, or the corresponding `/dev/cu.usbmodem...` on macOS).

```sh
python -m pip install esptool==4.12.0
python flash.py --port COM7
```

The helper verifies every image's SHA-256, checks ESP32-S3/16 MB flash, reads a **full backup first**, then writes the five build-defined segments. It stops if backup or validation fails. The backup stays on your computer; it is never uploaded. Do not delete it until you are satisfied with the new build.

There is no blanket erase and no padded merged image. The stock NVS region, `0x9000–0xcfff`, is left untouched. That preserves stock credentials/calibration when the previous firmware used the same layout. A different old layout may require setup again. The bootloader, partition table, OTA selection, application and assets are replaced. The full backup remains the way back to the exact previous image.

To inspect commands without touching a device:

```sh
python flash.py --port COM7 --dry-run
```

## First use

PHOSPHOR keeps stock StackChan provisioning and compatible cloud activation. Existing valid configuration should be reused; otherwise use the usual StackChan app setup flow. After configuration, the voice app starts by default. Setup remains available through the on-screen home control. No local Kadence service, Docker stack or computer voice gateway is required.

The avatar is generated on-device. Tap the face to start/interrupt a conversation. The stock wake-word path is retained. Head motion is local, restrained by narrower software limits, and keeps the original pitch stall protection. Automatic provider firmware replacement is disabled so the custom face is not silently overwritten.

Open-ended speech still needs Wi-Fi and a compatible cloud service. This is not an offline English assistant or an included API subscription.

## One acceptance pass

1. Confirm a clean boot, sensible head position, usable touch controls and Wi-Fi/cloud activation.
2. Ask a few short questions and one longer question. Confirm the first words are audible, the mouth follows speech and playback ends cleanly.
3. Tap during a reply. Confirm interruption and return to a usable listening/idle state.
4. Watch several gentle head movements, including during voice activity. Stop if there is binding, repeated hard stops or abnormal buzzing.
5. Interrupt Wi-Fi once and restore it. Confirm the device remains usable and can converse again.
6. Compare the same short questions against stock on the same backend/network if response speed is still in doubt. Measure the gap from the end of your speech to the first audible reply; note audio dropouts separately.

There is no need for a sequence of tiny releases. Keep this candidate intact and record the results of this one session.

## Restore the previous image

Use the backup made by the helper, substituting its actual filename and your port:

```sh
python -m esptool --chip esp32s3 --port COM7 --baud 460800 write_flash 0x0 backups/stackchan-before-phosphor-YOUR-BACKUP.bin
```

The backup contains device settings and credentials; keep it private. Source and the full research report are at https://github.com/neoncrucible/StackChan/tree/phosphor/fresh-start. The package manifest records the exact source commit and image hashes.
