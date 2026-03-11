#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class screen {
public:
  enum animation_control {
    NEXT,
    BACK,
    PAUSE,
    NONE
  };

  static void setup();

  static void updateDisplay();

  static void set_track(String track);

  static void set_album(String album);

  static void set_artist(String artist);

  static void set_animation_state(animation_control animation);

  static animation_control get_animation_state();

private:
  /// @brief Milliseconds between scroll steps
  static const unsigned long scrollDelay = 150; // 3/20 seconds
  static const unsigned long scrollPauseTime = 3000;  // 3 seconds pause at start
  static const int maxChars = 18;  // Max characters that fit after symbol (21 - 3)
  
  static void metadata_display();

  static void draw_next_arrow();

  static void draw_back_animation();

  static void draw_pause_animation();
};