#include "bluetooth.h"
#include "playback.h"
#include "utilities.h"
#include "screen.h"

static BluetoothA2DPSink a2dp_sink;

static bool isConnected = false;

static bool isPlaying = false;

void bluetooth::setup() {
  a2dp_sink.set_stream_reader(playback::handle_audio, false);
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.set_avrc_rn_playstatus_callback(screen::avrc_rn_playstatus_callback);
  a2dp_sink.set_on_connection_state_changed(connection_state_changed);
  a2dp_sink.set_avrc_metadata_attribute_mask(
    ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM);
  a2dp_sink.set_auto_reconnect(true);
  a2dp_sink.start("P3R Walkman");
  delay(500);                // Wait for A2DP to initialize
  a2dp_sink.set_volume(12);  // 6/127 ≈ 5%
}

void bluetooth::next() {
  a2dp_sink.next();
  screen::draw_next_arrow();
}

void bluetooth::previous() {
  a2dp_sink.previous();
  screen::draw_back_animation();
}

void bluetooth::setPlaying(bool playing)
{
  isPlaying = playing;
}

bool bluetooth::getPlaying() {
  return isPlaying;
}

bool bluetooth::getConnected() {
  return isConnected;
}

void bluetooth::avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  switch (id) {
    case 0x1:
      screen::set_track(utilities::cleanText(String((char*)text)));
      Serial.printf("♫ Title: %s\n", text);
      break;
    case 0x2:
      screen::set_artist(utilities::cleanText(String((char*)text)));
      Serial.printf("♫ Artist: %s\n", text);
      break;
    case 0x4:
      screen::set_album(utilities::cleanText(String((char*)text)));
      Serial.printf("♫ Album: %s\n", text);
      break;
  }
}

void bluetooth::connection_state_changed(esp_a2d_connection_state_t state, void *ptr) {
  if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
    isConnected = true;
    Serial.println("Bluetooth Connected");
  } else {
    isConnected = false;
    screen::set_track("");
    screen::set_artist("");
    screen::set_album("");
    Serial.println("Bluetooth Disconnected");
  }
}