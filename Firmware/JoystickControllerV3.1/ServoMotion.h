/*
*   Contains the math to convert joystick input to servo angle commands
*/

#ifndef SERVO_MOTION_H
#define SERVO_MOTION_H

#include "CommandProtocol.h"
#include "HubConfig.h"


namespace ServoMotion
{
    // Clamp value v between lo and hi - Ex. clamp(200,0,180) returns 180
    constexpr float clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
    
    // Choose how much travel is allowed in the chair joystick
    constexpr float speedScale(CommandProtocol::SpeedMode mode)
    {
        return mode == CommandProtocol::SpeedMode::Slow ? HubConfig::SLOW_TRAVEL :
               mode == CommandProtocol::SpeedMode::Medium ? HubConfig::MEDIUM_TRAVEL :
               mode == CommandProtocol::SpeedMode::Max ? HubConfig::MAX_TRAVEL : 0.0f;
    }
    
    // Converts one joystick axis into a target angle
    constexpr float axisTarget(int16_t axis, float center, float lo, float hi, float scale)
    {
        // Map axis input (-1000 to 1000) to an angle around center
        // Use the available travel in that direction and the speed-mode scale
        return center + (axis < 0 ? center - lo : hi - center) * (axis / 1000.0f) * scale;
    }

    // Calculate the next commanded angle
    constexpr float step(float current, float target, float center, float lo, float hi, float scale, float dt)
    {
        // Returns neutral position when joystick centered or when changing directions
        if (target == center || (current < center && target > center) || (current > center && target < center)) return center;

        // Limit normal movement per update
        const float maxStep = HubConfig::SERVO_DEGREES_PER_SECOND * clamp(dt, 0.0f, 0.1f);
        float result = current + clamp(target - current, -maxStep, maxStep);
        
        // Enforce the selected speed modes travel limits
        return clamp(result, center - (center - lo) * scale, center + (hi - center) * scale);
    }
}
#endif
