#include "BluetoothA2DPSink.h"

BluetoothA2DPSink a2dp_sink;

// Button pin
const int BUTTON = 0;  // GPIO0 (BOOT button)

// Button state tracking
bool lastButtonState = HIGH;

// Multi-click detection
unsigned long lastPress = 0;
unsigned long clickWindow = 400;  // 400ms window between clicks
int clickCount = 0;
bool processingClicks = false;

// Connection tracking
bool isConnected = false;

// Playstate tracking
bool isPlaying = true;

void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  // Only print the metadata we care about
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
      // Ignore 0x8, 0x10, 0x20 and other IDs
  }
}

void read_data_stream(const uint8_t *data, uint32_t length) {
  // Discard audio data
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

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Setup button with internal pull-up
  pinMode(BUTTON, INPUT_PULLUP);

  a2dp_sink.set_stream_reader(read_data_stream, false);
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.set_avrc_rn_playstatus_callback(avrc_rn_playstatus_callback);
  a2dp_sink.set_on_connection_state_changed(connection_state_changed);


  // Only request Title, Artist, and Album metadata
  a2dp_sink.set_avrc_metadata_attribute_mask(
    ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM);

  // Enable auto-reconnect to last device
  a2dp_sink.set_auto_reconnect(true);

  a2dp_sink.start("Remote_Only_No_Audio");
  Serial.println("ESP32 Bluetooth Remote Ready!");
  Serial.println("Auto-reconnect enabled");
  Serial.println("1 click = Play/Pause");
  Serial.println("2 clicks = Next Track");
  Serial.println("3 clicks = Previous Track");
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

void loop() {
  // Handle button press
  bool buttonState = digitalRead(BUTTON);

  if (buttonState == LOW && lastButtonState == HIGH) {
    delay(50);  // Debounce
    if (digitalRead(BUTTON) == LOW) {
      clickCount++;
      lastPress = millis();
      processingClicks = true;
      Serial.printf("Click %d\n", clickCount);
    }
  }
  lastButtonState = buttonState;

  // Check if click window expired - execute command based on click count
  if (processingClicks && (millis() - lastPress) > clickWindow) {
    if (isConnected) {
      if (clickCount == 1) {
        // Single click - Toggle play/pause
        Serial.println("⏯ Toggle Play/Pause");
        // Send the AVRCP PAUSE passthrough command - acts as toggle on most devices
        bool previousIsPlaying = isPlaying;
        if (isPlaying) {
          esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PAUSE, ESP_AVRC_PT_CMD_STATE_PRESSED);
        } else {
          esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PLAY, ESP_AVRC_PT_CMD_STATE_PRESSED);
        }

        delay(200);

        // Checks to make sure that the music state did change
        if (previousIsPlaying == isPlaying) {
          if (isPlaying) {
            esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PAUSE, ESP_AVRC_PT_CMD_STATE_PRESSED);
          } else {
            esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PLAY, ESP_AVRC_PT_CMD_STATE_PRESSED);
          }
        }
      } else if (clickCount == 2) {
        // Double click - Next Track
        Serial.println("⏭ Next Track");
        delay(100);
        a2dp_sink.next();
      } else if (clickCount >= 3) {
        // Triple click - Previous Track
        Serial.println("⏮ Previous Track");
        delay(100);
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