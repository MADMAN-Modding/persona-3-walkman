#include "BluetoothA2DPSink.h"
#include "driver/i2s.h"

BluetoothA2DPSink a2dp_sink;

#define I2S_DOUT  22
#define I2S_BCLK  26
#define I2S_LRC   25

// Button pin
const int BUTTON = 0;  // GPIO0 (BOOT button)

// Button state tracking with noise filtering
bool stableButtonState = HIGH;
unsigned long stableStartTime = 0;
unsigned long stableDelay = 100;  // Button must be stable for 100ms

// Multi-click detection
unsigned long lastPress = 0;
unsigned long clickWindow = 400;  // 400ms window between clicks
int clickCount = 0;
bool processingClicks = false;

// Connection tracking
bool isConnected = false;

// Playstate tracking
bool isPlaying = false;

void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  switch (id) {
    case 0x1:
      Serial.printf("♫ Title: %s\n", text);
      break;
    case 0x2:
      Serial.printf("♫ Artist: %s\n", text);
      break;
    case 0x4:
      Serial.printf("♫ Album: %s\n", text);
      break;
  }
}

void connection_state_changed(esp_a2d_connection_state_t state, void *ptr) {
  if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
    isConnected = true;
    Serial.println("✓ Bluetooth Connected");
  } else {
    isConnected = false;
    Serial.println("✗ Bluetooth Disconnected");
  }
}

void read_data_stream(const uint8_t *data, uint32_t length) {
  size_t bytes_written;
  i2s_write(I2S_NUM_0, data, length, &bytes_written, portMAX_DELAY);
}

void avrc_rn_playstatus_callback(esp_avrc_playback_stat_t playback) {
  switch (playback) {
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_STOPPED:
      Serial.println("Stopped.");
      isPlaying = false;
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_PLAYING:
      Serial.println("Playing.");
      isPlaying = true;
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_PAUSED:
      Serial.println("Paused.");
      isPlaying = false;
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_FWD_SEEK:
      Serial.println("Forward seek.");
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_REV_SEEK:
      Serial.println("Reverse seek.");
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_ERROR:
      Serial.println("Error.");
      break;
    default:
      Serial.printf("Got unknown playback status %d\n", playback);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUTTON, INPUT_PULLUP);

  // Configure I2S
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = -1
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_0);

  a2dp_sink.set_stream_reader(read_data_stream, false);
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.set_avrc_rn_playstatus_callback(avrc_rn_playstatus_callback);
  a2dp_sink.set_on_connection_state_changed(connection_state_changed);
  a2dp_sink.set_avrc_metadata_attribute_mask(
    ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM);
  a2dp_sink.set_auto_reconnect(true);
  a2dp_sink.start("P3R Walkman");
  
  Serial.println("ESP32 Bluetooth Audio Receiver Ready!");
  Serial.println("1 click = Play/Pause");
  Serial.println("2 clicks = Next Track");
  Serial.println("3 clicks = Previous Track");
}

void loop() {
  bool reading = digitalRead(BUTTON);

  if (reading == stableButtonState) {
    stableStartTime = millis();
  } else {
    if ((millis() - stableStartTime) > stableDelay) {
      if (stableButtonState == HIGH && reading == LOW) {
        clickCount++;
        lastPress = millis();
        processingClicks = true;
        Serial.printf("Click %d\n", clickCount);
      }
      stableButtonState = reading;
      stableStartTime = millis();
    }
  }

  if (processingClicks && (millis() - lastPress) > clickWindow) {
    if (isConnected) {
      if (clickCount == 1) {
        Serial.println("⏯ Toggle Play/Pause");
        if (isPlaying) {
          esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PAUSE, ESP_AVRC_PT_CMD_STATE_PRESSED);
        } else {
          esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PLAY, ESP_AVRC_PT_CMD_STATE_PRESSED);
        }
      } else if (clickCount == 2) {
        Serial.println("⏭ Next Track");
        a2dp_sink.next();
      } else if (clickCount >= 3) {
        Serial.println("⏮ Previous Track");
        a2dp_sink.previous();
      }
    } else {
      Serial.println("Not connected!");
    }

    clickCount = 0;
    processingClicks = false;
  }

  delay(10);
}