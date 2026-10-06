/*
*   Initializes ESP-NOW, checks incoming packets, and sends status back to the remote
*/

#include "EspNowInput.h"
#include "HubConfig.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_random.h>
#include <string.h>


// Object used by the static ESP-NOW receive callback
EspNowInput *EspNowInput::instance = nullptr;

// Initialize remote input and status feedback
bool EspNowInput::begin()
{
    bool remoteAllZero = true;
    bool camAllZero = true;

    for (uint8_t byte : HubConfig::REMOTE_MAC)
        if (byte != 0) remoteAllZero = false;

    for (uint8_t byte : HubConfig::CAM_MAC)
        if (byte != 0) camAllZero = false;

    remoteConfigured = !remoteAllZero && !camAllZero && (HubConfig::REMOTE_MAC[0] & 1) == 0 && (HubConfig::CAM_MAC[0] & 1) == 0;

    if (!remoteConfigured)
        Serial.println("Set valid REMOTE_MAC and CAM_MAC");

    // Create a bounded queue for transferring packets to the main loop
    queue = xQueueCreate(HubConfig::RX_QUEUE_LENGTH, sizeof(InputEvent));
    if (queue == nullptr) return false;
    // Use station mode and disable Wi-Fi sleep
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    Serial.print("Hub station MAC: ");
    Serial.println(WiFi.macAddress());
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
    esp_now_peer_info_t remotePeer = {};
    memcpy(remotePeer.peer_addr, HubConfig::REMOTE_MAC, 6);
    remotePeer.channel = HubConfig::WIFI_CHANNEL;
    remotePeer.ifidx = WIFI_IF_STA;
    remotePeer.encrypt = false;

    if (!esp_now_is_peer_exist(remotePeer.peer_addr))
    {
        if (esp_now_add_peer(&remotePeer) != ESP_OK)
        {
            Serial.println("Failed to add remote controller peer");
        }
    }

    statusPeerReady = esp_now_is_peer_exist(remotePeer.peer_addr);

    esp_now_peer_info_t camPeer = {};
    memcpy(camPeer.peer_addr, HubConfig::CAM_MAC, 6);
    camPeer.channel = HubConfig::WIFI_CHANNEL;
    camPeer.ifidx = WIFI_IF_STA;
    camPeer.encrypt = false;

    if (!esp_now_is_peer_exist(camPeer.peer_addr))
    {
        esp_now_add_peer(&camPeer);
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
void EspNowInput::onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int length)
{
    EspNowInput *self = instance;
    // Reject invalid callback data and packets from a different MAC address
    if (self == nullptr ||
        info == nullptr ||
        data == nullptr ||
        length < 0)
    {
        return;
    }

    ReceiverMode source;

    if (memcmp(info->src_addr, HubConfig::REMOTE_MAC, 6) == 0)
    {
        source = ReceiverMode::Controller;
    }
    else if (memcmp(info->src_addr, HubConfig::CAM_MAC, 6) == 0)
    {
        source = ReceiverMode::App;
    }
    else
    {
        return;
    }

    // Decode the command and record when it arrived
    InputEvent event = {};
    if (!CommandProtocol::decode(data, static_cast<size_t>(length), event.command))
    {
        return;
    }
    event.receivedAt = millis();
    event.source = source;

    // Wi-Fi task context: copy only; no servo writes, Serial I/O, or waiting here.
    // Queue without waiting - Flag overflow if no space is available
    if (xQueueSend(self->queue, &event, 0) != pdTRUE) self->overflow.store(true);
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
