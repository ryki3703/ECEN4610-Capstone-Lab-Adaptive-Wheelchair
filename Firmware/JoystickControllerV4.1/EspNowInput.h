/*
*   Class to receive remote commands, queue input, and send hub status over ESP-NOW
*/

#ifndef ESP_NOW_INPUT_H
#define ESP_NOW_INPUT_H

#include <Arduino.h>
#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <atomic>
#include "CommandProtocol.h"
#include "HubStatusProtocol.h"
#include "HubControl.h"


// Queue entry containing a decoded command and its arrival time
struct InputEvent
{
    CommandProtocol::DecodedCommand command;
    uint32_t receivedAt; // Arrival time in milliseconds - Used to reject old queued input
    ReceiverMode source;
};

class EspNowInput
{
public:
    bool begin();                   // Initialize the radio, input queue, and status peer
    bool poll(InputEvent &event);   // Take the next queued command without waiting
    void discardPending();          // Clear commands waiting in the queue
    bool takeOverflow();            // Read and clear the queue overflow flag
    // Send hub status when the reporting interval has elapsed
    bool publishStatus(CommandProtocol::SpeedMode speed, uint8_t receiverMode, bool armed, bool ready);
private:
    static EspNowInput *instance;       // Lets the static receive callback access this object
    QueueHandle_t queue = nullptr;      // Transfers input from the Wi-Fi callback to the main loop
    std::atomic<bool> overflow{false};  // Overflow flag shared safely between callback and loop
    bool remoteConfigured = false;      // Does the configured remote MAC pass the address checks?
    bool statusPeerReady = false;       // Is the remote registered as a status destination?
    uint32_t statusSession = 0;         // Nonzero identifier generated during initialization
    uint32_t statusSequence = 0;        // Counter for outgoing status packets
    uint32_t lastStatusTime = 0;        // Time of the previous status send attempt
    // Radio callback - Validates and queues input without commanding servos
    static void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int length);
};
#endif
