#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>

#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_random.h>

// ===========================
// Select camera model in board_config.h
// ===========================
#include "board_config.h"
#include "CommandProtocol.h"

// ===========================
// Enter your WiFi credentials
// ===========================
const char *ap_ssid = "Wheelchair-001";
const char *ap_password = "WheelchairB3";

constexpr uint8_t WIFI_CHANNEL = 6;

extern const uint8_t hubMac[6] = {
  0x30, 0x30, 0xF9, 0x15, 0x26, 0x54
};

uint32_t driveSession = 0;
uint32_t driveSequence = 0;

void startCameraServer();
void setupLedFlash();

bool setupEspNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return false;
  }

  esp_now_peer_info_t peer = {};

  memcpy(peer.peer_addr, hubMac, 6);
  peer.channel = WIFI_CHANNEL;
  peer.ifidx = WIFI_IF_AP;
  peer.encrypt = false;

  if (!esp_now_is_peer_exist(hubMac)) {
    if (esp_now_add_peer(&peer) != ESP_OK) {
      Serial.println("Failed to add S3 ESP-NOW peer");
      return false;
    }
  }

  do {
    driveSession = esp_random();
  } while (driveSession == 0);

  Serial.print("Drive session: ");
  Serial.println(driveSession);

  Serial.println("ESP-NOW ready");

  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash();
#endif

  WiFi.softAP(
    ap_ssid,
    ap_password,
    WIFI_CHANNEL
  );

  WiFi.setSleep(false);
  setupEspNow();

  Serial.println("");
  Serial.println("Access Point started");
  Serial.print("Network: ");
  Serial.println(ap_ssid);

  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("' to connect");
}

void loop() {
  // Do nothing. Everything is done in another task by the web server
  delay(10000);
}
