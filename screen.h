#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BluetoothA2DPSink.h>

class screen {
public:
  static void setup();

  static void updateDisplay();

  static void set_track(String track);

  static void set_album(String album);

  static void set_artist(String artist);

  static void avrc_rn_playstatus_callback(esp_avrc_playback_stat_t playback);

  static void draw_next_arrow();

  static void draw_back_animation();

private:
  /// @brief Milliseconds between scroll steps
  static const unsigned long scrollDelay = 150; // 3/20 seconds
  static const unsigned long scrollPauseTime = 3000;  // 3 seconds pause at start
  static const int maxChars = 18;  // Max characters that fit after symbol (21 - 3)
  
  static void setScrollPosition(int pose);

  static void resetScrollPosition() {
    setScrollPosition(0);
  };
};