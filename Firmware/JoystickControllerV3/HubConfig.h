#ifndef HUB_CONFIG_H
#define HUB_CONFIG_H

#include <stdint.h>

namespace HubConfig
{
    // ESP32-S3 GPIO numbers, not the old ESP32's D32/D33 assignments.
    constexpr uint8_t X_SERVO_PIN = 4;
    constexpr uint8_t Y_SERVO_PIN = 5;
    constexpr int8_t RECEIVER_MODE_BUTTON_PIN = -1; // Disabled: no pin assigned.
    constexpr uint8_t RX_LED_PIN = 6; // Blue, active HIGH.
    constexpr uint8_t TX_LED_PIN = 7; // Green, active HIGH.
    constexpr uint8_t ESTOP_PIN = 21; // INPUT_PULLUP, active LOW.
    constexpr uint8_t I2C_SDA_PIN = 8;
    constexpr uint8_t I2C_SCL_PIN = 9;
    constexpr uint8_t SPI_CS_PIN = 10;
    constexpr uint8_t SPI_MOSI_PIN = 11;
    constexpr uint8_t SPI_SCK_PIN = 12;
    constexpr uint8_t SPI_MISO_PIN = 13;
    constexpr uint8_t UART_TX_PIN = 17;
    constexpr uint8_t UART_RX_PIN = 18;

    constexpr uint8_t WIFI_CHANNEL = 6; // Must match remote/camera AP.
    constexpr uint8_t REMOTE_MAC[6] = {0x30, 0x30, 0xF9, 0x15, 0x26, 0x54};
    constexpr uint8_t CAMERA_MAC[6] = {0x8C, 0x4F, 0x00, 0xAC, 0x20, 0x00};

    constexpr uint32_t COMMAND_TIMEOUT_MS = 500;
    constexpr uint32_t MAX_QUEUE_AGE_MS = 100;
    constexpr uint32_t BUTTON_DEBOUNCE_MS = 40;
    constexpr uint32_t RX_LED_FLASH_MS = 20;
    constexpr uint32_t TX_LED_FLASH_MS = 20;
    constexpr uint32_t CONTROL_PERIOD_MS = 5;
    constexpr unsigned RX_QUEUE_LENGTH = 12;

    // Retains V2.2 calibration as a starting point; verify the new linkage.
    constexpr float X_CENTER = 85.0f;
    constexpr float Y_CENTER = 85.0f;
    constexpr float X_MIN = 0.0f;
    constexpr float X_MAX = 180.0f;
    constexpr float Y_MIN = 0.0f;
    constexpr float Y_MAX = 180.0f;
    constexpr bool INVERT_X = false;
    constexpr bool INVERT_Y = false;
    constexpr int SERVO_MIN_US = 500;
    constexpr int SERVO_MAX_US = 2500;
    constexpr float SERVO_DEGREES_PER_SECOND = 125.0f;
    constexpr float SLOW_TRAVEL = 0.35f;
    constexpr float MEDIUM_TRAVEL = 0.65f;
    constexpr float MAX_TRAVEL = 1.0f;
    static_assert(X_MIN < X_CENTER && X_CENTER < X_MAX &&
                  Y_MIN < Y_CENTER && Y_CENTER < Y_MAX, "Invalid servo calibration");
    static_assert(X_MIN >= 0 && X_MAX <= 180 && Y_MIN >= 0 && Y_MAX <= 180,
                  "Servo angles must be within 0..180");
    static_assert(SLOW_TRAVEL > 0 && SLOW_TRAVEL <= MEDIUM_TRAVEL &&
                  MEDIUM_TRAVEL <= MAX_TRAVEL && MAX_TRAVEL <= 1,
                  "Speed travel fractions must increase within 0..1");
}
#endif
