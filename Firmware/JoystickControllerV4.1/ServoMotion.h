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
    
    struct LinkageInput
    {
        int16_t ne; // Positive requests clockwise rotation
        int16_t se; // Positive requests clockwise rotation
    };

    // Mix joystick directions for the NE and SE linkages - Negative Y means forward
    constexpr LinkageInput mix(int16_t x, int16_t y)
    {
        const int32_t ne = int32_t(x) - y;
        const int32_t se = -int32_t(x) - y;
        const int32_t neMagnitude = ne < 0 ? -ne : ne;
        const int32_t seMagnitude = se < 0 ? -se : se;
        const int32_t largest = neMagnitude > seMagnitude ? neMagnitude : seMagnitude;
        // Scale both together so diagonal input preserves their relative movement
        const int32_t divisor = largest > 1000 ? largest : 1000;
        return {static_cast<int16_t>(ne * 1000 / divisor), static_cast<int16_t>(se * 1000 / divisor)};
    }

    // Use equal travel in both directions so paired servo rotations have the same magnitude
    constexpr float linkageTravel()
    {
        const float neTravel = clamp(HubConfig::NE_CENTER - HubConfig::NE_MIN, 0.0f, HubConfig::NE_MAX - HubConfig::NE_CENTER);
        const float seTravel = clamp(HubConfig::SE_CENTER - HubConfig::SE_MIN, 0.0f, HubConfig::SE_MAX - HubConfig::SE_CENTER);
        return neTravel < seTravel ? neTravel : seTravel;
    }

    // Converts one linkage input into a target angle
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
