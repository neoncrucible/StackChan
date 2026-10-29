# Validation of the single PHOSPHOR candidate

Both stock and PHOSPHOR were compiled locally with ESP-IDF **v5.5.4**, Xtensa GCC **14.2.0**, the CoreS3 configuration and the same resolved dependencies. Stock source: `f901cc2969189f3f0e96af1442c0f96e77e1e2c1`. PHOSPHOR's dependency commits are in `firmware/repos.json`; registry versions are in `firmware/dependencies.lock`.

| Build result | Stock | PHOSPHOR | Difference |
|---|---:|---:|---:|
| Application binary | 3,730,352 B | 3,467,408 B | **262,944 B smaller (7.05%)** |
| Link-time DIRAM occupied | 210,111 B | 189,187 B | **20,924 B less (20.4 KiB)** |
| Link-time DIRAM available | 131,649 B | 152,573 B | 20,924 B more |
| Shared assets image | 2,298,029 B | 2,298,029 B | Unchanged |
| Image cache limit on 8 MiB board | 2 MiB | 64 KiB | Configured budget reduced |

The application fits its 0x4f0000-byte slot with **33% free**. Link-time available memory is not a runtime heap measurement. The cache limit is not a claim of permanently allocated memory. No response-latency, physical smoothness, battery-life or long-run reliability result is asserted.

**Four focused host checks pass:** existing motion math, PHOSPHOR face bounds/audio envelope/stale-meter handling, and the real UART source under timed fakes at 100 Hz and 1,000 Hz. These cover missing replies, partial/complete packets, request versus inter-byte deadlines, invalid/discard reads, PCM extremes, and timestamp wrap. The same face geometry was rendered and visually inspected in `phosphor-avatar.png`. Required dependency patches were checked idempotently; `git diff --check` passes.

The native ESP-IDF build and its partition-size checks pass. `CONFIG_PHOSPHOR_MANUAL_UPDATES=y`, `CONFIG_LV_FONT_UNSCII_8=y`, ESP32-S3, 16 MiB flash and PSRAM were checked in the effective configuration. The camera constructor is not invoked; camera MCP registration is conditional on the null camera pointer. This is source verification, not a hardware memory measurement.

The local build environment required a compatibility fallback in Component Manager's PID lookup because its `psutil` view and process namespace differed. The fallback uses the documented Linux parent PID. It is outside the firmware repository and does not alter the compiled firmware sources. Normal Linux/GitHub builds do not need it.

**Hardware acceptance remains open.** No robot serial port is attached to this environment, so the binary has not been flashed or physically exercised here. In one session: verify boot/provisioning, saved settings, a full reply, tap-to-interrupt, gentle head movement during audio, one network interruption/recovery, and stock-versus-PHOSPHOR first-audio timing. Codec-fault retry and the TCP teardown failure path also need real device fault testing before a production reliability claim.

There is one candidate version, **1.0.0-phosphor**. No GitHub Release is automatically published. CI reproduces the same focused checks and produces a build artifact; its eventual result is distinct from these completed local checks.
