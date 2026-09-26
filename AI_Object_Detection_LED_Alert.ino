/*
 * AI-Based Object Detection LED Alert System
 * Board   : ESP32-CAM (AI-Thinker module)
 * Model   : TinyML image-classification model (trained in Edge Impulse Studio)
 *           Classes -> "person", "background"
 * Purpose : Continuously classifies camera frames on-device. When a "person"
 *           is detected above a confidence threshold, an external LED and
 *           buzzer are triggered as a visual/audible alert, and the event is
 *           logged over Serial with a timestamp and confidence score.
 *
 * Repository: AI-Object-Detection-LED-Alert
 * File      : AI_Object_Detection_LED_Alert.ino
 * Author    : <your name>
 * Course    : Project Management (2307476T) - Activity 2
 */

#include <AI_Object_Detection_LED_Alert_inferencing.h>   // Edge Impulse exported library
#include "esp_camera.h"

// ---------- Pin configuration ----------
#define LED_PIN            2      // Onboard/external alert LED
#define BUZZER_PIN         14     // Optional audible alert
#define CONFIDENCE_THRESH  0.75f  // Raised from 0.60 after Issue #2 fix

// ---------- Camera pin map (AI-Thinker ESP32-CAM) ----------
#define PWDN_GPIO_NUM   32
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM   26
#define SIOC_GPIO_NUM   27
#define Y9_GPIO_NUM     35
#define Y8_GPIO_NUM     34
#define Y7_GPIO_NUM     39
#define Y6_GPIO_NUM     36
#define Y5_GPIO_NUM     21
#define Y4_GPIO_NUM     19
#define Y3_GPIO_NUM     18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM  25
#define HREF_GPIO_NUM   23
#define PCLK_GPIO_NUM   22

static bool camera_init_ok = false;

// Buffer that the classifier reads pixel data from
static uint8_t *snapshot_buf = NULL;

int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);

void setup() {
  Serial.begin(115200);           // Fixed after Issue #5 (baud mismatch)
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;   config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;   config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_QVGA;      // 320 x 240
  config.jpeg_quality  = 12;
  config.fb_count      = 1;

  camera_init_ok = (esp_camera_init(&config) == ESP_OK);
  if (!camera_init_ok) {
    Serial.println("[ERROR] Camera init failed. Check power supply (see Issue #3).");
  } else {
    Serial.println("[INFO] Camera initialised. Starting inference loop...");
  }
}

void loop() {
  if (!camera_init_ok) { delay(1000); return; }

  ei::signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data = &ei_camera_get_data;

  ei_impulse_result_t result = { 0 };
  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false /* debug */);
  if (err != EI_IMPULSE_OK) {
    Serial.printf("[ERROR] Classifier failed (%d)\n", err);
    return;
  }

  // Find the highest-confidence label
  float best_score = 0.0f;
  const char *best_label = "background";
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    if (result.classification[ix].value > best_score) {
      best_score = result.classification[ix].value;
      best_label = result.classification[ix].label;
    }
  }

  bool person_detected = (strcmp(best_label, "person") == 0) &&
                          (best_score >= CONFIDENCE_THRESH);

  // --- Fix for Issue #1: digitalWrite() moved INSIDE this conditional ---
  if (person_detected) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.printf("[ALERT] t=%lums  person detected  confidence=%.2f\n",
                  millis(), best_score);
  } else {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.printf("[INFO]  t=%lums  no person  best=%s (%.2f)\n",
                  millis(), best_label, best_score);
  }

  delay(200); // ~5 inferences/sec
}

// ---- Edge Impulse camera helper callbacks (capture + resize) ----
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[ERROR] Frame capture failed.");
    return false;
  }
  // Resize/convert fb->buf (JPEG) into out_buf as RGB888 at img_width x img_height
  // (conversion routine omitted here for brevity - see /src/image_utils.cpp in repo)
  esp_camera_fb_return(fb);
  return true;
}

int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
  // Converts RGB888 snapshot_buf bytes into the float format the model expects
  size_t pixel_ix = offset * 3;
  for (size_t i = 0; i < length; i++) {
    out_ptr[i] = (snapshot_buf[pixel_ix] << 16) +
                 (snapshot_buf[pixel_ix + 1] << 8) +
                  snapshot_buf[pixel_ix + 2];
    pixel_ix += 3;
  }
  return 0;
}
