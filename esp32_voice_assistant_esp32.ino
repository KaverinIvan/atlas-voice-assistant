================= esp32/voice_assistant_esp32.ino ====================

// ============================================================
// Atlas Voice Assistant — ESP32-S3 (N16R8)
// Библиотеки: WebSockets (Links2004), ArduinoJson 6.x
// ============================================================

#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include "driver/i2s.h"

// ─── ЗАМЕНИТЕ ЭТИ ЗНАЧЕНИЯ ────────────────────────────────────
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
const char* SERVER_IP = "192.168.1.100";   // IP сервера ассистента
// ──────────────────────────────────────────────────────────────

const int   SERVER_PORT = 8002;
const char* WS_PATH     = "/ws";

// GPIO (см. docs/HARDWARE.md)
#define MIC_SD    42
#define MIC_WS    15
#define MIC_SCK   14
#define AMP_DIN   41
#define AMP_LRC   18
#define AMP_BCLK  21
#define AMP_SD    38

#define MIC_SAMPLE_RATE  16000
#define AMP_SAMPLE_RATE  22050
#define MIC_BUF_SIZE     640

WebSocketsClient ws;
enum State { IDLE, PLAYING };
State state = IDLE;
bool  wsConnected = false;
bool  paused = true;
int16_t micBuf[MIC_BUF_SIZE];

void webSocketEvent(WStype_t type, uint8_t *payload, size_t len);
void handleJson(const String& msg);
void printMicLevel(size_t bytes_read);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Atlas Voice Assistant ESP32-S3 ===");

  pinMode(AMP_SD, OUTPUT);
  digitalWrite(AMP_SD, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.printf("\nIP: %s\n", WiFi.localIP().toString().c_str());

  // I2S RX (mic)
  i2s_config_t rx_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = MIC_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
  };
  i2s_driver_install(I2S_NUM_0, &rx_config, 0, NULL);
  i2s_pin_config_t rx_pin = {
    .bck_io_num = MIC_SCK, .ws_io_num = MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE, .data_in_num = MIC_SD
  };
  i2s_set_pin(I2S_NUM_0, &rx_pin);

  // I2S TX (amp)
  i2s_config_t tx_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = AMP_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
  };
  i2s_driver_install(I2S_NUM_1, &tx_config, 0, NULL);
  i2s_pin_config_t tx_pin = {
    .bck_io_num = AMP_BCLK, .ws_io_num = AMP_LRC,
    .data_out_num = AMP_DIN, .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_set_pin(I2S_NUM_1, &tx_pin);

  ws.begin(SERVER_IP, SERVER_PORT, WS_PATH);
  ws.setReconnectInterval(5000);
  ws.enableHeartbeat(15000, 3000, 2);
  ws.onEvent(webSocketEvent);

  Serial.println("Ready.");
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t len) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("[WS] Connected");
      wsConnected = true;
      paused = true;
      break;
    case WStype_DISCONNECTED:
      Serial.println("[WS] Disconnected");
      wsConnected = false;
      paused = false;
      state = IDLE;
      digitalWrite(AMP_SD, LOW);
      i2s_zero_dma_buffer(I2S_NUM_1);
      break;
    case WStype_TEXT: {
      String msg = String((char*)payload, len);
      handleJson(msg);
      break;
    }
    case WStype_BIN:
      if (state == PLAYING) {
        size_t written;
        i2s_write(I2S_NUM_1, (char*)payload, len, &written, portMAX_DELAY);
      }
      break;
    default: break;
  }
}

void handleJson(const String& msg) {
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, msg) != DeserializationError::Ok) return;
  const char* event = doc["event"] | "";
  Serial.printf("[WS] event=%s\n", event);

  if (strcmp(event, "wake") == 0) {
    paused = true;
  } else if (strcmp(event, "listening") == 0) {
    paused = false;
    state = IDLE;
  } else if (strcmp(event, "transcribing") == 0 ||
             strcmp(event, "thinking") == 0 ||
             strcmp(event, "agent_thinking") == 0) {
    paused = true;
  } else if (strcmp(event, "tts_start") == 0) {
    state = PLAYING;
    i2s_zero_dma_buffer(I2S_NUM_1);
    digitalWrite(AMP_SD, HIGH);
  } else if (strcmp(event, "tts_end") == 0) {
    state = IDLE;
    paused = true;
    digitalWrite(AMP_SD, LOW);
    i2s_zero_dma_buffer(I2S_NUM_1);
  } else if (strcmp(event, "idle") == 0) {
    state = IDLE;
    paused = false;
    digitalWrite(AMP_SD, LOW);
  }
}

void loop() {
  ws.loop();
  if (!wsConnected) { delay(10); return; }

  size_t bytes_read = 0;
  if (i2s_read(I2S_NUM_0, (char*)micBuf, sizeof(micBuf), &bytes_read, 10) != ESP_OK) return;
  if (bytes_read == 0) return;

  if (state == PLAYING || paused) return;

  ws.sendBIN((uint8_t*)micBuf, bytes_read);
  printMicLevel(bytes_read);
}

void printMicLevel(size_t bytes_read) {
  static unsigned long last = 0;
  if (millis() - last < 500) return;
  int samples = bytes_read / 2;
  int64_t sum_sq = 0;
  for (int i = 0; i < samples; i++) {
    int32_t s = micBuf[i];
    sum_sq += s * s;
  }
  float rms = sqrtf((float)sum_sq / (float)samples);
  Serial.printf("[PCM] RMS=%.0f, state=%d, paused=%d, heap=%d\n",
                rms, (int)state, (int)paused, ESP.getFreeHeap());
  last = millis();
}