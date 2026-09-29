#ifndef HUB_STATUS_PROTOCOL_H
#define HUB_STATUS_PROTOCOL_H

#include "CommandProtocol.h"

// Separate feedback packet: command 3, 16 bytes; existing drive packets unchanged.
namespace HubStatusProtocol
{
    constexpr uint8_t COMMAND = 3;
    constexpr size_t PACKET_SIZE = 16;
    struct Status
    {
        CommandProtocol::SpeedMode speed = CommandProtocol::SpeedMode::Slow;
        uint8_t receiverMode = 0; // 0 ESP-NOW, 1 BLE, 2 I2C, 3 SPI.
        bool armed = false;
        bool ready = false;
        uint32_t session = 0;
        uint32_t sequence = 0;
    };
    struct Packet { uint8_t bytes[PACKET_SIZE]; };
    constexpr Packet encode(const Status &status)
    {
        Packet packet = {};
        packet.bytes[0] = CommandProtocol::MARKER;
        packet.bytes[1] = CommandProtocol::VERSION;
        packet.bytes[2] = COMMAND;
        packet.bytes[3] = static_cast<uint8_t>(status.speed);
        packet.bytes[4] = status.receiverMode;
        packet.bytes[5] = (status.armed ? 1 : 0) | (status.ready ? 2 : 0);
        // Bytes 6..7 are reserved and zero. Session 8..11, sequence 12..15, LE.
        CommandProtocol::write32(packet.bytes + 8, status.session);
        CommandProtocol::write32(packet.bytes + 12, status.sequence);
        return packet;
    }
    constexpr bool decode(const uint8_t *data, size_t size, Status &status)
    {
        if (data == nullptr || size != PACKET_SIZE || data[0] != CommandProtocol::MARKER ||
            data[1] != CommandProtocol::VERSION || data[2] != COMMAND ||
            !CommandProtocol::validSpeed(static_cast<CommandProtocol::SpeedMode>(data[3])) ||
            data[4] > 3 || data[5] > 3 || data[5] == 1 || data[6] != 0 || data[7] != 0)
            return false;
        Status next;
        next.speed = static_cast<CommandProtocol::SpeedMode>(data[3]);
        next.receiverMode = data[4];
        next.armed = (data[5] & 1) != 0;
        next.ready = (data[5] & 2) != 0;
        next.session = CommandProtocol::read32(data + 8);
        next.sequence = CommandProtocol::read32(data + 12);
        if (next.session == 0) return false;
        status = next;
        return true;
    }
}
#endif
