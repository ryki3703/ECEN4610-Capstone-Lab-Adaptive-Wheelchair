/*
*   Initializes the servos, tracks command angles, and updates commands based on remote input
*/

#include "ServoController.h"
#include "HubConfig.h"
#include "ServoMotion.h"

bool ServoController::begin()           // Initialize Servos
{
    // Set control signals to 50Hz
    xServo.setPeriodHertz(50);
    yServo.setPeriodHertz(50);

    // Connect servo objects to GPIO pins
    // Pulse range defines how angle commands map to control signal
    xServo.attach(HubConfig::X_SERVO_PIN, HubConfig::SERVO_MIN_US, HubConfig::SERVO_MAX_US);
    yServo.attach(HubConfig::Y_SERVO_PIN, HubConfig::SERVO_MIN_US, HubConfig::SERVO_MAX_US);

    ready = xServo.attached() && yServo.attached(); // True when both servos initialize successfully
    center();                                       // Send servos to center on startup
    return ready;                                   // Return the initialization result to caller
}

void ServoController::writeOutputs()    // Rounds stored angles to whole degrees & writes them to servos
{
    if (xServo.attached()) xServo.write(static_cast<int>(xPosition + 0.5f));
    if (yServo.attached()) yServo.write(static_cast<int>(yPosition + 0.5f));
}

void ServoController::center()          // Send Servos to neutral position
{
    xPosition = HubConfig::X_CENTER;
    yPosition = HubConfig::Y_CENTER;
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
    
    // Calculate target angles
    const float scale = ServoMotion::speedScale(input.speed);   // Converts speed mode into travel fraction
    const float xTarget = ServoMotion::axisTarget(input.x, HubConfig::X_CENTER, HubConfig::X_MIN, HubConfig::X_MAX, scale);
    const float yTarget = ServoMotion::axisTarget(input.y, HubConfig::Y_CENTER, HubConfig::Y_MIN, HubConfig::Y_MAX, scale);

    // Calculate the next angle commands
    xPosition = ServoMotion::step(xPosition, xTarget, HubConfig::X_CENTER, HubConfig::X_MIN, HubConfig::X_MAX, scale, dt);
    yPosition = ServoMotion::step(yPosition, yTarget, HubConfig::Y_CENTER, HubConfig::Y_MIN, HubConfig::Y_MAX, scale, dt);
    
    // Send commands
    writeOutputs();
}
