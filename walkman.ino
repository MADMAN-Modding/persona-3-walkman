#include "BluetoothA2DPSink.h"
#include "driver/i2s.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

BluetoothA2DPSink a2dp_sink;

#define I2S_DOUT  22
#define I2S_BCLK  26
#define I2S_LRC   25

// OLED Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1  // No reset pin
#define SCREEN_ADDRESS 0x3C  // Common I2C address (try 0x3D if this doesn't work)

// Track name scrolling
int scrollPosition = 0;
unsigned long lastScrollTime = 0;
unsigned long scrollDelay = 150;  // Milliseconds between scroll steps
unsigned long scrollPauseTime = 3000;  // 3 seconds pause at start
bool scrollPaused = true;
unsigned long scrollPauseStart = 0;
int maxChars = 18;  // Max characters that fit after symbol (21 - 3)

// Default I2C pins for ESP32
#define I2C_SDA 21
#define I2C_SCL 19

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Button pin
const int BUTTON = 15; 

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
bool isPlaying = false;

// Track info
String currentTitle = "";
String currentArtist = "";
String currentAlbum = "";

const unsigned char PROGMEM pauseIcon[] = {
  0b01101100,
  0b01101100,
  0b01101100,
  0b01101100,
  0b01101100,
  0b01101100,
  0b01101100
};

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  if (!isConnected) {
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("P3R WALKMAN");
    display.setTextSize(1);
    display.println("Waiting for Bluetooth...");
    scrollPosition = 0;
    scrollPaused = true;
  } else {
    // Show play/pause symbol (stays in place)
    display.setCursor(0, 0);
    if (isPlaying) {
      display.print("\x10");
    } else {
      display.drawBitmap(0, 0, pauseIcon, 8, 7, SSD1306_WHITE);
    }

    display.print(" ");
    
    // Show track info with scrolling title
    if (currentTitle.length() > 0) {
      String title = currentTitle;
      int symbolWidth = 3;  // Width taken by play/pause symbol and space
      
      
      if (title.length() > maxChars) {
        // Title needs scrolling
        unsigned long currentTime = millis();
        
        if (scrollPaused) {
          // Check if pause time is over
          if (currentTime - scrollPauseStart >= scrollPauseTime) {
            scrollPaused = false;
            lastScrollTime = currentTime;
          }
          // Show from beginning during pause
          display.setCursor(18, 0);  // Position after symbol
          display.println(title.substring(0, maxChars));
        } else {
          // Scroll the text
          if (currentTime - lastScrollTime >= scrollDelay) {
            scrollPosition++;
            
            // Reset when we've scrolled through entire title
            if (scrollPosition > title.length() - maxChars) {
              scrollPosition = 0;
              scrollPaused = true;
              scrollPauseStart = currentTime;
            }
            
            lastScrollTime = currentTime;
          }
          
          // Display scrolled portion
          display.setCursor(18, 0);  // Position after symbol
          display.println(title.substring(scrollPosition, scrollPosition + maxChars));
        }
      } else {
        // Title fits, no scrolling needed
        display.setCursor(18, 0);  // Position after symbol
        display.println(title);
        scrollPosition = 0;
        scrollPaused = true;
      }
    } else {
      display.setCursor(18, 0);
      display.println("No Track Info");
      scrollPosition = 0;
      scrollPaused = true;
    }
    
    // Show artist on second line (y=10)
    if (currentArtist.length() > 0) {
      String artist = currentArtist;
      if (artist.length() > 21) {
        artist = artist.substring(0, 21);
      }
      display.setCursor(0, 10);
      display.println(artist);
    }

    // Show album on the third line (y=20)
    if (currentAlbum.length() > 0) {
      String album = currentAlbum;
      if (album.length() > 21) {
        album = album.substring(0, 21);
      }
      display.setCursor(0, 20);
      display.println(album);
    }
  }
  
  display.display();
}

void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  switch (id) {
    case 0x1:
      currentTitle = cleanText(String((char*)text));
      Serial.printf("♫ Title: %s\n", text);
      scrollPosition = 0;
      scrollPaused = true;
      scrollPauseStart = millis();
      updateDisplay();
      break;
    case 0x2:
      currentArtist = cleanText(String((char*)text));
      Serial.printf("♫ Artist: %s\n", text);
      updateDisplay();
      break;
    case 0x4:
      currentAlbum = cleanText(String((char*)text));
      Serial.printf("♫ Album: %s\n", text);
      updateDisplay();
      break;
  }
}

void connection_state_changed(esp_a2d_connection_state_t state, void *ptr) {
  if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
    isConnected = true;
    Serial.println("✓ Bluetooth Connected");
    updateDisplay();
  } else {
    isConnected = false;
    currentTitle = "";
    currentArtist = "";
    currentAlbum = "";
    Serial.println("✗ Bluetooth Disconnected");
    updateDisplay();
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
      updateDisplay();
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_PLAYING:
      Serial.println("Playing.");
      isPlaying = true;
      updateDisplay();
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_PAUSED:
      Serial.println("Paused.");
      isPlaying = false;
      updateDisplay();
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

  // Initialize I2C for OLED
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    // Continue anyway - display just won't work
  } else {
    Serial.println("OLED initialized");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("P3R WALKMAN");
    display.setTextSize(1);
    display.println("Starting...");
    display.display();
    delay(1000);
  }

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
  a2dp_sink.start("Waiting for bluetooth on P3R Walkman");
  delay(500);  // Wait for A2DP to initialize
  a2dp_sink.set_volume(12);  // 6/127 ≈ 5%
  
  Serial.println("ESP32 Bluetooth Audio Receiver Ready!");
  Serial.println("1 click = Play/Pause");
  Serial.println("2 clicks = Next Track");
  Serial.println("3 clicks = Previous Track");
  
  updateDisplay();
}

String cleanText(String text) {
  text.replace("â€™", "'");  // Right single quote
  text.replace("â€˜", "'");  // Left single quote
  text.replace("â€œ", "\""); // Left double quote
  text.replace("â€", "\"");  // Right double quote
  text.replace("â€", "-");  // Em dash
  text.replace("â€", "-");  // En dash
  text.replace("Ã©", "e");
  text.replace("Ã¡", "a");
  text.replace("Ã³", "o");
  text.replace("Ã­", "i");
  text.replace("Ãº", "u");
  return text;
}

void loop() {
  bool buttonState = digitalRead(BUTTON);

  // Detect button press (HIGH to LOW transition)
  if (buttonState == LOW && lastButtonState == HIGH) {
    delay(50);  // Debounce
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

  // Update display for scrolling animation
  if (isConnected && currentTitle.length() > 18) {
    updateDisplay();
  }

  delay(10);
}