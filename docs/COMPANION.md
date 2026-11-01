# PHOSPHOR 1.1 — companion update

One update for the existing CoreS3. No games, camera, external sensors or home server. The voice transport/codec and audio scheduling are unchanged while the reported crackle is being compared against stock and different voices.

The existing robot/UI task reserves an extra 2 KiB of stack for reminder storage calls. There is no new task, large framebuffer, model or partition. The idle panel updates once a second; reminder and alert capacities are bounded.

## What changes

- **Startup and home:** green terminal PHOSPHOR boot identity and a simple two-button conversation/settings home screen. The old carousel and bouncing screensaver are removed. Provisioning/settings still use the stock setup workflow.
- **Idle display:** after eight seconds in standby, the caption area shows local time, battery/charging state and the next reminder. It updates at most once a second, leaving the face intact. It returns to captions for conversation. The top status controls and bottom home control remain.
- **Reminders:** tap REMINDERS at the lower left while idle. Add 5-minute, 15-minute or 1-hour timers; view saved entries and cancel them. Named reminders and absolute times use the existing cloud tool path (e.g. “Remind me in 15 minutes to check the oven”). Each saved reminder has a stable ID, a due time and an optional repeat interval. Save errors are reported. Limit: eight reminders, 120 UTF-8 bytes of text each.
- **Persistence:** one versioned NVS record, written only on change. No partition change or NVS erase. A valid clock is required; retained RTC time works offline. After reboot an overdue reminder alerts once; repeating reminders skip missed occurrences. Repeat intervals are at least one minute. A bounded alert queue coalesces repeats while their previous alert remains visible. Alerts also appear on home/settings screens. In conversation mode a local chime waits for idle so it does not mix with a reply; outside conversation mode the alert is visual. There is no generated speech. Snooze adds a new 5-minute reminder; DONE dismisses the alert.
- **Power:** saved reminders suppress automatic idle power-off. The screen may still sleep/dim. Explicit power-off and an exhausted battery prevent timely alerts; the schedule is recovered at the next boot. Recurring reminders therefore keep the robot on until cancelled. A fired reminder holds automatic power-off for another minute.
- **Expressions and gestures:** six terminal expressions (neutral, happy, sad, angry, curious and sleepy), with block brows and smile/frown shapes driven by emotion messages. Curious glances and small acknowledgement nods on returning to standby after a reply respect the idle-motion setting. No continuous head dancing. A single three-step sequence is updated without sleeps. Listening/speaking cancels autonomous motion; explicit head-position tools pause it. Alerts and sleep keep autonomous gestures still. The optional gesture tool accepts nod/shake/curious only when idle and reports busy instead of claiming success.
- **Personality:** restrained British wit, concise useful replies, no pet behaviour or repetitive catchphrases. Local UI copy follows that voice. Firmware exposes the matching profile through MCP instructions and get_character. Provider support varies: use `PERSONALITY.md` to set the cloud agent role explicitly. A firmware flash cannot guarantee the cloud model's behaviour or change a provider-side voice.

## Wake word

The device name is Phosphor. **This build does not claim to recognise “Phosphor” as a wake word.** It keeps the working stock WakeNet model and tap-to-talk.

The pinned Xiaozhi source has a MultiNet custom-command detector, but that is a different recognition path and needs a suitable English model, asset space, pronunciation handling and noise/false-trigger testing. The stock assets partition is 4 MiB. Merely changing CUSTOM_WAKE_WORD_DISPLAY would rename the reported trigger, not retrain WakeNet. A dedicated Phosphor WakeNet model or a properly evaluated alternative detector is the remaining prerequisite; no paid training service has been ordered.

References: [Espressif wake customization](https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/wake_word_engine/ESP_Wake_Words_Customization.html), [English MultiNet commands](https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/speech_command_recognition/README.html), and the pinned Xiaozhi `main/audio/wake_words/custom_wake_word.cc`.

## One acceptance session

After the existing backup-first flash, inspect boot/home, allow eight idle seconds, create a short reminder, reboot before it expires, then check the alert/snooze/cancel controls. Ask one short question and watch the end-of-reply nod; turn idle motion off to confirm autonomous gestures stop. Use the same voice/volume as your earlier audio comparison. Hardware timing, gesture feel, recognition and cloud compliance cannot be validated by a build alone.
