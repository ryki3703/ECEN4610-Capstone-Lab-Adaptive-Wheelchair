#include "../MyoWareInput.h"
#include "../HubControl.h"
#include "../HubStatusProtocol.h"


constexpr bool muscleMappingTests()
{
    MyoWareInput input;
    auto state = input.decode(0, 0);
    if (!CommandProtocol::isNeutral(state)) return false;
    state = input.decode(HubConfig::EMG1_ACTIVATE, 0);
    if (state.x != -1000 || state.y != 0) return false;
    state = input.decode(HubConfig::EMG1_RELEASE, 0);
    if (state.x != -1000) return false;
    state = input.decode(HubConfig::EMG1_RELEASE - 1, 0);
    if (!CommandProtocol::isNeutral(state)) return false;
    state = input.decode(HubConfig::EMG1_ACTIVATE - 1, 0);
    if (!CommandProtocol::isNeutral(state)) return false;
    state = input.decode(0, HubConfig::EMG2_ACTIVATE);
    if (state.x != 1000 || state.y != 0) return false;
    state = input.decode(0, HubConfig::EMG2_RELEASE);
    if (state.x != 1000) return false;
    state = input.decode(0, HubConfig::EMG2_RELEASE - 1);
    if (!CommandProtocol::isNeutral(state)) return false;
    state = input.decode(4095, 4095);
    if (state.x != 0 || state.y != -1000) return false;
    return CommandProtocol::isNeutral(input.decode(0, 0));
}
static_assert(muscleMappingTests(), "MyoWare direction mapping or hysteresis failed");

constexpr bool adcArmingTests()
{
    HubControl hub;
    MyoWareInput input;
    hub.selectMode(ReceiverMode::Adc);
    CommandProtocol::DecodedCommand command = {CommandProtocol::Command::DriveState, input.decode(4095, 4095), 1, 1};
    hub.accept(ReceiverMode::Adc, command, 0, 0);
    if (hub.isArmed() || !CommandProtocol::isNeutral(hub.input())) return false;
    command.input = input.decode(0, 0);
    ++command.sequence;
    hub.accept(ReceiverMode::Adc, command, 10, 10);
    if (!hub.isArmed()) return false;
    command.input = input.decode(4095, 4095);
    ++command.sequence;
    hub.accept(ReceiverMode::Adc, command, 20, 20);
    if (hub.input().x != 0 || hub.input().y != -1000) return false;
    hub.stop();
    ++command.sequence;
    hub.accept(ReceiverMode::Adc, command, 30, 30);
    if (hub.isArmed()) return false;
    command.input = input.decode(0, 0);
    ++command.sequence;
    hub.accept(ReceiverMode::Adc, command, 40, 40);
    hub.update(40 + HubConfig::COMMAND_TIMEOUT_MS);
    return !hub.isArmed() && CommandProtocol::isNeutral(hub.input());
}
static_assert(adcArmingTests(), "ADC neutral arming, stop recovery or timeout failed");

constexpr bool adcStatusTests()
{
    HubStatusProtocol::Status status = {CommandProtocol::SpeedMode::Slow, 4, false, true, 1, 1};
    auto packet = HubStatusProtocol::encode(status);
    HubStatusProtocol::Status decoded;
    if (!HubStatusProtocol::decode(packet.bytes, sizeof(packet.bytes), decoded) || decoded.receiverMode != 4) return false;
    packet.bytes[4] = 5;
    return !HubStatusProtocol::decode(packet.bytes, sizeof(packet.bytes), decoded);
}
static_assert(adcStatusTests(), "ADC status value validation failed");
