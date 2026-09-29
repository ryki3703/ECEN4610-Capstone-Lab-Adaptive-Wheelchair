#include "../CommandProtocol.h"
#include "../HubControl.h"
#include "../ServoMotion.h"


using namespace CommandProtocol;
constexpr DecodedCommand drive(int16_t x, int16_t y, uint32_t seq,
                               SpeedMode speed = SpeedMode::Slow, uint32_t session = 1)
{
    return {Command::DriveState, {x, y, speed}, session, seq};
}
constexpr bool wireTests()
{
    auto packet = driveState({-1000, 1000, SpeedMode::Max}, 0x12345678, 0xFEDCBA98);
    DecodedCommand decoded = {};
    if (packet.bytes[4] != 0x18 || packet.bytes[5] != 0xFC ||
        packet.bytes[6] != 0xE8 || packet.bytes[7] != 3 ||
        packet.bytes[8] != 0x78 || packet.bytes[15] != 0xFE) return false;
    if (!decode(packet.bytes, 16, decoded) || decoded.input.x != -1000 ||
        decoded.input.y != 1000 || decoded.input.speed != SpeedMode::Max ||
        decoded.session != 0x12345678 || decoded.sequence != 0xFEDCBA98) return false;
    for (size_t n = 0; n < 20; ++n)
        if (n != 16 && decode(packet.bytes, n, decoded)) return false;
    auto bad = packet;
    bad.bytes[0] = 0; if (decode(bad.bytes, 16, decoded)) return false;
    bad = packet; bad.bytes[1] = 2; if (decode(bad.bytes, 16, decoded)) return false;
    bad = packet; bad.bytes[2] = 0; if (decode(bad.bytes, 16, decoded)) return false;
    bad = packet; bad.bytes[3] = 3; if (decode(bad.bytes, 16, decoded)) return false;
    bad = driveState({1001, 0, SpeedMode::Slow}, 1, 1);
    if (decode(bad.bytes, 16, decoded)) return false;
    bad = driveState({0, -1001, SpeedMode::Slow}, 1, 1);
    if (decode(bad.bytes, 16, decoded)) return false;
    bad = driveState({0, 0, SpeedMode::Slow}, 0, 1);
    if (decode(bad.bytes, 16, decoded)) return false;
    uint8_t speed[] = {MARKER, VERSION, 1, 1};
    if (!decode(speed, 4, decoded) || decoded.input.speed != SpeedMode::Medium) return false;
    speed[3] = 255;
    return !decode(speed, 4, decoded) && !decode(nullptr, 4, decoded);
}
static_assert(wireTests(), "Packet format/validation failed");
static_assert(newerSequence(0, 0xFFFFFFFF) && !newerSequence(1, 1) &&
              !newerSequence(1, 2) && !newerSequence(0x80000000, 0), "Sequence ordering failed");

constexpr bool startupAndReleaseTests()
{
    HubControl hub;
    if (hub.isArmed() || !isNeutral(hub.input())) return false;
    hub.accept(ReceiverMode::EspNow, drive(1000, 0, 1), 0, 0);
    if (hub.isArmed() || !isNeutral(hub.input())) return false;
    hub.accept(ReceiverMode::EspNow, drive(0, 0, 2), 10, 10);
    if (!hub.isArmed()) return false;
    hub.accept(ReceiverMode::EspNow, drive(1000, -500, 3, SpeedMode::Medium), 20, 20);
    if (hub.input().x != 1000 || hub.input().y != -500 || hub.input().speed != SpeedMode::Medium)
        return false;
    hub.accept(ReceiverMode::EspNow, drive(0, 0, 4), 30, 30);
    return isNeutral(hub.input());
}
static_assert(startupAndReleaseTests(), "Neutral startup and release failed");

constexpr bool timeoutTests()
{
    HubControl hub;
    hub.accept(ReceiverMode::EspNow, drive(0, 0, 1), 0, 0);
    hub.accept(ReceiverMode::EspNow, drive(1000, 0, 2), 10, 10);
    if (hub.accept(ReceiverMode::EspNow, drive(0, 0, 2), 100, 100)) return false;
    if (hub.accept(ReceiverMode::EspNow, drive(0, 0, 1), 200, 200)) return false;
    DecodedCommand speed = {Command::SetSpeedMode, {0, 0, SpeedMode::Max}, 0, 0};
    hub.accept(ReceiverMode::EspNow, speed, 400, 400);
    hub.update(509); if (!hub.isArmed()) return false;
    hub.update(510); if (hub.isArmed() || !isNeutral(hub.input())) return false;
    hub.accept(ReceiverMode::EspNow, drive(1000, 0, 3), 520, 520);
    if (hub.isArmed() || !isNeutral(hub.input())) return false;
    hub.accept(ReceiverMode::EspNow, drive(0, 0, 4), 530, 530);
    hub.accept(ReceiverMode::EspNow, drive(-1000, 0, 5), 540, 540);
    return hub.isArmed() && hub.input().x == -1000;
}
static_assert(timeoutTests(), "Timeout, duplicate rejection or speed-only lease failed");

constexpr bool modeAndSessionTests()
{
    HubControl hub;
    hub.accept(ReceiverMode::EspNow, drive(0, 0, 1), 0, 0);
    hub.accept(ReceiverMode::EspNow, drive(1000, 0, 2), 10, 10);
    hub.selectMode(ReceiverMode::Bluetooth);
    if (hub.isArmed() || !isNeutral(hub.input())) return false;
    if (hub.accept(ReceiverMode::EspNow, drive(1000, 0, 3), 20, 20)) return false;
    hub.selectMode(ReceiverMode::EspNow);
    hub.accept(ReceiverMode::EspNow, drive(1000, 0, 4), 30, 30);
    if (hub.isArmed()) return false;
    hub.accept(ReceiverMode::EspNow, drive(0, 0, 5), 40, 40);
    hub.accept(ReceiverMode::EspNow, drive(1000, 0, 1, SpeedMode::Slow, 2), 50, 50);
    if (hub.isArmed() || !isNeutral(hub.input())) return false;
    hub.accept(ReceiverMode::EspNow, drive(0, 0, 2, SpeedMode::Slow, 2), 60, 60);
    hub.stop(); // Queue overflow path.
    hub.accept(ReceiverMode::EspNow, drive(1000, 0, 3, SpeedMode::Slow, 2), 70, 70);
    if (hub.isArmed()) return false;
    if (hub.accept(ReceiverMode::EspNow, drive(0, 0, 4, SpeedMode::Slow, 2), 70, 171)) return false;
    return !hub.isArmed() && nextReceiverMode(ReceiverMode::Spi) == ReceiverMode::EspNow;
}
static_assert(modeAndSessionTests(), "Source/session changes or stale queue handling failed");

constexpr bool rolloverTests()
{
    HubControl hub;
    hub.accept(ReceiverMode::EspNow, drive(0, 0, 0xFFFFFFFF), 0xFFFFFFF0, 0xFFFFFFF0);
    hub.accept(ReceiverMode::EspNow, drive(1000, 0, 0), 10, 10);
    if (hub.input().x != 1000) return false;
    hub.update(510);
    return !hub.isArmed() && isNeutral(hub.input());
}
static_assert(rolloverTests(), "Clock or sequence rollover failed");

constexpr bool localSpeedTests(ReceiverMode mode)
{
    HubControl hub;
    hub.selectMode(mode);
    hub.cycleLocalSpeed();
    if (hub.input().speed != SpeedMode::Medium || hub.isArmed()) return false;
    // Initial session and later packets must retain the button selection.
    if (!hub.accept(mode, drive(0, 0, 1, SpeedMode::Max), 0, 0)) return false;
    if (!hub.accept(mode, drive(500, 0, 2, SpeedMode::Slow), 10, 10)) return false;
    if (hub.input().speed != SpeedMode::Medium || hub.input().x != 500) return false;
    DecodedCommand speed = {Command::SetSpeedMode, {0, 0, SpeedMode::Max}, 0, 0};
    if (hub.accept(mode, speed, 20, 20)) return false;
    hub.cycleLocalSpeed();
    if (hub.input().speed != SpeedMode::Max) return false;
    hub.cycleLocalSpeed();
    if (hub.input().speed != SpeedMode::Slow) return false;
    hub.cycleLocalSpeed();
    // Button presses must not extend the drive timeout or rearm after a stop.
    hub.update(510);
    if (hub.isArmed() || !isNeutral(hub.input()) || hub.input().speed != SpeedMode::Medium) return false;
    hub.cycleLocalSpeed();
    hub.accept(mode, drive(500, 0, 3), 520, 520);
    if (hub.isArmed() || !isNeutral(hub.input())) return false;
    hub.accept(mode, drive(0, 0, 4), 530, 530);
    if (!hub.isArmed() || hub.input().speed != SpeedMode::Max) return false;
    hub.stop();
    if (hub.isArmed() || hub.input().speed != SpeedMode::Max) return false;
    hub.selectMode(mode);
    return !hub.isArmed() && hub.input().speed == SpeedMode::Slow;
}
static_assert(localSpeedTests(ReceiverMode::I2c), "I2C local speed handling failed");
static_assert(localSpeedTests(ReceiverMode::Spi), "SPI local speed handling failed");

constexpr bool remoteSpeedTests(ReceiverMode mode)
{
    HubControl hub;
    hub.selectMode(mode);
    hub.cycleLocalSpeed();
    if (hub.input().speed != SpeedMode::Slow) return false;
    hub.accept(mode, drive(0, 0, 1, SpeedMode::Max), 0, 0);
    hub.cycleLocalSpeed();
    if (hub.input().speed != SpeedMode::Max) return false;
    DecodedCommand speed = {Command::SetSpeedMode, {0, 0, SpeedMode::Medium}, 0, 0};
    return hub.accept(mode, speed, 10, 10) && hub.input().speed == SpeedMode::Medium;
}
static_assert(remoteSpeedTests(ReceiverMode::EspNow), "ESP-NOW speed ownership failed");
static_assert(remoteSpeedTests(ReceiverMode::Bluetooth), "Bluetooth speed ownership failed");

constexpr bool buttonOnlyModeTests()
{
    HubControl hub;
    for (unsigned i = 0; i < 4; ++i)
    {
        hub.selectMode(nextReceiverMode(hub.mode()));
        if (hub.input().speed != SpeedMode::Slow) return false;
        hub.cycleLocalSpeed();
        hub.update(1000);
        if (hub.isArmed() || !isNeutral(hub.input())) return false;
        // Failed hardware initialization calls stop every loop but preserves local selection.
        hub.stop();
        if (hub.input().speed != (hub.usesLocalSpeed() ? SpeedMode::Medium : SpeedMode::Slow)) return false;
        if (hub.mode() != ReceiverMode::EspNow &&
            hub.accept(ReceiverMode::EspNow, drive(1000, 0, i + 1), 1000, 1000)) return false;
        if (hub.isArmed() || !isNeutral(hub.input())) return false;
    }
    return hub.mode() == ReceiverMode::EspNow;
}
static_assert(buttonOnlyModeTests(), "Button-only modes must stay neutral and cycle correctly");

constexpr bool near(float a, float b) { return a - b < 0.001f && b - a < 0.001f; }
static_assert(near(ServoMotion::axisTarget(1000, 85, 0, 180, 0.35f), 118.25f), "Slow travel");
static_assert(near(ServoMotion::axisTarget(-1000, 85, 0, 180, 0.65f), 29.75f), "Medium travel");
static_assert(ServoMotion::axisTarget(1000, 85, 0, 180, 1) == 180, "Max travel");
static_assert(ServoMotion::step(120, 85, 85, 0, 180, 1, 0.005f) == 85, "Immediate neutral");
static_assert(ServoMotion::step(120, 0, 85, 0, 180, 1, 0.005f) == 85, "Reversal centers");
static_assert(near(ServoMotion::step(180, 118.25f, 85, 0, 180, 0.35f, 0.005f), 118.25f),
              "Speed reduction must clamp existing deflection");
static_assert(ServoMotion::step(85, 180, 85, 0, 180, 1, 1) == 97.5f, "Time step clamp");

