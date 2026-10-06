/*
*   Initializes the servos, tracks command angles, and updates commands based on remote input
*/

#include "ServoController.h"
#include "HubConfig.h"
#include "ServoMotion.h"

bool ServoController::begin()           // Initialize Servos
{
    // Set control signals to 50Hz
    neServo.setPeriodHertz(50);
    seServo.setPeriodHertz(50);

    // Connect servo objects to GPIO pins
    // Pulse range defines how angle commands map to control signal
    neServo.attach(HubConfig::NE_SERVO_PIN, HubConfig::SERVO_MIN_US, HubConfig::SERVO_MAX_US);
    seServo.attach(HubConfig::SE_SERVO_PIN, HubConfig::SERVO_MIN_US, HubConfig::SERVO_MAX_US);

    ready = neServo.attached() && seServo.attached(); // True when both servos initialize successfully
    center();                                       // Send servos to center on startup
    return ready;                                   // Return the initialization result to caller
}

void ServoController::writeOutputs()    // Rounds stored angles to whole degrees & writes them to servos
{
    if (neServo.attached()) neServo.write(static_cast<int>(nePosition + 0.5f));
    if (seServo.attached()) seServo.write(static_cast<int>(sePosition + 0.5f));
}

void ServoController::center()          // Send Servos to neutral position
{
    nePosition = HubConfig::NE_CENTER;
    sePosition = HubConfig::SE_CENTER;
    lastUpdate = millis();
    writeOutputs();
}

// Main Servo controller function
void ServoController::update(CommandProtocol::InputState input, uint32_t now)
{
    // Skip update if servos not successfully initialized
    if (!ready) return;

    // Checks both axes are within range and speed mode is valid - End the update if not
    if (!CommandProtocol::validInput(input)) { center(); return; }

    // Calculate elapsed time
    const float dt = (now - lastUpdate) / 1000.0f;
    lastUpdate = now;

    // Apply direction inversions
    if (HubConfig::INVERT_X) input.x = -input.x;
    if (HubConfig::INVERT_Y) input.y = -input.y;
    
    // Mix joystick axes into clockwise-positive NE and SE movement
    const auto linkage = ServoMotion::mix(input.x, input.y);
    const float travel = ServoMotion::linkageTravel();

    // Calculate target angles
    const float scale = ServoMotion::speedScale(input.speed);   // Converts speed mode into travel fraction
    const float neTarget = ServoMotion::axisTarget(linkage.ne, HubConfig::NE_CENTER, HubConfig::NE_CENTER - travel, HubConfig::NE_CENTER + travel, scale);
    const float seTarget = ServoMotion::axisTarget(linkage.se, HubConfig::SE_CENTER, HubConfig::SE_CENTER - travel, HubConfig::SE_CENTER + travel, scale);

    // Calculate the next angle commands
    nePosition = ServoMotion::step(nePosition, neTarget, HubConfig::NE_CENTER, HubConfig::NE_CENTER - travel, HubConfig::NE_CENTER + travel, scale, dt);
    sePosition = ServoMotion::step(sePosition, seTarget, HubConfig::SE_CENTER, HubConfig::SE_CENTER - travel, HubConfig::SE_CENTER + travel, scale, dt);
    
    // Send commands
    writeOutputs();
}
