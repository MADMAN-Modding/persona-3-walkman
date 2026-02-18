#include "bluetooth.h"
#include "playback.h"
#include "utilities.h"

void bluetooth::setup() {
  a2dp_sink.set_stream_reader(playback::handle_audio, false);
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.set_avrc_rn_playstatus_callback(avrc_rn_playstatus_callback);
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
}

void bluetooth::previous() {
  a2dp_sink.previous();
}

void bluetooth::avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  switch (id) {
    case 0x1:
      currentTitle = utilities::cleanText(String((char*)text));
      Serial.printf("♫ Title: %s\n", text);
      scrollPosition = 0;
      scrollPaused = true;
      scrollPauseStart = millis();
      updateDisplay();
      break;
    case 0x2:
      currentArtist = utilities::cleanText(String((char*)text));
      Serial.printf("♫ Artist: %s\n", text);
      updateDisplay();
      break;
    case 0x4:
      currentAlbum = utilities::cleanText(String((char*)text));
      Serial.printf("♫ Album: %s\n", text);
      updateDisplay();
      break;
  }
}

void bluetooth::avrc_rn_playstatus_callback(esp_avrc_playback_stat_t playback)
{
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

void bluetooth::connection_state_changed(esp_a2d_connection_state_t state, void *ptr) {
  if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
    isConnected = true;
    Serial.println("Bluetooth Connected");
  } else {
    isConnected = false;
    currentTitle = "";
    currentArtist = "";
    currentAlbum = "";
    Serial.println("Bluetooth Disconnected");
  }
  
  updateDisplay();
}
