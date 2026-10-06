#include "../StatusLed.h"

using Color = StatusLed::Color;
constexpr Color off{0, 0, 0};

constexpr bool speedFlashes(CommandProtocol::SpeedMode speed, Color color)
{
    StatusLed led;
    led.showSpeed(speed, 100);
    for (uint32_t elapsed = 0; elapsed < 1200; ++elapsed)
    {
        if (led.update(ReceiverMode::EspNow, 100 + elapsed) !=
            (elapsed % 400 < 200 ? color : off)) return false;
    }
    return led.update(ReceiverMode::EspNow, 1300) == Color{32, 0, 32};
}
static_assert(speedFlashes(CommandProtocol::SpeedMode::Slow, {0, 32, 0}), "Slow flashes");
static_assert(speedFlashes(CommandProtocol::SpeedMode::Medium, {32, 32, 0}), "Medium flashes");
static_assert(speedFlashes(CommandProtocol::SpeedMode::Max, {32, 0, 0}), "Max flashes");

constexpr bool receiverColor(ReceiverMode mode, Color color)
{
    StatusLed led;
    for (uint32_t elapsed = 0; elapsed < 4000; ++elapsed)
    {
        if (led.update(mode, elapsed) != color) return false;
    }
    return true;
}
static_assert(receiverColor(ReceiverMode::EspNow, {32, 0, 32}), "ESP-NOW color");
static_assert(receiverColor(ReceiverMode::Bluetooth, {0, 0, 32}), "Bluetooth color");
static_assert(receiverColor(ReceiverMode::I2c, {32, 12, 0}), "I2C color");
static_assert(receiverColor(ReceiverMode::Spi, {0, 32, 32}), "SPI color");
static_assert(receiverColor(ReceiverMode::Adc, {32, 32, 32}), "ADC color");

constexpr bool overrideAndRollover()
{
    StatusLed led;
    led.showSpeed(CommandProtocol::SpeedMode::Slow, 0xFFFFFF00U);
    if (led.update(ReceiverMode::EspNow, 0xFFFFFF00U) != Color{0, 32, 0}) return false;
    if (led.update(ReceiverMode::I2c, 144) != Color{0, 32, 0}) return false;
    // A fresh press restarts the full indication, even during the previous one.
    led.showSpeed(CommandProtocol::SpeedMode::Max, 200);
    if (led.update(ReceiverMode::I2c, 200) != Color{32, 0, 0}) return false;
    if (led.update(ReceiverMode::I2c, 1399) != off) return false;
    if (led.update(ReceiverMode::I2c, 1400) != Color{32, 12, 0}) return false;
    return led.update(ReceiverMode::Bluetooth, 1500) == Color{0, 0, 32};
}
static_assert(overrideAndRollover(), "Flash override, receiver changes, or clock rollover");
constexpr HubControl startupHub;
static_assert(startupHub.input().speed == CommandProtocol::SpeedMode::Slow, "Power-up must start Slow");
