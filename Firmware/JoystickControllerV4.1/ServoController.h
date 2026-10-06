/*
*   Class to manage the two servos
*/

#ifndef SERVO_CONTROLLER_H
#define SERVO_CONTROLLER_H

#include <ESP32Servo.h>
#include "CommandProtocol.h"
#include "HubConfig.h"


class ServoController
{
public:
    bool begin();       // Initialize both servos
    void center();      // Sends both servos to center
    void update(CommandProtocol::InputState input, uint32_t now);   // Calculate and send next servo command
private:
    // Servo objects
    Servo neServo;
    Servo seServo;

    // Last commanded angles
    float nePosition = HubConfig::NE_CENTER;
    float sePosition = HubConfig::SE_CENTER;
    
    uint32_t lastUpdate = 0; // Previous update time
    bool ready = false;      // Did both servos successfully initialize?
    void writeOutputs();     // Helper that rounds stored angles to whole degrees & writes them to servos
};
#endif
