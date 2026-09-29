# JoystickControllerV3 — ESP32-S3 input hub

This sketch replaces the V2.2 one-byte direction receiver with a modular hub.
ESP-NOW remote input is implemented. Bluetooth LE, I2C and SPI have selectable
receiver slots and a shared input interface; their actual transports are future work.
Only the selected source can affect the servos. Unsupported modes hold neutral.

## Board and wiring

Arduino board: **ESP32S3 Dev Module**. For an S3-N16R8 with quad flash and octal
PSRAM, choose **16 MB flash**, **OPI PSRAM**, and the **16M Flash (3MB APP/9.9MB
FATFS)** partition scheme. USB CDC On Boot can be enabled for the board's native
USB connector. If using the UART bridge connector instead, select the corresponding
serial interface. Confirm your particular board's memory type/USB connector.

Library: **ESP32Servo**. Verified toolchain versions are recorded below.

| Connection | Default GPIO | Notes |
|---|---:|---|
| X servo signal | 4 | Left/right |
| Y servo signal | 5 | Forward/backward |
| E-Stop | 21 | INPUT_PULLUP, active LOW; switch to GND |
| RX blue LED | 6 | External LED with series resistor, active HIGH |
| TX green LED | 7 | Flashes when a status send is accepted by ESP-NOW; active HIGH |
| I2C SDA / SCL | 8 / 9 | Reserved for future adapter |
| SPI CS / MOSI / SCK / MISO | 10 / 11 / 12 / 13 | Reserved for future adapter |
| UART TX / RX | 17 / 18 | Reserved for future adapter |

All assignments are in `HubConfig.h`. These are GPIO numbers, not header positions.
Servo power should come from a supply suited to the servos, with a common signal
ground. GPIOs carry servo signals, not servo power. The old ESP32 GPIO32/33 mapping
is not carried onto this S3 board. GPIO35–37 are unavailable on relevant octal-memory
modules; GPIO19/20 are left available for native USB.

Espressif board reference:
https://documentation.espressif.com/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.0.html

## Pairing and first run

1. Upload the hub and open Serial Monitor at 115200. Record **Hub station MAC**.
2. Put that address in the matching remote's `RemoteConfig::JOYSTICK_CONTROLLER_MAC`.
3. Start the remote and copy its printed **Remote MAC** into `HubConfig::REMOTE_MAC`.
4. Rebuild/upload both sketches. The zero MAC placeholders disable remote control.
5. Both radios and the camera AP use **channel 6**. Keep those configurations aligned.
6. Release the remote joystick to neutral. The hub then prints **Input ready** and
   accepts movement. Deflect the remote joystick to command proportional motion.

The supplied remote update is required for movement: the previous version sent
only speed packets. Both folders contain identical `CommandProtocol.h` files.
There is no automatic discovery, application acknowledgement, or pairing UI yet.
An allowed MAC is a source filter, not cryptographic authentication; ESP-NOW traffic
continues to use unencrypted peers, matching the existing project.

## Receiver modes and future adapters

The receiver-mode button is disabled (`RECEIVER_MODE_BUTTON_PIN = -1`) because no
pin is assigned in this wiring. ESP-NOW remains the startup source. If a separate
unused pin is assigned later, each debounced button press cycles **ESP-NOW → Bluetooth → I2C → SPI → ESP-NOW**.
Startup selects ESP-NOW; mode selection is not persisted. A mode change immediately
commands center, discards queued remote messages, and requires fresh neutral input.
Bluetooth/I2C/SPI are deliberately inactive until implemented.

New adapters should copy validated input into a bounded queue in their callback,
then call `hub.accept(source, decodedCommand, receivedAt, now)` from `loop()`.
Use `CommandProtocol::InputState` (-1000..1000 axes plus speed), a nonzero session
identifier per source restart, and a monotonically increasing sequence. The hub
handles source selection, timeouts and neutral gating for every adapter. Callback
code must never write directly to servos. The future S3 Bluetooth app transport
should use BLE, not Bluetooth Classic SPP.
Espressif Bluetooth reference:
https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/ble/overview.html

## Speed and servo behavior

The remote joystick button selects **Slow → Medium → Max → Slow**. Speed starts
at Slow and is included in every drive packet, so a missed speed change is corrected
by the next current-state packet. Speed means a cap on each axis's calibrated
joystick deflection, not a measured wheelchair velocity:

| Mode | Available travel from center |
|---|---:|
| Slow | 35% |
| Medium | 65% |
| Max | 100% |

Edit these fractions and the mechanical calibration in `HubConfig.h`.
The V2.2 centers (85°), bounds (0°..180°), pulse range (500..2500 µs), and ramp rate
(125°/second) are retained as starting settings. Positive X means right; negative Y
means forward. `INVERT_X` / `INVERT_Y` handle linkage polarity. The remote also has
ADC deadband/axis inversion settings. Calibrate on the bench before attaching the
linkage; the retained full-range values have not been verified on the new hardware.

Analog input sets a proportional target; it no longer increments displacement
indefinitely as V2.2's digital direction command did. Travel toward a target ramps.
An axis returning to neutral commands center immediately; there is no five-STOP
delay. Direction reversal first commands center. Reducing the speed mode immediately
constrains existing commanded deflection to the lower travel cap.

## Protocol and loss handling

The four-byte SetSpeedMode packet remains accepted: `A7 01 01 <speed>`.
It changes speed only; it never arms or refreshes the movement timeout.

DriveState is exactly 16 bytes, explicitly serialized little-endian:

| Byte offset | Field |
|---:|---|
| 0 | Marker `0xA7` |
| 1 | Version `1` |
| 2 | Command `2` (DriveState) |
| 3 | Speed: 0 Slow, 1 Medium, 2 Max |
| 4–5 | Signed X, -1000..1000 |
| 6–7 | Signed Y, -1000..1000 |
| 8–11 | Nonzero remote boot-session ID |
| 12–15 | Unsigned sequence counter |

The remote sends a current snapshot every 50 ms, including neutral. Length, marker,
version, command, ranges and source MAC are checked before admission. Duplicate and
out-of-order sequences within a session are rejected without renewing the lease.
Frames that spend more than 100 ms in the local queue are discarded.

No fresh drive input for 500 ms commands center and disarms. Boot, source changes,
sender-session changes and queue overflow also require neutral before movement.
A held joystick cannot resume movement just because communication returns. Servo
updates run every approximately 5 ms; the timeout is checked on that loop.
These checks do not authenticate packets or establish their over-air age, and the
session field is not a replay-resistant handshake. Receiver restart also resets
sequence history. No physical servo-position feedback is available.

The remote input task runs independently of camera connection/frame reads, so its
20 Hz drive stream does not depend on camera UI loop progress. ESP-NOW send success
is not an application-level acknowledgement. CameraProtocol.h is unchanged.

ESP-NOW callback/channel reference:
https://docs.espressif.com/projects/esp-idf/en/release-v5.0/esp32s3/api-reference/network/esp_now.html

## Files

- `JoystickControllerV3.ino`: setup, mode changes and loop coordination.
- `HubConfig.h`: pins, MAC, channel, timing, servo calibration and speed limits.
- `CommandProtocol.h`: shared wire format, serialization and validation.
- `HubControl.h`: source arbitration, sequence checks, timeout and neutral gating.
- `EspNowInput.h/.cpp`: source filtering and bounded callback-to-loop queue.
- `ServoController.h/.cpp`, `ServoMotion.h`: outputs, target calculation and ramping.
- `ModeButton.h`: independent mode-button debounce.

## Verification

Build target: Arduino-ESP32 3.3.2, ESP32Servo 3.2.1, ESP32S3 Dev Module with
16 MB flash and OPI PSRAM. Matching remote target: XIAO_ESP32S3.
Compiler-evaluated tests in `tests/hub_tests.cpp` cover wire format,
invalid packets, axis ranges, source switching, timeouts, neutral
rearming, session changes, sequence/timer rollover, speed caps and servo targets.
The remote's `tests/mapping_tests.cpp` checks ADC endpoints, deadband and inversion.
Remote libraries used: Adafruit GFX 1.12.6, Adafruit ILI9341 1.6.3, TJpg_Decoder 1.1.0.

Hardware checks still needed: verify axes and center/travel calibration, each speed
cap, button cycling, held-input startup, release-to-center, remote power loss and
recovery, hub restart, and movement while starting/stopping the camera stream.
No firmware upload or physical servo test was performed as part of this change.

## Remote home-screen feedback

The hub now sends its current software speed mode, selected input source, armed
state and hardware-ready state to the configured remote every 200 ms. Set
`HubConfig::REMOTE_MAC` to the remote's station MAC and upload both updated sketches.
Status transmission is best-effort and does not block servo updates; it is not
per-command acknowledgement or physical speed/servo-position feedback.

`HubStatusProtocol.h` is shared with the remote. Its 16-byte status frame contains
marker `0xA7`, version `1`, command `3`, speed, receiver mode, flags (bit 0 armed,
bit 1 ready), two reserved zero bytes, a nonzero hub boot-session ID (LE32), and
a sequence (LE32). Existing drive/camera formats stay unchanged. The remote
rejects invalid or wrong-source frames and duplicate/out-of-order sequences
within a session. After one second without new status it shows no chair feedback.

## E-stop and peripheral pin assignments

GPIO21 is sampled active LOW with INPUT_PULLUP. A LOW commands servo center,
disarms input, and discards queued commands. While held, movement is blocked.
Release discards pending input again and requires a fresh neutral drive packet
before movement can resume. Status reports not ready while E-stop is held.
This is a polled software stop commanding neutral, not a motor-power disconnect.
Verify stopping and neutral rearming on the bench before use.

RX blue flashes for accepted input. TX green flashes when a status transmission
is queued successfully; it does not confirm delivery. The old GPIO15 armed LED
is removed. I2C, SPI and UART assignments are constants in HubConfig.h; these
unimplemented adapters do not initialize buses yet. Serial remains the existing
debug console; UART_TX_PIN/UART_RX_PIN describe the future peripheral UART.

Pin-alignment changes have not been uploaded or hardware-tested.