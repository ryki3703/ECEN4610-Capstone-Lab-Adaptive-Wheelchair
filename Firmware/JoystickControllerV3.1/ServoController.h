/*
*   Class to manage the two servos
*/

#ifndef SERVO_CONTROLLER_H
#define SERVO_CONTROLLER_H

#include <ESP32Servo.h>
#include "CommandProtocol.h"


class ServoController
{
public:
    bool begin();       // Initialize both servos
    void center();      // Sends both servos to center
    void update(CommandProtocol::InputState input, uint32_t now);   // Calculate and send next servo command
private:
    // Servo objects
    Servo xServo;
    Servo yServo;

    // Last commanded angles
    float xPosition = 85;
    float yPosition = 85;
    
    uint32_t lastUpdate = 0; // Previous update time
    bool ready = false;      // Did both servos successfully initialize?
    void writeOutputs();     // Helper that rounds stored angles to whole degrees & writes them to servos
};
#endif
