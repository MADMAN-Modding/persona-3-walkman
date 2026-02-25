#include "playback.h"
#include "bluetooth.h"
#include "screen.h"

// Button pin
const int BUTTON = 15;
// Button state tracking
bool lastButtonState = HIGH;

// Multi-click detection
unsigned long lastPress = 0;
unsigned long clickWindow = 400;  // 400ms window between clicks
int clickCount = 0;
bool processingClicks = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUTTON, INPUT_PULLUP);

  // Audio setup
  playback::setup();

  // Screen setup
  screen::setup();

  // Bluetooth setup
  bluetooth::setup();

  Serial.println("ESP32 Bluetooth Audio Receiver Ready!");
  Serial.println("1 click = Play/Pause");
  Serial.println("2 clicks = Next Track");
  Serial.println("3 clicks = Previous Track");
}

void loop() {
  bool buttonState = digitalRead(BUTTON);

  // Detect button press (HIGH to LOW transition)
  if (buttonState == LOW && lastButtonState == HIGH) {
    delay(50);                         // Debounce
    if (digitalRead(BUTTON) == LOW) {  // Confirm it's still pressed
      clickCount++;
      lastPress = millis();
      processingClicks = true;
      Serial.printf("Click %d\n", clickCount);
    }
  }
  lastButtonState = buttonState;

  // Process clicks after window expires
  if (processingClicks && (millis() - lastPress) > clickWindow) {
    if (bluetooth::getConnected()) {
      if (clickCount == 1) {
        Serial.println("⏯ Toggle Play/Pause");
        if (bluetooth::getPlaying()) {
          esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PAUSE, ESP_AVRC_PT_CMD_STATE_PRESSED);
        } else {
          esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PLAY, ESP_AVRC_PT_CMD_STATE_PRESSED);
        }
      } else if (clickCount == 2) {
        Serial.println("⏭ Next Track");
        bluetooth::next();
      } else if (clickCount >= 3) {
        Serial.println("⏮ Previous Track");
        bluetooth::previous();
      }
    } else {
      Serial.println("Not connected!");
    }

    clickCount = 0;
    processingClicks = false;
  }

  screen::updateDisplay();
  delay(10);
}