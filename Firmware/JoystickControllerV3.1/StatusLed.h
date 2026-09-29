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
        if (mode != receiverMode)
        {
            receiverMode = mode;
            receiverStarted = now;
        }

        if (showingSpeed)
        {
            const uint32_t elapsed = now - flashStarted;
            // Three 200 ms flashes, each followed by 200 ms off
            if (elapsed < 1200) return elapsed % 400 < 200 ? speedColor : Color{0, 0, 0};
            showingSpeed = false;
            receiverStarted = now; // Resume the receiver pattern from its first pulse
        }

        // ESP-NOW stays blue - Other modes repeat their pulse group every two seconds
        if (mode == ReceiverMode::EspNow) return {0, 0, 32};
        const uint32_t phase = (now - receiverStarted) % 2000;
        const unsigned pulses = mode == ReceiverMode::Bluetooth ? 1 :
                                mode == ReceiverMode::I2c ? 2 : 3;
        if (phase >= pulses * 300 || phase % 300 >= 150) return {0, 0, 0};
        if (mode == ReceiverMode::Bluetooth) return {0, 32, 32}; // Cyan single blink
        if (mode == ReceiverMode::I2c) return {32, 0, 32};       // Purple double blink
        return {32, 32, 32};                                   // White triple blink
    }

private:
    ReceiverMode receiverMode = ReceiverMode::EspNow;
    uint32_t receiverStarted = 0;
    uint32_t flashStarted = 0;
    bool showingSpeed = false;
    Color speedColor = {0, 32, 0};
};

#endif
