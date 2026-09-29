/*
*   Button class - detects one debounced press on the selected pin
*/

#ifndef MODE_BUTTON_H
#define MODE_BUTTON_H

#include <Arduino.h>
#include "HubConfig.h"


class ModeButton
{
public:
    explicit ModeButton(int8_t buttonPin = HubConfig::RECEIVER_MODE_BUTTON_PIN) : pin(buttonPin) {}

    void begin()                    // Configure the button
    {
        // Skip while the button isn't wired (Prototyping necessity)
        if (pin < 0) return;

        pinMode(pin, INPUT_PULLUP);

        // Recent State = Accepted debounce state = Current state
        lastReading = stable = digitalRead(pin);
        // Record starting time for debounce tracking
        changedAt = millis();   
    }
    bool pressed(uint32_t now)      // Check for a new press
    {
        // Skip while the button isn't wired (Prototyping necessity)
        if (pin < 0) return false;
        
        // Read the pin
        const bool reading = digitalRead(pin);

        // Restart the timer when reading changes
        if (reading != lastReading) 
        { 
            lastReading = reading; changedAt = now; 
        }

        // Accept a change only after button has settled
        if (reading != stable && now - changedAt >= HubConfig::BUTTON_DEBOUNCE_MS)
        {
            stable = reading;
            return stable == LOW;
        }

        // False when no newly accepted state change
        return false;
    }
private:
    int8_t pin;                 // GPIO for this button - Negative disables it
    bool lastReading = HIGH;    
    bool stable = HIGH;
    uint32_t changedAt = 0;
};
#endif
