#ifndef COMMAND_PROTOCOL_H
#define COMMAND_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

// Shared wire format for the hub and remote. Never send native int or float.
namespace CommandProtocol
{
    constexpr uint8_t MARKER = 0xA7;
    constexpr uint8_t VERSION = 1;
    enum class Command : uint8_t { SetSpeedMode = 1, DriveState = 2 };
    enum class SpeedMode : uint8_t { Slow = 0, Medium = 1, Max = 2 };

    // Retains compatibility with the original four-byte speed command.
    struct CommandPacket
    {
        uint8_t marker;
        uint8_t version;
        Command command;
        SpeedMode speedMode;
    };
    static_assert(sizeof(CommandPacket) == 4, "Speed packet must be four bytes");
    constexpr bool validSpeed(SpeedMode mode)
    {
        return mode == SpeedMode::Slow || mode == SpeedMode::Medium ||
               mode == SpeedMode::Max;
    }
    constexpr CommandPacket setSpeedMode(SpeedMode mode)
    {
        return {MARKER, VERSION, Command::SetSpeedMode, mode};
    }
    constexpr bool isValid(const CommandPacket &packet)
    {
        return packet.marker == MARKER && packet.version == VERSION &&
               packet.command == Command::SetSpeedMode && validSpeed(packet.speedMode);
    }

    // DriveState: [marker, version, command, speed, xLE16, yLE16,
    //              bootSessionLE32, sequenceLE32]. Exactly 16 bytes.
    // Axes -1000..1000: +X right, -X left, -Y forward, +Y backward.
    // Every DriveState carries speed, including neutral heartbeats.
    constexpr size_t DRIVE_PACKET_SIZE = 16;
    struct DrivePacket { uint8_t bytes[DRIVE_PACKET_SIZE]; };
    struct InputState
    {
        int16_t x = 0;
        int16_t y = 0;
        SpeedMode speed = SpeedMode::Slow;
    };
    struct DecodedCommand
    {
        Command command = Command::SetSpeedMode;
        InputState input;
        uint32_t session = 0;
        uint32_t sequence = 0;
    };
    constexpr bool validInput(const InputState &state)
    {
        return state.x >= -1000 && state.x <= 1000 &&
               state.y >= -1000 && state.y <= 1000 && validSpeed(state.speed);
    }
    constexpr bool isNeutral(const InputState &state) { return state.x == 0 && state.y == 0; }
    constexpr void write16(uint8_t *p, uint16_t value)
    {
        p[0] = static_cast<uint8_t>(value);
        p[1] = static_cast<uint8_t>(value >> 8);
    }
    constexpr void write32(uint8_t *p, uint32_t value)
    {
        for (unsigned i = 0; i < 4; ++i) p[i] = static_cast<uint8_t>(value >> (8 * i));
    }
    constexpr uint32_t read32(const uint8_t *p)
    {
        return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
               (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
    }
    constexpr int16_t readSigned16(const uint8_t *p)
    {
        const int32_t value = uint16_t(p[0]) | (uint16_t(p[1]) << 8);
        return static_cast<int16_t>(value > 32767 ? value - 65536 : value);
    }
    constexpr DrivePacket driveState(InputState state, uint32_t session, uint32_t sequence)
    {
        DrivePacket packet = {};
        packet.bytes[0] = MARKER;
        packet.bytes[1] = VERSION;
        packet.bytes[2] = static_cast<uint8_t>(Command::DriveState);
        packet.bytes[3] = static_cast<uint8_t>(state.speed);
        write16(packet.bytes + 4, static_cast<uint16_t>(state.x));
        write16(packet.bytes + 6, static_cast<uint16_t>(state.y));
        write32(packet.bytes + 8, session);
        write32(packet.bytes + 12, sequence);
        return packet;
    }
    constexpr bool decode(const uint8_t *data, size_t length, DecodedCommand &out)
    {
        if (data == nullptr || (length != 4 && length != DRIVE_PACKET_SIZE)) return false;
        if (data[0] != MARKER || data[1] != VERSION) return false;
        DecodedCommand next = {};
        next.command = static_cast<Command>(data[2]);
        next.input.speed = static_cast<SpeedMode>(data[3]);
        if (!validSpeed(next.input.speed)) return false;
        if (next.command == Command::SetSpeedMode)
        {
            if (length != 4) return false;
        }
        else if (next.command == Command::DriveState)
        {
            if (length != DRIVE_PACKET_SIZE) return false;
            next.input.x = readSigned16(data + 4);
            next.input.y = readSigned16(data + 6);
            next.session = read32(data + 8);
            next.sequence = read32(data + 12);
            if (!validInput(next.input) || next.session == 0) return false;
        }
        else return false;
        out = next;
        return true;
    }
    constexpr bool newerSequence(uint32_t candidate, uint32_t previous)
    {
        const uint32_t delta = candidate - previous;
        return delta != 0 && delta < 0x80000000UL;
    }
}
#endif
