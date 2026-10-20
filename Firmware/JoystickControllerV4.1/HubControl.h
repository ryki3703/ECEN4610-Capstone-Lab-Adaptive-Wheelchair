/*
*   Checks input source, packet order, and timeouts before allowing joystick movement
*/

#ifndef HUB_CONTROL_H
#define HUB_CONTROL_H

#include "CommandProtocol.h"
#include "HubConfig.h"


// Transport-independent arbitration. Only the Arduino loop may call this class.
enum class ReceiverMode : uint8_t { Controller, App, I2c, Spi, Adc };

// Cycle through receiver modes and wrap back to ESP-NOW
constexpr ReceiverMode nextReceiverMode(ReceiverMode mode)
{
    return static_cast<ReceiverMode>((static_cast<uint8_t>(mode) + 1) % 5);
}

// Return a readable receiver name for Serial output
inline const char *receiverModeName(ReceiverMode mode)
{
    switch (mode)
    {
    case ReceiverMode::Controller: return "Controller";
    case ReceiverMode::App: return "App";
    case ReceiverMode::I2c: return "I2C (not implemented)";
    case ReceiverMode::Spi: return "SPI (not implemented)";
    case ReceiverMode::Adc: return "ADC (MyoWare)";
    }
    return "Unknown";
}

class HubControl
{
public:
    // Return the selected input source
    constexpr ReceiverMode mode() const { return selectedMode; }

    // Has neutral input enabled movement?
    constexpr bool isArmed() const { return armed; }

    // Return the current accepted joystick state
    constexpr CommandProtocol::InputState input() const { return state; }

    // I2C, SPI and ADC use the physical speed button instead of packet speed
    constexpr bool usesLocalSpeed() const
    {
        return selectedMode == ReceiverMode::I2c || selectedMode == ReceiverMode::Spi ||
               selectedMode == ReceiverMode::Adc;
    }

    // Cycle local speed without arming movement or refreshing the drive timeout
    constexpr void cycleLocalSpeed()
    {
        if (!usesLocalSpeed()) return;
        using CommandProtocol::SpeedMode;
        localSpeed = localSpeed == SpeedMode::Slow ? SpeedMode::Medium :
                     localSpeed == SpeedMode::Medium ? SpeedMode::Max : SpeedMode::Slow;
        state.speed = localSpeed;
    }

    // Reset axes and require neutral again - Keep the local button selection
    constexpr void stop()
    {
        state = {};
        if (usesLocalSpeed()) state.speed = localSpeed;
        armed = false;
        haveDrive = false;
        // Keep sequence history across timeouts; duplicates cannot renew a lease.
    }

    // Change input source and clear the previous session tracking
    constexpr void selectMode(ReceiverMode mode)
    {
        selectedMode = mode;
        localSpeed = CommandProtocol::SpeedMode::Slow; // Start each receiver selection at Slow
        stop();
        haveSession = false;
    }

    // Stop if the latest accepted drive input has timed out
    constexpr void update(uint32_t now)
    {
        if (haveDrive && now - lastDriveTime >= HubConfig::COMMAND_TIMEOUT_MS) stop();
    }

    // Check a decoded command before updating the joystick state
    constexpr bool accept(ReceiverMode source, const CommandProtocol::DecodedCommand &event, uint32_t receivedAt, uint32_t now)
    {
        using namespace CommandProtocol;

        // Apply any existing timeout before considering this command
        update(now);

        // Reject the wrong source, old queued input, or invalid axis/speed values
        if (source != selectedMode || now - receivedAt > HubConfig::MAX_QUEUE_AGE_MS 
                                   || !validInput(event.input)) return false;

        // A speed-only command changes speed without enabling movement
        if (event.command == Command::SetSpeedMode)
        {
            if (usesLocalSpeed()) return false; // Local modes take speed only from the button
            state.speed = event.input.speed;
            return true; // Speed alone never arms motion or refreshes the drive lease.
        }

        // Remaining commands must be drive packets with a nonzero session
        if (event.command != Command::DriveState || event.session == 0) return false;

        // A new sender session stops movement until neutral is received
        if (!haveSession || event.session != session)
        {
            stop();
            session = event.session;
            haveSession = true;
        }
        // Within the same session, reject duplicate or older packets
        else if (!newerSequence(event.sequence, sequence)) return false;

        // Remember packet order and use arrival time for the movement timeout
        sequence = event.sequence;
        lastDriveTime = receivedAt;
        haveDrive = true;
        state.speed = usesLocalSpeed() ? localSpeed : event.input.speed;
        
        // Only neutral input can arm movement after a stop
        if (isNeutral(event.input)) armed = true;

        // Hold both axes at neutral until armed
        state.x = armed ? event.input.x : 0;
        state.y = armed ? event.input.y : 0;
        return true;
    }

private:
    CommandProtocol::SpeedMode localSpeed = CommandProtocol::SpeedMode::Slow;
    ReceiverMode selectedMode = ReceiverMode::Controller;   // Startup input source
    CommandProtocol::InputState state;                  // Accepted axes and speed used by the servo controller
    bool armed = false;                                 // Movement enabled after neutral input
    bool haveDrive = false;                             // A drive packet is being tracked for timeout
    bool haveSession = false;                           // A sender session has been recorded
    uint32_t session = 0;                               // Current sender session identifier
    uint32_t sequence = 0;                              // Most recently accepted drive sequence
    uint32_t lastDriveTime = 0;                         // Arrival time of the latest accepted drive packet
};

#endif
