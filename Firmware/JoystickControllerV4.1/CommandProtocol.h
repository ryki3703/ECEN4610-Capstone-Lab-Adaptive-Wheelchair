/*
*   Defines remote command packets and converts between input values and transmitted bytes
*/

#ifndef COMMAND_PROTOCOL_H
#define COMMAND_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>


// Shared wire format for the hub and remote. Never send native int or float.
namespace CommandProtocol
{
    constexpr uint8_t MARKER = 0xA7; // Identifies packets using this protocol
    constexpr uint8_t VERSION = 1; // Packet format version
    
    // Command types accepted by the hub
    enum class Command : uint8_t { SetSpeedMode = 1, DriveState = 2 };

    // Speed selections sent as one-byte values
    enum class SpeedMode : uint8_t { Slow = 0, Medium = 1, Max = 2 };

    // Retains compatibility with the original four-byte speed command.
    struct CommandPacket
    {
        uint8_t marker;
        uint8_t version;
        Command command;
        SpeedMode speedMode;
    };

    // Check the speed packet size at compile time
    static_assert(sizeof(CommandPacket) == 4, "Speed packet must be four bytes");

    constexpr bool validSpeed(SpeedMode mode)               // Check that the speed is Slow, Medium, or Max
    {
        return mode == SpeedMode::Slow || mode == SpeedMode::Medium || mode == SpeedMode::Max;
    }
    constexpr CommandPacket setSpeedMode(SpeedMode mode)    // Build a speed-only command
    {
        return {MARKER, VERSION, Command::SetSpeedMode, mode};
    }
    constexpr bool isValid(const CommandPacket &packet)     // Check the identifying fields and speed value
    {
        return packet.marker == MARKER && 
               packet.version == VERSION &&
               packet.command == Command::SetSpeedMode && 
               validSpeed(packet.speedMode);
    }

    // DriveState: [marker, version, command, speed, xLE16, yLE16, bootSessionLE32, sequenceLE32]. Exactly 16 bytes.
    // Axes -1000..1000: +X right, -X left, -Y forward, +Y backward.
    // Every DriveState carries speed, including neutral heartbeats.
    constexpr size_t DRIVE_PACKET_SIZE = 16;

    // Fixed byte array sent over the radio
    struct DrivePacket 
    { 
        uint8_t bytes[DRIVE_PACKET_SIZE]; 
    };

    // Joystick axes and speed - Starts neutral at Slow
    struct InputState
    {
        int16_t x = 0;
        int16_t y = 0;
        SpeedMode speed = SpeedMode::Slow;
    };

    // Unpacked command with sender session and packet counter
    struct DecodedCommand
    {
        Command command = Command::SetSpeedMode;
        InputState input;
        uint32_t session = 0;
        uint32_t sequence = 0;
    };

    // Check both axis ranges and the selected speed
    constexpr bool validInput(const InputState &state)
    {
        return state.x >= -1000 && state.x <= 1000 &&
               state.y >= -1000 && state.y <= 1000 && 
               validSpeed(state.speed);
    }

    // True only when both joystick axes are centered
    constexpr bool isNeutral(const InputState &state) 
    { 
        return state.x == 0 && state.y == 0; 
    }

    // Store a 16-bit value with the lowest byte first
    constexpr void write16(uint8_t *p, uint16_t value)
    {
        p[0] = static_cast<uint8_t>(value);
        p[1] = static_cast<uint8_t>(value >> 8);
    }

    // Store a 32-bit value with the lowest byte first
    constexpr void write32(uint8_t *p, uint32_t value)
    {
        for (unsigned i = 0; i < 4; ++i) p[i] = static_cast<uint8_t>(value >> (8 * i));
    }

    // Combine four little-endian bytes into a 32-bit value
    constexpr uint32_t read32(const uint8_t *p)
    {
        return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
    }

    // Read two bytes and restore the signed joystick value
    constexpr int16_t readSigned16(const uint8_t *p)
    {
        const int32_t value = uint16_t(p[0]) | (uint16_t(p[1]) << 8);
        // Values above 32767 represent negative 16-bit numbers
        return static_cast<int16_t>(value > 32767 ? value - 65536 : value);
    }

    // Pack joystick input, session, and sequence into a drive packet
    constexpr DrivePacket driveState(InputState state, uint32_t session, uint32_t sequence)
    {
        // Initialize all packet bytes to zero
        DrivePacket packet = {};
        // Write the identifying fields and speed
        packet.bytes[0] = MARKER;
        packet.bytes[1] = VERSION;
        packet.bytes[2] = static_cast<uint8_t>(Command::DriveState);
        packet.bytes[3] = static_cast<uint8_t>(state.speed);

        // Store X and Y in bytes 4-7 - Negative axes retain their 16-bit representation
        write16(packet.bytes + 4, static_cast<uint16_t>(state.x));
        write16(packet.bytes + 6, static_cast<uint16_t>(state.y));

        // Store session in bytes 8-11 and sequence in bytes 12-15
        write32(packet.bytes + 8, session);
        write32(packet.bytes + 12, sequence);

        return packet;
    }

    // Validate received bytes and unpack the command
    constexpr bool decode(const uint8_t *data, size_t length, DecodedCommand &out)
    {
        // Reject missing data or an unsupported packet length before reading bytes
        if (data == nullptr || (length != 4 && length != DRIVE_PACKET_SIZE)) return false;

        // Reject an unknown protocol marker or version
        if (data[0] != MARKER || data[1] != VERSION) return false;

        // Use temporary storage so rejected packets leave the output unchanged
        DecodedCommand next = {};
        next.command = static_cast<Command>(data[2]);
        next.input.speed = static_cast<SpeedMode>(data[3]);
        if (!validSpeed(next.input.speed)) return false;

        // Speed-only packets must contain exactly four bytes
        if (next.command == Command::SetSpeedMode)
        {
            if (length != 4) return false;
        }
        // Drive packets also contain axes, session, and sequence
        else if (next.command == Command::DriveState)
        {
            if (length != DRIVE_PACKET_SIZE) return false;
            next.input.x = readSigned16(data + 4);
            next.input.y = readSigned16(data + 6);
            next.session = read32(data + 8);
            next.sequence = read32(data + 12);
            // Reject out-of-range input or a zero session identifier
            if (!validInput(next.input) || next.session == 0) return false;
        }
        else return false;

        // Copy the command to the caller only after all checks pass
        out = next;
        return true;
    }
    
    // Check packet order while allowing the unsigned counter to wrap around
    constexpr bool newerSequence(uint32_t candidate, uint32_t previous)
    {
        const uint32_t delta = candidate - previous;
        
        // Reject duplicates and differences at least halfway around the 32-bit range
        return delta != 0 && delta < 0x80000000UL;
    }
}
#endif
