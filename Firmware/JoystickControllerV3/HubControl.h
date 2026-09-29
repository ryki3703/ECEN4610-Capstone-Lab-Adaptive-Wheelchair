#ifndef HUB_CONTROL_H
#define HUB_CONTROL_H

#include "CommandProtocol.h"
#include "HubConfig.h"

// Transport-independent arbitration. Only the Arduino loop may call this class.
enum class ReceiverMode : uint8_t { EspNow, Bluetooth, I2c, Spi };
constexpr ReceiverMode nextReceiverMode(ReceiverMode mode)
{
    return static_cast<ReceiverMode>((static_cast<uint8_t>(mode) + 1) % 4);
}
inline const char *receiverModeName(ReceiverMode mode)
{
    switch (mode)
    {
    case ReceiverMode::EspNow: return "ESP-NOW";
    case ReceiverMode::Bluetooth: return "Bluetooth (not implemented)";
    case ReceiverMode::I2c: return "I2C (not implemented)";
    case ReceiverMode::Spi: return "SPI (not implemented)";
    }
    return "Unknown";
}

class HubControl
{
public:
    constexpr ReceiverMode mode() const { return selectedMode; }
    constexpr bool isArmed() const { return armed; }
    constexpr CommandProtocol::InputState input() const { return state; }
    constexpr void stop()
    {
        state = {};
        armed = false;
        haveDrive = false;
        // Keep sequence history across timeouts; duplicates cannot renew a lease.
    }
    constexpr void selectMode(ReceiverMode mode)
    {
        selectedMode = mode;
        stop();
        haveSession = false;
    }
    constexpr void update(uint32_t now)
    {
        if (haveDrive && now - lastDriveTime >= HubConfig::COMMAND_TIMEOUT_MS) stop();
    }
    constexpr bool accept(ReceiverMode source, const CommandProtocol::DecodedCommand &event,
                          uint32_t receivedAt, uint32_t now)
    {
        using namespace CommandProtocol;
        update(now);
        if (source != selectedMode || now - receivedAt > HubConfig::MAX_QUEUE_AGE_MS ||
            !validInput(event.input)) return false;
        if (event.command == Command::SetSpeedMode)
        {
            state.speed = event.input.speed;
            return true; // Speed alone never arms motion or refreshes the drive lease.
        }
        if (event.command != Command::DriveState || event.session == 0) return false;
        if (!haveSession || event.session != session)
        {
            stop();
            session = event.session;
            haveSession = true;
        }
        else if (!newerSequence(event.sequence, sequence)) return false;
        sequence = event.sequence;
        lastDriveTime = receivedAt;
        haveDrive = true;
        state.speed = event.input.speed;
        if (isNeutral(event.input)) armed = true;
        state.x = armed ? event.input.x : 0;
        state.y = armed ? event.input.y : 0;
        return true;
    }

private:
    ReceiverMode selectedMode = ReceiverMode::EspNow;
    CommandProtocol::InputState state;
    bool armed = false;
    bool haveDrive = false;
    bool haveSession = false;
    uint32_t session = 0;
    uint32_t sequence = 0;
    uint32_t lastDriveTime = 0;
};
#endif
