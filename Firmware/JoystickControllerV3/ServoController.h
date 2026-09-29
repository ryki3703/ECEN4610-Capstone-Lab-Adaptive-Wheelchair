#ifndef SERVO_CONTROLLER_H
#define SERVO_CONTROLLER_H
#include <ESP32Servo.h>
#include "CommandProtocol.h"

class ServoController
{
public:
    bool begin();
    void center();
    void update(CommandProtocol::InputState input, uint32_t now);
private:
    Servo xServo;
    Servo yServo;
    float xPosition = 85;
    float yPosition = 85;
    uint32_t lastUpdate = 0;
    bool ready = false;
    void writeOutputs();
};
#endif
