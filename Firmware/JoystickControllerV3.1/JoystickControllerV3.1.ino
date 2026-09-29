/*
*   Initializes the joystick hub and coordinates remote input, stopping, servo commands, and status
*/

/* ESP32-S3 N16R8 joystick hub. See README.md for wiring and setup. */
#include "HubConfig.h"
#include "HubControl.h"
#include "ServoController.h"
#include "EspNowInput.h"
#include "ModeButton.h"
#include "StatusLed.h"


// Objects for input checks, servo outputs, radio communication, and the mode button
HubControl hub;
ServoController servos;
EspNowInput remote;
ModeButton modeButton;
ModeButton speedButton(HubConfig::SPEED_BUTTON_PIN); // Separate debounce state for local speed
StatusLed statusLed;
StatusLed::Color lastLedColor = {0, 0, 0};

// Send RGB data only when the displayed color changes
void updateStatusLed(uint32_t now)
{
    const auto color = statusLed.update(hub.mode(), now);
    if (color == lastLedColor) return;
    rgbLedWrite(RGB_BUILTIN, color.red, color.green, color.blue);
    lastLedColor = color;
}

bool hardwareReady = false; // True when servo attachment and radio initialization succeed
bool rxLedOn = false; // Tracks the blue receive LED flash
bool txLedOn = false; // Tracks the green transmit LED flash
bool estopWasActive = false; // Remembers a stop so queued input is also cleared on release

// Start times for the LED flashes
uint32_t lastTxFlash = 0;
uint32_t lastRxFlash = 0;

bool lastArmed = false; // Previous armed state - Print only when it changes

// Previous speed mode used to detect changes for Serial output
CommandProtocol::SpeedMode lastSpeed = CommandProtocol::SpeedMode::Slow;


// Initialize pins, servos, and radio once at startup
void setup()
{
    // Start the debug console and configure LEDs and the active-low stop input
    Serial.begin(115200);
    rgbLedWrite(RGB_BUILTIN, 0, 0, 0); // Addressable onboard LED starts off
    pinMode(HubConfig::RX_LED_PIN, OUTPUT);
    pinMode(HubConfig::TX_LED_PIN, OUTPUT);
    pinMode(HubConfig::ESTOP_PIN, INPUT_PULLUP);
    digitalWrite(HubConfig::RX_LED_PIN, LOW);
    digitalWrite(HubConfig::TX_LED_PIN, LOW);
    modeButton.begin();
    speedButton.begin();

    const bool servoReady = servos.begin(); // Command neutral before network startup.
    delay(500);

    Serial.println("Joystick Controller V3 - ESP32-S3 hub");
    const bool radioReady = remote.begin();
    //const bool radioReady = true;         // Uncomment to bypass remote's wifi setup - used for testing at one point

    // Allow input only when both initialization steps succeed
    hardwareReady = servoReady && radioReady;
    if (!servoReady) Serial.println("Servo attachment failed; movement disabled");
    if (!radioReady) Serial.println("ESP-NOW initialization failed; movement disabled");
    Serial.println("Receiver: ESP-NOW. Release joystick to neutral before moving.");

    // HubControl starts at Slow on every power-up - Show three green flashes
    statusLed.showSpeed(CommandProtocol::SpeedMode::Slow, millis());
    updateStatusLed(millis());
}

// Process input and update outputs repeatedly
void loop()
{
    uint32_t now = millis();        // Keep live time
    // A LOW stop input requests neutral and blocks movement
    const bool estopActive = digitalRead(HubConfig::ESTOP_PIN) == LOW;

    // Clear queued input while held and on release; require fresh neutral to rearm.
    if (estopActive || estopWasActive)
    {
        hub.stop();
        servos.center();
        remote.discardPending();
    }

    estopWasActive = estopActive;

    // If enabled, a mode-button press changes source and clears pending input
    if (modeButton.pressed(now))
    {
        hub.selectMode(nextReceiverMode(hub.mode()));
        servos.center();
        remote.discardPending();
        Serial.print("Receiver: ");
        Serial.println(receiverModeName(hub.mode()));
    }

    // Always sample the button so presses in remote modes are consumed
    // Allow local speed/LED testing without initialized hardware - E-stop still blocks changes
    const bool speedPressed = speedButton.pressed(now);
    if (speedPressed && !estopActive) hub.cycleLocalSpeed();

    // Stop if the receive queue filled and a command could not be stored
    if (remote.takeOverflow())
    {
        hub.stop();
        servos.center();
        remote.discardPending();
        Serial.println("Input queue overflow; return joystick to neutral");
    }

    // Process at most one queue capacity per loop so other work can continue
    InputEvent event;
    for (unsigned i = 0; i < HubConfig::RX_QUEUE_LENGTH && remote.poll(event); ++i)
    {
        // Accept commands only when initialized, not stopped, and approved by the hub
        if (hardwareReady && !estopActive && hub.accept(ReceiverMode::EspNow, event.command, event.receivedAt, millis()))
        {
            Serial.print("Accepted input - X: ");
            Serial.print(hub.input().x);

            Serial.print(" Y: ");
            Serial.print(hub.input().y);

            Serial.print(" Speed: ");

            switch (hub.input().speed)
            {
                case CommandProtocol::SpeedMode::Slow:
                    Serial.println("SLOW");
                    break;

                case CommandProtocol::SpeedMode::Medium:
                    Serial.println("MEDIUM");
                    break;

                case CommandProtocol::SpeedMode::Max:
                    Serial.println("MAX");
                    break;
            }
            
            // Preserve a neutral event even if a newer drive is already queued.
            if (CommandProtocol::isNeutral(hub.input())) servos.center();
            // Flash blue for an accepted input command
            digitalWrite(HubConfig::RX_LED_PIN, HIGH);
            rxLedOn = true;
            lastRxFlash = millis();
        }
    }

    // Future BLE, I2C and SPI adapters submit decoded events through hub.accept() here, with their ReceiverMode, arrival time, session and sequence.
    // They must not write directly to servos or call hub.accept() from callbacks.
    now = millis();     // update current time
    // Check whether fresh drive input has timed out
    hub.update(now);
    // Read the stop input again immediately before updating servo commands
    const bool stopBeforeOutput = digitalRead(HubConfig::ESTOP_PIN) == LOW;
    
    // Force neutral if initialization failed or either stop check was active
    if (!hardwareReady || estopActive || stopBeforeOutput)
    {
        hub.stop();
        servos.center();
    }
    estopWasActive = estopActive || stopBeforeOutput;
    // Calculate and send servo angles from the accepted input state
    servos.update(hub.input(), now);
    // Publish hub feedback when due - Flash green if the send is accepted locally
    if (remote.publishStatus(hub.input().speed, static_cast<uint8_t>(hub.mode()), hub.isArmed(), hardwareReady && !estopWasActive))
    {
        digitalWrite(HubConfig::TX_LED_PIN, HIGH);
        txLedOn = true;
        lastTxFlash = millis();
    }
    // Print armed state changes without repeating the message every loop
    if (hub.isArmed() != lastArmed)
    {
        lastArmed = hub.isArmed();
        Serial.println(lastArmed ? "Input ready" : "Input stopped; return to neutral");
    }
    // Print the new speed only when it changes
    if (hub.input().speed != lastSpeed)
    {
        lastSpeed = hub.input().speed;
        Serial.print("Speed: ");
        Serial.println(lastSpeed == CommandProtocol::SpeedMode::Slow ? "SLOW" :
                       lastSpeed == CommandProtocol::SpeedMode::Medium ? "MEDIUM" : "MAX");
    }
    now = millis();
    // Every button press shows the actual speed after input and stop checks
    // Remote modes only display speed; their packets still control the selection
    if (speedPressed) statusLed.showSpeed(hub.input().speed, now);
    updateStatusLed(now);
    // Turn off the transmit LED after its flash interval
    if (txLedOn && now - lastTxFlash >= HubConfig::TX_LED_FLASH_MS)
    {
        digitalWrite(HubConfig::TX_LED_PIN, LOW);
        txLedOn = false;
    }
    // Turn off the receive LED after its flash interval
    if (rxLedOn && now - lastRxFlash >= HubConfig::RX_LED_FLASH_MS)
    {
        digitalWrite(HubConfig::RX_LED_PIN, LOW);
        rxLedOn = false;
    }
    delay(HubConfig::CONTROL_PERIOD_MS); // Pause briefly before the next control update
}
