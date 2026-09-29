#include "ServoController.h"
#include "HubConfig.h"
#include "ServoMotion.h"

bool ServoController::begin()
{
    xServo.setPeriodHertz(50);
    yServo.setPeriodHertz(50);
    xServo.attach(HubConfig::X_SERVO_PIN, HubConfig::SERVO_MIN_US, HubConfig::SERVO_MAX_US);
    yServo.attach(HubConfig::Y_SERVO_PIN, HubConfig::SERVO_MIN_US, HubConfig::SERVO_MAX_US);
    ready = xServo.attached() && yServo.attached();
    center();
    return ready;
}
void ServoController::writeOutputs()
{
    if (xServo.attached()) xServo.write(static_cast<int>(xPosition + 0.5f));
    if (yServo.attached()) yServo.write(static_cast<int>(yPosition + 0.5f));
}
void ServoController::center()
{
    xPosition = HubConfig::X_CENTER;
    yPosition = HubConfig::Y_CENTER;
    lastUpdate = millis();
    writeOutputs();
}
void ServoController::update(CommandProtocol::InputState input, uint32_t now)
{
    if (!ready) return;
    if (!CommandProtocol::validInput(input)) { center(); return; }
    const float dt = (now - lastUpdate) / 1000.0f;
    lastUpdate = now;
    if (HubConfig::INVERT_X) input.x = -input.x;
    if (HubConfig::INVERT_Y) input.y = -input.y;
    const float scale = ServoMotion::speedScale(input.speed);
    const float xTarget = ServoMotion::axisTarget(input.x, HubConfig::X_CENTER,
                                                HubConfig::X_MIN, HubConfig::X_MAX, scale);
    const float yTarget = ServoMotion::axisTarget(input.y, HubConfig::Y_CENTER,
                                                HubConfig::Y_MIN, HubConfig::Y_MAX, scale);
    xPosition = ServoMotion::step(xPosition, xTarget, HubConfig::X_CENTER,
                                 HubConfig::X_MIN, HubConfig::X_MAX, scale, dt);
    yPosition = ServoMotion::step(yPosition, yTarget, HubConfig::Y_CENTER,
                                 HubConfig::Y_MIN, HubConfig::Y_MAX, scale, dt);
    writeOutputs();
}
