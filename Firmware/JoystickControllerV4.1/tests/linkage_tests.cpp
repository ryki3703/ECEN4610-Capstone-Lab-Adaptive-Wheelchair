#include "../ServoMotion.h"


constexpr bool directionTests()
{
    auto input = ServoMotion::mix(0, 0);
    if (input.ne != 0 || input.se != 0) return false;
    input = ServoMotion::mix(1000, 0);
    if (input.ne != 1000 || input.se != -1000) return false;
    input = ServoMotion::mix(-1000, 0);
    if (input.ne != -1000 || input.se != 1000) return false;
    input = ServoMotion::mix(0, -1000);
    if (input.ne != 1000 || input.se != 1000) return false;
    input = ServoMotion::mix(0, 1000);
    if (input.ne != -1000 || input.se != -1000) return false;
    input = ServoMotion::mix(1000, -1000);
    if (input.ne != 1000 || input.se != 0) return false;
    input = ServoMotion::mix(-1000, -1000);
    if (input.ne != 0 || input.se != 1000) return false;
    input = ServoMotion::mix(1000, 1000);
    if (input.ne != 0 || input.se != -1000) return false;
    input = ServoMotion::mix(-1000, 1000);
    return input.ne == -1000 && input.se == 0;
}
static_assert(directionTests(), "NE/SE direction mixing failed");

constexpr bool proportionalMixTests()
{
    auto input = ServoMotion::mix(250, -500);
    if (input.ne != 750 || input.se != 250) return false;
    input = ServoMotion::mix(500, -1000);
    if (input.ne != 1000 || input.se != 333) return false;
    input = ServoMotion::mix(-500, 1000);
    return input.ne == -1000 && input.se == -333;
}
static_assert(proportionalMixTests(), "NE/SE proportional scaling failed");

constexpr bool equalTravelTests()
{
    const float travel = ServoMotion::linkageTravel();
    if (travel <= 0 || HubConfig::NE_CENTER - travel < HubConfig::NE_MIN ||
        HubConfig::NE_CENTER + travel > HubConfig::NE_MAX ||
        HubConfig::SE_CENTER - travel < HubConfig::SE_MIN ||
        HubConfig::SE_CENTER + travel > HubConfig::SE_MAX) return false;
    for (unsigned mode = 0; mode < 3; ++mode)
    {
        const float scale = ServoMotion::speedScale(static_cast<CommandProtocol::SpeedMode>(mode));
        const float ne = ServoMotion::axisTarget(1000, HubConfig::NE_CENTER, HubConfig::NE_CENTER - travel, HubConfig::NE_CENTER + travel, scale);
        const float se = ServoMotion::axisTarget(-1000, HubConfig::SE_CENTER, HubConfig::SE_CENTER - travel, HubConfig::SE_CENTER + travel, scale);
        const float difference = (ne - HubConfig::NE_CENTER) - (HubConfig::SE_CENTER - se);
        if (difference < -0.001f || difference > 0.001f) return false;
    }
    return true;
}
static_assert(equalTravelTests(), "NE/SE travel limits or equal rotation failed");