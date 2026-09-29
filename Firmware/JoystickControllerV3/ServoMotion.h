#ifndef SERVO_MOTION_H
#define SERVO_MOTION_H

#include "CommandProtocol.h"
#include "HubConfig.h"

namespace ServoMotion
{
    constexpr float clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
    constexpr float speedScale(CommandProtocol::SpeedMode mode)
    {
        return mode == CommandProtocol::SpeedMode::Slow ? HubConfig::SLOW_TRAVEL :
               mode == CommandProtocol::SpeedMode::Medium ? HubConfig::MEDIUM_TRAVEL :
               mode == CommandProtocol::SpeedMode::Max ? HubConfig::MAX_TRAVEL : 0.0f;
    }
    constexpr float axisTarget(int16_t axis, float center, float lo, float hi, float scale)
    {
        return center + (axis < 0 ? center - lo : hi - center) * (axis / 1000.0f) * scale;
    }
    constexpr float step(float current, float target, float center, float lo, float hi,
                         float scale, float dt)
    {
        // Neutral and direction reversals command center without a release delay.
        if (target == center || (current < center && target > center) ||
            (current > center && target < center)) return center;
        const float maxStep = HubConfig::SERVO_DEGREES_PER_SECOND * clamp(dt, 0.0f, 0.1f);
        float result = current + clamp(target - current, -maxStep, maxStep);
        // A lower speed mode must constrain an already-deflected joystick too.
        return clamp(result, center - (center - lo) * scale, center + (hi - center) * scale);
    }
}
#endif
