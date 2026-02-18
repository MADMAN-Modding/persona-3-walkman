#include <BluetoothA2DPSink.h>
#include <Arduino.h>

class bluetooth {
public:
  static void setup();

  static void next();

  static void previous();

private:
  static BluetoothA2DPSink a2dp_sink;

  static bool isPlaying;

  static void avrc_rn_playstatus_callback(esp_avrc_playback_stat_t playback);

  static void avrc_metadata_callback(uint8_t id, const uint8_t *text);

  static void connection_state_changed(esp_a2d_connection_state_t state, void *ptr);

  static String currentTitle;
  static String currentArtist;
  static String currentAlbum;
};