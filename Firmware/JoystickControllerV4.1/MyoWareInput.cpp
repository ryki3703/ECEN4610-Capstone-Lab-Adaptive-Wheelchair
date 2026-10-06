/*
*   Initializes the ADC and samples the two MyoWare 2.0 Link Shield envelope outputs
*/

#include "MyoWareInput.h"
#include <Arduino.h>


// Use 12-bit readings and the widest ADC input range on the ESP32-S3
void MyoWareInput::begin()
{
    pinMode(HubConfig::EMG1_PIN, INPUT);
    pinMode(HubConfig::EMG2_PIN, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(HubConfig::EMG1_PIN, ADC_11db);
    analogSetPinAttenuation(HubConfig::EMG2_PIN, ADC_11db);
    reset();
}

// Discard previous activity without resetting command order
void MyoWareInput::reset()
{
    // Start active so both readings must fall below their release thresholds to arm
    active1 = true;
    active2 = true;
    sampled = false;
}

// Generate one local drive command for each fresh sample pair
bool MyoWareInput::poll(CommandProtocol::DecodedCommand &command, uint32_t now)
{
    if (sampled && now - lastSampleTime < HubConfig::EMG_SAMPLE_PERIOD_MS) return false;
    lastSampleTime = now;
    sampled = true;
    const uint16_t sensor1 = analogRead(HubConfig::EMG1_PIN);
    const uint16_t sensor2 = analogRead(HubConfig::EMG2_PIN);
    command = {CommandProtocol::Command::DriveState, decode(sensor1, sensor2), 1, ++sequence};
    return true;
}
