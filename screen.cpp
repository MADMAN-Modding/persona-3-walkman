#include "screen.h"
#include "bluetooth.h"
#include "animations.h"

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
static bool scrollPaused = true;
/// @brief Used to detect if the scroll direction should be left or right
static bool scrollLeft = true;
/// @brief Time stamp for current pause on scroll (millis)
static unsigned long scrollPauseStart = 0;
/// @brief Last scrolls time stamp (millis)
static unsigned long lastScrollTime = 0;

// Animation tracking to handle button logic while an animation is in loop
static const unsigned char PROGMEM previous_animation[] = {};
static const unsigned char PROGMEM current_animation[] = {};
static unsigned int animation_index = 0;


static enum screen::animation_control animation = screen::animation_control::NONE;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


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

void screen::set_animation_state(screen::animation_control new_animation) {
	animation = new_animation;
	animation_index = 0;
}

void screen::updateDisplay() {
	if (!bluetooth::getPlaying()) {
		animation = animation_control::PAUSE;
	}

	switch (animation) {
		case animation_control::NEXT:
			draw_next_arrow();
			break;
		case animation_control::BACK:
			draw_back_animation();
			break;
		case animation_control::PAUSE:
			draw_pause_animation();
			break;
		case animation_control::NONE:
			metadata_display();
			break;
	}
}

void screen::metadata_display() {
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
			display.drawBitmap(0, 0, animations::get_pause_icon(), 8, 7, SSD1306_INVERSE);
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

void screen::set_track(String track) {
	currentTrack = track;
	scrollPosition = 0;
	scrollPaused = true;
	scrollLeft = true;
	scrollPauseStart = millis();
	lastScrollTime = 0;
}

void screen::set_album(String album) {
	currentAlbum = album;
}

void screen::set_artist(String artist) {
	currentArtist = artist;
}

void screen::draw_next_arrow() {
	unsigned int animation_length = 208;

	display.clearDisplay();
	display.drawBitmap(-80 + animation_index, 0, animations::get_next_animation(), 80, 32, SSD1306_WHITE);
	display.display();

	animation_index += 3;

	if (animation_index >= animation_length) {
		animation = screen::animation_control::NONE;
		animation_index = 0;
	}
}

void screen::draw_back_animation() {
	unsigned int animation_length = 44;
	
	display.clearDisplay();
	display.drawBitmap(28, 0, animations::get_back_animation(animation_index), 57, 32, SSD1306_WHITE);
	display.display();

	animation_index++;

	if (animation_index >= animation_length) {
		animation = screen::animation_control::NONE;
		animation_index = 0;
		display.clearDisplay();
	}
}

void screen::draw_pause_animation() {
	unsigned int animation_length = 239;

	unsigned int animation_loop_start = 100;

	if (animation_index >= animation_length) {
		animation_index = animation_loop_start;
	}

	display.clearDisplay();
	display.drawBitmap(28, 0, animations::get_pause_animation(animation_index), 57, 32, SSD1306_WHITE);
	display.display();

	animation_index++;
}

screen::animation_control screen::get_animation_state() {
	return animation;
}
