#include "BluetoothA2DPSink.h"

BluetoothA2DPSink a2dp_sink;

void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  Serial.printf("Metadata - ID: 0x%x, Value: %s\n", id, text);
  
  // Metadata attribute IDs:
  // 0x1 = Title
  // 0x2 = Artist  
  // 0x4 = Album
  // 0x7 = Track Number
}

// Empty audio callback - we don't process audio
void read_data_stream(const uint8_t *data, uint32_t length) {
  // Do nothing - just discard the audio data
}

void setup() {
  Serial.begin(115200);
  
  // Set the audio callback to our empty function
  a2dp_sink.set_stream_reader(read_data_stream, false);
  
  // Set metadata callback to receive track info
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  
  // Enable AVRCP metadata attribute notifications
  a2dp_sink.set_avrc_metadata_attribute_mask(ESP_AVRC_MD_ATTR_TITLE | 
                                              ESP_AVRC_MD_ATTR_ARTIST | 
                                              ESP_AVRC_MD_ATTR_ALBUM);
  
  a2dp_sink.start("ESP32_Remote");
}

void loop() {
  // Control playback
  delay(5000);
  a2dp_sink.pause();
  Serial.println("Paused");
  
  delay(2000);
  a2dp_sink.play();
  Serial.println("Playing");
  
  delay(5000);
  a2dp_sink.next();
  Serial.println("Next track");
  
  delay(5000);
  a2dp_sink.previous();
  Serial.println("Previous track");
}