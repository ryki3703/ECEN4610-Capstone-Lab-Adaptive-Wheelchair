/*
*   Stores pin assignments, communication settings, timing, and servo calibration
*/

#ifndef HUB_CONFIG_H
#define HUB_CONFIG_H

#include <stdint.h>


namespace HubConfig
{
    // ESP32-S3 GPIO numbers, not the old ESP32's D32/D33 assignments.
    constexpr uint8_t NE_SERVO_PIN = 4;
    constexpr uint8_t SE_SERVO_PIN = 5;
    constexpr int8_t RECEIVER_MODE_BUTTON_PIN = 14; // INPUT_PULLUP, active LOW
    constexpr uint8_t SPEED_BUTTON_PIN = 15;        // INPUT_PULLUP, active LOW
    constexpr uint8_t RX_LED_PIN = 6;               // Blue, active HIGH
    constexpr uint8_t TX_LED_PIN = 7;               // Green, active HIGH
    constexpr uint8_t ESTOP_PIN = 21;               // INPUT_PULLUP, active LOW
    constexpr uint8_t EMG1_PIN = 1;                 // MyoWare 2.0 EMG Sensor 1 - ADC1_CH0
    constexpr uint8_t EMG2_PIN = 2;                 // MyoWare 2.0 EMG Sensor 2 - ADC1_CH1

    // ESP32-S3 Communications Ports - Reserved for future adapters
    constexpr uint8_t I2C_SDA_PIN = 8;
    constexpr uint8_t I2C_SCL_PIN = 9;
    constexpr uint8_t SPI_CS_PIN = 10;
    constexpr uint8_t SPI_MOSI_PIN = 11;
    constexpr uint8_t SPI_SCK_PIN = 12;
    constexpr uint8_t SPI_MISO_PIN = 13;
    constexpr uint8_t UART_TX_PIN = 17;
    constexpr uint8_t UART_RX_PIN = 18;

    constexpr uint8_t WIFI_CHANNEL = 6; // Must match remote/camera AP.
    constexpr uint8_t REMOTE_MAC[6] = {0x28, 0x84, 0x85, 0xB3, 0xA0, 0x44}; // Remote allowed to send commands and receive status
    constexpr uint8_t CAM_MAC[6] = {0x8C, 0x4F, 0x00, 0xAC, 0x20, 0x01}; // Camera's MAC address for status updates

    constexpr uint32_t COMMAND_TIMEOUT_MS = 500;    // Stop when fresh drive input has been absent this long
    constexpr uint32_t MAX_QUEUE_AGE_MS = 100;      // Reject commands that waited too long in the input queue
    constexpr uint32_t BUTTON_DEBOUNCE_MS = 40;     // Time a button reading must remain unchanged
    constexpr uint32_t RX_LED_FLASH_MS = 20;        // Blue LED flash duration after accepted input
    constexpr uint32_t TX_LED_FLASH_MS = 20;        // Green LED flash duration after an accepted status send
    constexpr uint32_t CONTROL_PERIOD_MS = 5;       // Delay at the end of each control loop
    constexpr unsigned RX_QUEUE_LENGTH = 12;        // Maximum number of queued input events

    // MyoWare envelope thresholds in 12-bit ADC counts - Calibrate for each muscle
    constexpr uint16_t EMG1_ACTIVATE = 1200;
    constexpr uint16_t EMG1_RELEASE = 900;
    constexpr uint16_t EMG2_ACTIVATE = 1200;
    constexpr uint16_t EMG2_RELEASE = 900;
    constexpr uint32_t EMG_SAMPLE_PERIOD_MS = 10;    // Sample both envelope outputs at most every 10 ms

    // Hysteresis requires a lower release threshold and valid ADC counts
    static_assert(EMG1_RELEASE > 0 && EMG1_RELEASE < EMG1_ACTIVATE && EMG1_ACTIVATE <= 4095 &&
                  EMG2_RELEASE > 0 && EMG2_RELEASE < EMG2_ACTIVATE && EMG2_ACTIVATE <= 4095,
                  "Invalid MyoWare thresholds");
                  
    static_assert(EMG_SAMPLE_PERIOD_MS > 0 && EMG_SAMPLE_PERIOD_MS < COMMAND_TIMEOUT_MS,
                  "MyoWare sampling must be faster than the command timeout");

    // Verify neutral and travel calibration for the NE and SE linkages.
    // Neutral servo angles in degrees - Reposition the horns mechanically for the new mount
    constexpr float NE_CENTER = 85.0f;
    constexpr float SE_CENTER = 85.0f;

    // Minimum and maximum angles for each servo
    constexpr float NE_MIN = 0.0f;
    constexpr float NE_MAX = 180.0f;
    constexpr float SE_MIN = 0.0f;
    constexpr float SE_MAX = 180.0f;

    // Reverse a joystick axis before mixing the NE and SE servo movement
    constexpr bool INVERT_X = false;
    constexpr bool INVERT_Y = false;

    // Pulse widths used to map angle commands to the servo signal
    constexpr int SERVO_MIN_US = 500;
    constexpr int SERVO_MAX_US = 2500;

    // Normal rate of change for commanded angles
    constexpr float SERVO_DEGREES_PER_SECOND = 125.0f; 

    // Allowed fraction of travel from center for each speed mode
    constexpr float SLOW_TRAVEL = 0.35f;
    constexpr float MEDIUM_TRAVEL = 0.65f;
    constexpr float MAX_TRAVEL = 1.0f;

    // Check that each neutral angle lies inside its travel limits
    static_assert(NE_MIN < NE_CENTER && NE_CENTER < NE_MAX &&
                  SE_MIN < SE_CENTER && SE_CENTER < SE_MAX, 
                  "Invalid servo calibration");

    // Check that configured angles stay within 0-180 degrees
    static_assert(NE_MIN >= 0 && NE_MAX <= 180 && 
                  SE_MIN >= 0 && SE_MAX <= 180,
                  "Servo angles must be within 0..180");
                  
    // Check that travel fractions increase and do not exceed full travel
    static_assert(SLOW_TRAVEL > 0 && 
                  SLOW_TRAVEL <= MEDIUM_TRAVEL &&
                                 MEDIUM_TRAVEL <= MAX_TRAVEL && 
                                                  MAX_TRAVEL <= 1,
                  "Speed travel fractions must increase within 0..1");
}
#endif
