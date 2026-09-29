/* ESP32-S3 N16R8 joystick hub. See README.md for wiring and setup. */
#include "HubConfig.h"
#include "HubControl.h"
#include "ServoController.h"
#include "EspNowInput.h"
#include "ModeButton.h"

HubControl hub;
ServoController servos;
EspNowInput remote;
ModeButton modeButton;
bool hardwareReady = false;
bool rxLedOn = false;
bool txLedOn = false;
bool estopWasActive = false;
uint32_t lastTxFlash = 0;
uint32_t lastRxFlash = 0;
bool lastArmed = false;
CommandProtocol::SpeedMode lastSpeed = CommandProtocol::SpeedMode::Slow;

void setup()
{
    Serial.begin(115200);
    pinMode(HubConfig::RX_LED_PIN, OUTPUT);
    pinMode(HubConfig::TX_LED_PIN, OUTPUT);
    pinMode(HubConfig::ESTOP_PIN, INPUT_PULLUP);
    digitalWrite(HubConfig::RX_LED_PIN, LOW);
    digitalWrite(HubConfig::TX_LED_PIN, LOW);
    modeButton.begin();
    const bool servoReady = servos.begin(); // Command neutral before network startup.
    delay(500);
    Serial.println("Joystick Controller V3 - ESP32-S3 hub");
    const bool radioReady = remote.begin();
    hardwareReady = servoReady && radioReady;
    if (!servoReady) Serial.println("Servo attachment failed; movement disabled");
    if (!radioReady) Serial.println("ESP-NOW initialization failed; movement disabled");
    Serial.println("Receiver: ESP-NOW. Release joystick to neutral before moving.");
}

void loop()
{
    uint32_t now = millis();
    const bool estopActive = digitalRead(HubConfig::ESTOP_PIN) == LOW;
    // Clear queued input while held and on release; require fresh neutral to rearm.
    if (estopActive || estopWasActive)
    {
        hub.stop();
        servos.center();
        remote.discardPending();
    }
    estopWasActive = estopActive;
    if (modeButton.pressed(now))
    {
        hub.selectMode(nextReceiverMode(hub.mode()));
        servos.center();
        remote.discardPending();
        Serial.print("Receiver: ");
        Serial.println(receiverModeName(hub.mode()));
    }

    if (remote.takeOverflow())
    {
        hub.stop();
        servos.center();
        remote.discardPending();
        Serial.println("Input queue overflow; return joystick to neutral");
    }

    InputEvent event;
    for (unsigned i = 0; i < HubConfig::RX_QUEUE_LENGTH && remote.poll(event); ++i)
    {
        if (hardwareReady && !estopActive && hub.accept(ReceiverMode::EspNow, event.command,
                                         event.receivedAt, millis()))
        {
            // Preserve a neutral event even if a newer drive is already queued.
            if (CommandProtocol::isNeutral(hub.input())) servos.center();
            digitalWrite(HubConfig::RX_LED_PIN, HIGH);
            rxLedOn = true;
            lastRxFlash = millis();
        }
    }
    // Future BLE, I2C and SPI adapters submit decoded events through hub.accept()
    // here, with their ReceiverMode, arrival time, session and sequence.
    // They must not write directly to servos or call hub.accept() from callbacks.
    now = millis();
    hub.update(now);
    const bool stopBeforeOutput = digitalRead(HubConfig::ESTOP_PIN) == LOW;
    if (!hardwareReady || estopActive || stopBeforeOutput)
    {
        hub.stop();
        servos.center();
    }
    estopWasActive = estopActive || stopBeforeOutput;
    servos.update(hub.input(), now);
    if (remote.publishStatus(hub.input().speed, static_cast<uint8_t>(hub.mode()),
                             hub.isArmed(), hardwareReady && !estopWasActive))
    {
        digitalWrite(HubConfig::TX_LED_PIN, HIGH);
        txLedOn = true;
        lastTxFlash = millis();
    }
    if (hub.isArmed() != lastArmed)
    {
        lastArmed = hub.isArmed();
        Serial.println(lastArmed ? "Input ready" : "Input stopped; return to neutral");
    }
    if (hub.input().speed != lastSpeed)
    {
        lastSpeed = hub.input().speed;
        Serial.print("Speed: ");
        Serial.println(lastSpeed == CommandProtocol::SpeedMode::Slow ? "SLOW" :
                       lastSpeed == CommandProtocol::SpeedMode::Medium ? "MEDIUM" : "MAX");
    }
    now = millis();
    if (txLedOn && now - lastTxFlash >= HubConfig::TX_LED_FLASH_MS)
    {
        digitalWrite(HubConfig::TX_LED_PIN, LOW);
        txLedOn = false;
    }
    if (rxLedOn && now - lastRxFlash >= HubConfig::RX_LED_FLASH_MS)
    {
        digitalWrite(HubConfig::RX_LED_PIN, LOW);
        rxLedOn = false;
    }
    delay(HubConfig::CONTROL_PERIOD_MS);
}
