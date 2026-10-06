# JoystickControllerV4 - ESP32-S3 input hub

Receives joystick commands over ESP-NOW and drives two servos that move the chair joystick. 
Sends status back to the remote. 
ESP-NOW and ADC (two MyoWare 2.0 sensors) are implemented input sources; Bluetooth, I2C, and SPI are future work.


## Setup
- Board: **ESP32S3 Dev Module**, S3-N16R8 with **16 MB flash**, **OPI PSRAM**, and **16M Flash (3MB APP/9.9MB FATFS)** partition scheme. Match these settings to the actual board. Enable USB CDC On Boot when using native USB for Serial output.
- Library: **ESP32Servo**. Previously recorded build versions: Arduino-ESP32 **3.3.2**, ESP32Servo **3.2.1**.
- Serial Monitor: **115200 baud**.

1. Upload the hub and copy its printed **Hub station MAC** into the remote's `RemoteConfig::JOYSTICK_CONTROLLER_MAC`.
2. Verify `HubConfig::REMOTE_MAC` matches the remote's station MAC. It already contains an address; replace it if using a different remote.
3. Keep the hub, remote, and camera AP on **channel 6**, then rebuild/upload any changed sketches. The remote must send the DriveState format defined in `CommandProtocol.h`.
4. Release the remote joystick to neutral. **Input ready** means movement is armed.


## Wiring
GPIO assignments are in `HubConfig.h`; these are GPIO numbers, not header positions.

| Connection | GPIO | Notes |
|---|---:|---|
| NE / SE servo signals | 4 / 5 | Northeast / southeast joystick linkages |
| E-stop | 21 | INPUT_PULLUP; active LOW, switch to GND |
| Receiver-mode button | 14 | Normally open button to GND; INPUT_PULLUP |
| Speed button | 15 | Normally open button to GND; INPUT_PULLUP |
| RX blue LED | 6 | Active HIGH; flashes for accepted input |
| TX green LED | 7 | Active HIGH; flashes when a status send is queued |
| MyoWare sensor 1 / 2 envelope | 1 / 2 | Link Shield ENV outputs; use 3.3 V sensor power and common GND |
| I2C SDA / SCL | 8 / 9 | Reserved |
| SPI CS / MOSI / SCK / MISO | 10 / 11 / 12 / 13 | Reserved |
| UART TX / RX | 17 / 18 | Reserved |

Use series resistors for external LEDs. Power servos from a suitable supply with a common signal ground; GPIOs provide signals only. Reserved bus pins are not initialized by this sketch.

The receiver-mode button uses **GPIO14**, with a normally open button to GND. Each press cycles **ESP-NOW → Bluetooth → I2C → SPI → ADC → ESP-NOW**. Bluetooth, I2C, and SPI currently provide mode/button/LED testing only; they have no input adapter and hold neutral.

The speed button cycles **Slow → Medium → Max → Slow** in **I2C, SPI and ADC modes**, with 40 ms debounce and one change per press. ESP-NOW and Bluetooth use the speed supplied by their input packets. Local modes ignore packet speed selections; drive packets must still contain a valid speed value. The local selection survives stops/timeouts and resets to Slow when selecting a receiver mode. E-stop blocks speed changes. Local speed/LED testing works even if hardware initialization fails; movement remains blocked. Changing speed never arms movement or refreshes the drive timeout.

For a button/LED test, power up and watch three green flashes followed by steady purple. Press GPIO14's button to see blue (Bluetooth), orange (I2C), cyan (SPI), white (ADC), then purple (ESP-NOW). In I2C or SPI, press GPIO15's button three times to see yellow, red, then green triple flashes. Wait for each indication to finish to see the receiver color return. No connected remote or bus device is needed. Bluetooth, I2C and SPI do not generate movement commands; ADC does when the sensors are connected and calibrated.


## Movement and stopping

The onboard RGB LED shows the receiver mode: **purple = ESP-NOW**, **blue = Bluetooth**, **orange = I2C**, **cyan = SPI**, and **white = ADC**. Receiver colors stay steady and show the selected mode, not connection or readiness.

Power-up starts at **Slow** and displays **three green flashes**. Every controller speed-button press temporarily replaces the receiver color with three flashes: **green = Slow**, **yellow = Medium**, **red = Max** (200 ms on/off). A new press restarts the flashes. In ESP-NOW/Bluetooth, or when a speed change is blocked, the flashes report the current selection without changing it. Afterward the receiver color returns. Remote speed commands can change speed after startup. LED timing does not add delays to the control loop.

The LED uses the board's `RGB_BUILTIN` mapping (GPIO48 for the current ESP32S3 Dev Module target). Boards with a different onboard RGB pin need a matching board definition.
- Speed modes limit travel from center: **Slow 35%**, **Medium 65%**, **Max 100%**. These are joystick deflection limits, not measured chair speeds.
- Current calibration: **85 degrees** neutral on both servos, **0-180 degrees** travel, **500-2500 microseconds** servo pulses, and **125 degrees/second** normal command ramp. Adjust calibration and `INVERT_X` / `INVERT_Y` in `HubConfig.h` for the linkage.
- For the new mount, keep both commanded neutrals at 85 degrees and reposition the NE horn approximately 90 degrees clockwise and the SE horn approximately 90 degrees counterclockwise from their old orientations. Establish the commanded neutral with linkages disconnected, then power off before refitting the horns. Fine-tune `NE_CENTER` / `SE_CENTER` for the spline positions and verify linkage travel before reconnecting chair propulsion.
- Joystick axes are mixed into NE = X - Y and SE = -X - Y, where positive servo movement is clockwise and negative joystick Y is forward. Right rotates NE clockwise and SE counterclockwise; forward rotates both clockwise. Diagonal commands are normalized together. Both servos use the same symmetric travel, limited by the smallest available distance from either neutral to its limits, then scaled by speed mode. This gives equal angular changes for cardinal directions; the straight-linkage geometry still needs bench calibration.
- Neutral input commands center immediately. Direction reversals command center first; lower speed modes immediately constrain existing deflection. Physical servo movement still takes time.
- No fresh drive input for **500 ms** stops and disarms the hub. Startup, receiver/session changes, queue overflow, and E-stop recovery require fresh neutral input before movement resumes.
- E-stop commands center, disarms, and clears queued input while held and on release. It is a **polled software stop**, not a motor-power disconnect.
- Each loop ends with a **5 ms delay**; processing adds to the total loop time.


## MyoWare ADC input

Select ADC using the receiver-mode button (four presses from ESP-NOW). Sensor 1 alone commands left, sensor 2 alone commands right, both command forward, and neither commands neutral. Commands use full selected-speed travel; muscle amplitude does not proportionally control speed. There is no reverse command. The physical speed button selects Slow, Medium, or Max.

`MyoWareInput.h/.cpp` samples both envelope outputs every 10 ms from the main loop. Each channel activates at 1200 ADC counts and releases below 900 counts, using 12-bit readings (0-4095). These are starting values, not measured muscle calibration. Adjust `EMG1_ACTIVATE`, `EMG1_RELEASE`, `EMG2_ACTIVATE`, and `EMG2_RELEASE` in `HubConfig.h` after measuring relaxed and contracted levels. The gap prevents chatter near a threshold. Both sensors must fall below their release thresholds after startup, a mode change, or E-stop before movement arms. ADC commands pass through the existing hub arbitration, timeout, speed limits, and servo handling. ADC works even if radio initialization fails; status feedback still requires the radio.

Use the Link Shield's envelope output, not raw EMG, for this mapping. Power each sensor at 3.3 V and share signal ground with the hub; never connect a 5 V envelope output directly to an ESP32-S3 GPIO. The 11 dB ADC setting measures approximately up to 3.1 V on ESP32-S3, so upper readings can saturate. Software cannot detect a disconnected sensor reliably: a floating ADC input can look like muscle activity. Verify all four input combinations and E-stop/rearming with chair propulsion disabled before operating the linkage.

Reference: [SparkFun MyoWare guide](https://learn.sparkfun.com/tutorials/getting-started-with-the-myoware-20-muscle-sensor-ecosystem/all) and [Espressif ADC API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html).

ADC has receiver-mode value **4** in hub status packets. Update the remote's status decoder and display to recognize this value; an older decoder limited to 0-3 rejects ADC status packets. The packet layout is unchanged.


## Communication
The hub checks the sender MAC, packet format, axis ranges, session, and sequence. Duplicate/older packets within a session and commands queued longer than **100 ms** are rejected. The queue holds **12 events**. ESP-NOW peers are unencrypted; the MAC filter is not authentication.

`CommandProtocol.h` defines 4-byte speed-only commands and 16-byte drive commands. Drive packets carry axes (-1000 to 1000), speed, session, and sequence. Speed-only commands never arm movement or refresh its timeout. Packet layouts and byte conversion are documented in the protocol headers; hub and remote must use compatible formats.

Status is attempted every **200 ms** and reports speed, receiver mode, armed/ready flags, session, and sequence. It reports not ready during E-stop. A successful send or green LED flash does not confirm delivery or physical servo position.


## Code map
| File | Purpose |
|---|---|
| `JoystickControllerV4.ino` | Startup and main loop coordination |
| `HubConfig.h` | Pins, communication settings, timing, and calibration |
| `HubControl.h` | Input selection, packet order, timeout, and neutral rearming |
| `CommandProtocol.h` | Command packet encoding and validation |
| `HubStatusProtocol.h` | Status packet encoding and validation |
| `EspNowInput.h/.cpp` | ESP-NOW setup, input queue, and status sending |
| `MyoWareInput.h/.cpp` | ADC sampling, muscle thresholds, and direction mapping |
| `ServoController.h/.cpp` | Servo initialization and output commands |
| `ServoMotion.h` | Target angles, travel limits, and movement steps |
| `ModeButton.h` | Shared debounce class for receiver and speed buttons |
| `StatusLed.h` | Receiver indication and temporary speed flash timing |

Future input adapters should queue commands and call `hub.accept()` from the main loop. Callbacks must not write directly to servos.


## Verification
`tests/hub_tests.cpp` contains compile-time checks for packet validation, neutral rearming, timeouts, session/sequence handling, rollover, servo calculations, and local/remote speed selection. `tests/status_led_tests.cpp` checks steady receiver colors, all three speed flashes, interruption/restart, clock rollover, and the Slow startup default. `tests/myoware_tests.cpp` checks all four muscle combinations, threshold hysteresis, ADC arming/stop/timeout behavior, and ADC status decoding. `tests/linkage_tests.cpp` checks NE/SE direction mixing, diagonal normalization, and equal angular travel. All four compile-time suites and the V4 ESP32-S3 build passed. These do not verify physical hardware or the complete E-stop path.

Before operating the linkage, verify center/travel calibration, axis direction, speed limits, E-stop/rearming, communication-loss recovery, and LED colors on the bench. The speed-button and LED changes were compiled for the target above and passed the compile-time tests; they have not been uploaded or hardware-tested.
