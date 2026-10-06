/*
*   Reads two MyoWare envelope signals and converts muscle activity into joystick commands
*/

#ifndef MYOWARE_INPUT_H
#define MYOWARE_INPUT_H

#include "CommandProtocol.h"
#include "HubConfig.h"


class MyoWareInput
{
public:
    // Configure GPIO 1 and 2 for the Link Shield envelope outputs
    void begin();

    // Read fresh samples from the Arduino loop - Never write directly to servos
    bool poll(CommandProtocol::DecodedCommand &command, uint32_t now);

    // Forget muscle activity after a stop or receiver change
    void reset();

    // Convert envelope readings with separate activation and release thresholds
    constexpr CommandProtocol::InputState decode(uint16_t sensor1, uint16_t sensor2)
    {
        active1 = sensor1 >= (active1 ? HubConfig::EMG1_RELEASE : HubConfig::EMG1_ACTIVATE);
        active2 = sensor2 >= (active2 ? HubConfig::EMG2_RELEASE : HubConfig::EMG2_ACTIVATE);
        CommandProtocol::InputState input;
        input.x = active1 == active2 ? 0 : active1 ? -1000 : 1000;
        input.y = active1 && active2 ? -1000 : 0;
        return input;
    }

private:
    bool active1 = false;           // Sensor 1 requests left
    bool active2 = false;           // Sensor 2 requests right - Both request forward
    bool sampled = false;           // Allows an immediate first sample
    uint32_t lastSampleTime = 0;    // Start time of the previous sample pair
    uint32_t sequence = 0;          // Increasing command counter, retained across stops
};

#endif
