/*
*   Calculates receiver colors and temporary speed flashes without delaying the loop
*/

#ifndef STATUS_LED_H
#define STATUS_LED_H

#include "HubControl.h"

class StatusLed
{
public:
    struct Color
    {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        constexpr bool operator==(Color other) const
        {
            return red == other.red && green == other.green && blue == other.blue;
        }
        constexpr bool operator!=(Color other) const { return !(*this == other); }
    };

    // Start three flashes for the selected speed - A new press restarts the indication
    constexpr void showSpeed(CommandProtocol::SpeedMode speed, uint32_t now)
    {
        using CommandProtocol::SpeedMode;
        speedColor = speed == SpeedMode::Slow ? Color{0, 32, 0} :
                     speed == SpeedMode::Medium ? Color{32, 32, 0} : Color{32, 0, 0};
        flashStarted = now;
        showingSpeed = true;
    }

    // Return the color needed now - The sketch writes it only when it changes
    constexpr Color update(ReceiverMode mode, uint32_t now)
    {
        if (showingSpeed)
        {
            const uint32_t elapsed = now - flashStarted;
            // Three 200 ms flashes, each followed by 200 ms off
            if (elapsed < 1200) return elapsed % 400 < 200 ? speedColor : Color{0, 0, 0};
            showingSpeed = false;
        }

        // Hold the selected receiver color until a speed indication temporarily overrides it
        if (mode == ReceiverMode::Controller) return {32, 0, 32};   // Purple
        if (mode == ReceiverMode::App) return {0, 0, 32}; // Blue
        if (mode == ReceiverMode::I2c) return {32, 12, 0};       // Orange
        if (mode == ReceiverMode::Spi) return {0, 32, 32};      // Cyan
        return {32, 32, 32};                                   // White - ADC
    }

private:
    uint32_t flashStarted = 0;
    bool showingSpeed = false;
    Color speedColor = {0, 32, 0};
};

#endif
