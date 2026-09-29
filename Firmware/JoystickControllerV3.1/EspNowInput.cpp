/*
*   Initializes ESP-NOW, checks incoming packets, and sends status back to the remote
*/

#include "EspNowInput.h"
#include "HubConfig.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_random.h>
#include <string.h>
#include <esp_mac.h>


// Object used by the static ESP-NOW receive callback
EspNowInput *EspNowInput::instance = nullptr;

// Initialize remote input and status feedback
bool EspNowInput::begin()
{
    // Check the configured MAC is nonzero and is not a group address
    bool allZero = true;
    for (uint8_t byte : HubConfig::REMOTE_MAC) if (byte != 0) allZero = false;
    remoteConfigured = !allZero && (HubConfig::REMOTE_MAC[0] & 1) == 0;
    if (!remoteConfigured)
        Serial.println("Set HubConfig::REMOTE_MAC; remote packets currently rejected");

    // Create a bounded queue for transferring packets to the main loop
    queue = xQueueCreate(HubConfig::RX_QUEUE_LENGTH, sizeof(InputEvent));
    if (queue == nullptr) return false;
    // Use station mode and disable Wi-Fi sleep
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);

    uint8_t mac[6];

    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    Serial.printf(
        "Hub STA MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
        mac[0], mac[1], mac[2],
        mac[3], mac[4], mac[5]
    );

    // Set the shared channel and initialize ESP-NOW
    if (esp_wifi_set_channel(HubConfig::WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK || esp_now_init() != ESP_OK) return false;

    // Connect this object to the receive callback
    instance = this;
    if (esp_now_register_recv_cb(onReceive) != ESP_OK)
    {
        instance = nullptr;
        return false;
    }

    // Register the remote as a destination for status packets
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
    // Choose a nonzero session identifier for this initialization
    do { statusSession = esp_random(); } while (statusSession == 0);
    lastStatusTime = millis() - 200; // Allow the first status packet to send immediately
    return true;
}

// Send status at most once every 200 milliseconds
bool EspNowInput::publishStatus(CommandProtocol::SpeedMode speed, uint8_t receiverMode, bool armed, bool ready)
{
    const uint32_t now = millis();
    if (!statusPeerReady || now - lastStatusTime < 200) return false;

    lastStatusTime = now;
    // Only report armed when ready - Increment the status packet counter
    const HubStatusProtocol::Status status = {speed, receiverMode, armed && ready, ready, statusSession, ++statusSequence};
    const auto packet = HubStatusProtocol::encode(status);

    // Telemetry is best-effort; retry on the next interval without delaying servos.
    // True means the send was accepted locally, not that the remote received it
    return esp_now_send(HubConfig::REMOTE_MAC, packet.bytes, sizeof(packet.bytes)) == ESP_OK;
}

// Check incoming packets in the Wi-Fi task
void EspNowInput::onReceive(
    const esp_now_recv_info_t *info,
    const uint8_t *data,
    int length)
{
    EspNowInput *self = instance;

    if (self == nullptr ||
        !self->remoteConfigured ||
        info == nullptr ||
        data == nullptr ||
        length < 0 ||
        memcmp(info->src_addr, HubConfig::REMOTE_MAC, 6) != 0)
    {
        return;
    }

    InputEvent event = {};

    if (!CommandProtocol::decode(
            data,
            static_cast<size_t>(length),
            event.command))
    {
        return;
    }

    event.receivedAt = millis();

    // Wi-Fi task context: copy only.
    // No servo writes, Serial I/O, or waiting here.
    if (xQueueSend(self->queue, &event, 0) != pdTRUE)
    {
        self->overflow.store(true);
    }
}

// Return the next queued event if one is available
bool EspNowInput::poll(InputEvent &event)
{
    return queue != nullptr && xQueueReceive(queue, &event, 0) == pdTRUE;
}

// Discard queued input after a stop or receiver change
void EspNowInput::discardPending()
{
    if (queue != nullptr) xQueueReset(queue);
}

// Return the overflow state and clear it in one atomic operation
bool EspNowInput::takeOverflow()
{ 
    return overflow.exchange(false); 
}
