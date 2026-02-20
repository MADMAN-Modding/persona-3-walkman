#include "screen.h"
#include "bluetooth.h"

// OLED Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1        // No reset pin
#define SCREEN_ADDRESS 0x3C  // Common I2C address (try 0x3D if this doesn't work)

//I2C pins for ESP32
#define I2C_SDA 21
#define I2C_SCL 19

static String currentTrack = "";
static String currentArtist = "";
static String currentAlbum = "";

/// @brief Position for tracking the position of the text
static int scrollPosition = 0;
/// @brief Is the scroll paused
static bool scrollPaused = false;
/// @brief Used to detect if the scroll direction should be left or right
static bool scrollLeft = true;
/// @brief Time stamp for current pause on scroll (millis)
static unsigned long scrollPauseStart = 0;
/// @brief Last scrolls time stamp (millis)
static unsigned long lastScrollTime = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const unsigned char PROGMEM pauseIcon[] = {
  0b01101100,
  0b01101100,
  0b01101100,
  0b01101100,
  0b01101100,
  0b01101100,
  0b01101100
};

void screen::setup() {
  // Initialize I2C for OLED
  Wire.begin(I2C_SDA, I2C_SCL);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
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
}

void screen::avrc_rn_playstatus_callback(esp_avrc_playback_stat_t playback) {
  switch (playback) {
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_STOPPED:
      Serial.println("Stopped.");
      bluetooth::setPlaying(false);
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_PLAYING:
      Serial.println("Playing.");
      bluetooth::setPlaying(true);
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_PAUSED:
      Serial.println("Paused.");
      bluetooth::setPlaying(false);
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

void screen::updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (!bluetooth::getConnected()) {
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
    if (bluetooth::getPlaying()) {
      display.print("\x10");
    } else {
      display.drawBitmap(0, 0, pauseIcon, 8, 7, SSD1306_WHITE);
    }

    display.print(" ");

    // Show track info with scrolling Track
    if (currentTrack.length() > 0) {
      String track = currentTrack;
      int symbolWidth = 3;  // Width taken by play/pause symbol and space


      if (track.length() > maxChars) {
        // Time of the system
        unsigned long currentTime = millis();

        if (scrollPaused) {
          // Check if pause time is over
          if (currentTime - scrollPauseStart >= scrollPauseTime) {
            scrollPaused = false;
            lastScrollTime = currentTime;
          }
          // Show from beginning during pause
          display.setCursor(18, 0);  // Position after symbols
          if (scrollLeft) {
            display.println(track.substring(0, maxChars));
          } else {
            int offset = track.length() - maxChars;
            String subTrack = track.substring(offset, track.length());
            display.println(track.substring(offset, track.length()));
          }
        } else {
          // Scroll the text
          if (currentTime - lastScrollTime >= scrollDelay) {
            scrollPosition += scrollLeft ? 1 : -1;

            // Reset when we've scrolled through entire Track (the minus 1 prevents the text from going too far off to the side)
            if (scrollPosition > track.length() - maxChars - 1) {
              scrollLeft = false;
              scrollPaused = true;
              scrollPauseStart = currentTime;
            // If scrolled all the way to the right
            } else if (scrollPosition == 0) {
              scrollLeft = true;
              scrollPaused = true;
              scrollPauseStart = currentTime;
            }

            lastScrollTime = currentTime;
          }

          // Display scrolled portion
          display.setCursor(18, 0);  // Position after symbol
          display.println(track.substring(scrollPosition, scrollPosition + maxChars));
        }
      } else {
        // Track fits, no scrolling needed
        display.setCursor(18, 0);  // Position after symbol
        display.println(track);
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

void screen::setTrack(String track) {
  currentTrack = track;
}

void screen::setAlbum(String album) {
  currentAlbum = album;
}

void screen::setArtist(String artist) {
  currentArtist = artist;
}
