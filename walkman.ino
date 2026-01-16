#include "BluetoothA2DPSink.h"

BluetoothA2DPSink a2dp_sink;

// Button pin
const int BUTTON = 0;  // GPIO0 (BOOT button)

// Button state tracking
bool lastButtonState = HIGH;
bool isPlaying = false;

// Multi-click detection
unsigned long lastPress = 0;
unsigned long clickWindow = 400; // 400ms window between clicks
int clickCount = 0;
bool processingClicks = false;

// Connection tracking
bool isConnected = false;

void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  Serial.printf("Metadata - ID: 0x%x, Value: %s\n", id, text);
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
  a2dp_sink.set_on_connection_state_changed(connection_state_changed);
  
  a2dp_sink.start("ESP32_Remote");
  Serial.println("ESP32 Bluetooth Remote Ready!");
  Serial.println("1 click = Play/Pause");
  Serial.println("2 clicks = Next Track");
  Serial.println("3 clicks = Previous Track");
}

void loop() {
  // Handle button press
  bool buttonState = digitalRead(BUTTON);
  
  if (buttonState == LOW && lastButtonState == HIGH) {
    delay(50); // Debounce
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
        // Single click - Play/Pause
        isPlaying = !isPlaying;
        if (isPlaying) {
          a2dp_sink.play();
          Serial.println("▶ Play");
        } else {
          a2dp_sink.pause();
          Serial.println("⏸ Pause");
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