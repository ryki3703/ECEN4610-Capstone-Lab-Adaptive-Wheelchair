#ifndef MODE_BUTTON_H
#define MODE_BUTTON_H
#include <Arduino.h>
#include "HubConfig.h"

class ModeButton
{
public:
    void begin()
    {
        if (HubConfig::RECEIVER_MODE_BUTTON_PIN < 0) return;
        pinMode(HubConfig::RECEIVER_MODE_BUTTON_PIN, INPUT_PULLUP);
        lastReading = stable = digitalRead(HubConfig::RECEIVER_MODE_BUTTON_PIN);
        changedAt = millis();
    }
    bool pressed(uint32_t now)
    {
        if (HubConfig::RECEIVER_MODE_BUTTON_PIN < 0) return false;
        const bool reading = digitalRead(HubConfig::RECEIVER_MODE_BUTTON_PIN);
        if (reading != lastReading) { lastReading = reading; changedAt = now; }
        if (reading != stable && now - changedAt >= HubConfig::BUTTON_DEBOUNCE_MS)
        {
            stable = reading;
            return stable == LOW;
        }
        return false;
    }
private:
    bool lastReading = HIGH;
    bool stable = HIGH;
    uint32_t changedAt = 0;
};
#endif
