#include "driver/i2s.h"

class playback {
  public:
    static void handle_audio(const uint8_t *data, uint32_t length);

    static void setup();
};