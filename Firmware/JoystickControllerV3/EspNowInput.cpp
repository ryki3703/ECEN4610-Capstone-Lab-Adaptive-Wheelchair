#include "EspNowInput.h"
#include "HubConfig.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_random.h>
#include <string.h>

EspNowInput *EspNowInput::instance = nullptr;

bool EspNowInput::begin()
{
    bool allZero = true;
    for (uint8_t byte : HubConfig::REMOTE_MAC) if (byte != 0) allZero = false;
    remoteConfigured = !allZero && (HubConfig::REMOTE_MAC[0] & 1) == 0;
    if (!remoteConfigured)
        Serial.println("Set HubConfig::REMOTE_MAC; remote packets currently rejected");

    queue = xQueueCreate(HubConfig::RX_QUEUE_LENGTH, sizeof(InputEvent));
    if (queue == nullptr) return false;
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    Serial.print("Hub station MAC: ");
    Serial.println(WiFi.macAddress());
    if (esp_wifi_set_channel(HubConfig::WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK ||
        esp_now_init() != ESP_OK) return false;
    instance = this;
    if (esp_now_register_recv_cb(onReceive) != ESP_OK)
    {
        instance = nullptr;
        return false;
    }
    if (remoteConfigured)
    {
        esp_now_peer_info_t peer = {};
        memcpy(peer.peer_addr, HubConfig::REMOTE_MAC, 6);
        peer.channel = HubConfig::WIFI_CHANNEL;
        peer.ifidx = WIFI_IF_STA;
        peer.encrypt = false;
        statusPeerReady = esp_now_is_peer_exist(peer.peer_addr) || esp_now_add_peer(&peer) == ESP_OK;
        if (!statusPeerReady) Serial.println("Status peer unavailable; feedback disabled");
    }
    do { statusSession = esp_random(); } while (statusSession == 0);
    lastStatusTime = millis() - 200;
    return true;
}

bool EspNowInput::publishStatus(CommandProtocol::SpeedMode speed, uint8_t receiverMode,
                                bool armed, bool ready)
{
    const uint32_t now = millis();
    if (!statusPeerReady || now - lastStatusTime < 200) return false;
    lastStatusTime = now;
    const HubStatusProtocol::Status status = {
        speed, receiverMode, armed && ready, ready, statusSession, ++statusSequence
    };
    const auto packet = HubStatusProtocol::encode(status);
    // Telemetry is best-effort; retry on the next interval without delaying servos.
    return esp_now_send(HubConfig::REMOTE_MAC, packet.bytes, sizeof(packet.bytes)) == ESP_OK;
}

void EspNowInput::onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int length)
{
    EspNowInput *self = instance;
    if (self == nullptr || !self->remoteConfigured || info == nullptr || data == nullptr ||
        length < 0 || memcmp(info->src_addr, HubConfig::REMOTE_MAC, 6) != 0) return;
    InputEvent event = {};
    if (!CommandProtocol::decode(data, static_cast<size_t>(length), event.command)) return;
    event.receivedAt = millis();
    // Wi-Fi task context: copy only; no servo writes, Serial I/O, or waiting here.
    if (xQueueSend(self->queue, &event, 0) != pdTRUE) self->overflow.store(true);
}

bool EspNowInput::poll(InputEvent &event)
{
    return queue != nullptr && xQueueReceive(queue, &event, 0) == pdTRUE;
}
void EspNowInput::discardPending()
{
    if (queue != nullptr) xQueueReset(queue);
}
bool EspNowInput::takeOverflow() { return overflow.exchange(false); }
