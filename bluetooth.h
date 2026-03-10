#include <BluetoothA2DPSink.h>

class bluetooth {
public:
  static void setup();

  static void next();

  static void previous();

  static void setPlaying(bool playing);

  static bool getPlaying();

  static bool getConnected();
private:
  static void avrc_metadata_callback(uint8_t id, const uint8_t *text);

  static void connection_state_changed(esp_a2d_connection_state_t state, void *ptr);

  static void avrc_rn_playstatus_callback(esp_avrc_playback_stat_t playback);
};