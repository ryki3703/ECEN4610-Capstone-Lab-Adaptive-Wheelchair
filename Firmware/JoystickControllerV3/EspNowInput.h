#ifndef ESP_NOW_INPUT_H
#define ESP_NOW_INPUT_H

#include <Arduino.h>
#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <atomic>
#include "CommandProtocol.h"
#include "HubStatusProtocol.h"

struct InputEvent
{
    CommandProtocol::DecodedCommand command;
    uint32_t receivedAt;
};

class EspNowInput
{
public:
    bool begin();
    bool poll(InputEvent &event);
    void discardPending();
    bool takeOverflow();
    bool publishStatus(CommandProtocol::SpeedMode speed, uint8_t receiverMode,
                       bool armed, bool ready);
private:
    static EspNowInput *instance;
    QueueHandle_t queue = nullptr;
    std::atomic<bool> overflow{false};
    bool remoteConfigured = false;
    bool statusPeerReady = false;
    uint32_t statusSession = 0;
    uint32_t statusSequence = 0;
    uint32_t lastStatusTime = 0;
    static void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int length);
};
#endif
