# Stream32 Player

Stream32 Player is an experimental MP3 player for ESP32 with a TFT display, SD card storage, and Bluetooth A2DP audio output.

**Current version: `0.0.10-beta`**

This is a public beta release. The main features are working, but the firmware may still change and should be tested with your specific board and Bluetooth device.

## Features

- MP3 playback from an SD card;
- Bluetooth A2DP Source with saved device selection;
- automatic track-to-track playback;
- track library and favorites;
- portrait interface built with TFT_eSPI;
- Classic Green, Blue, Purple, Amber, and Rose themes;
- screen brightness and inactivity lock timer;
- auto-play after Bluetooth connection;
- persistent settings stored in ESP32 Preferences.

## Requirements

- ESP32-compatible board with a TFT display and XPT2046 touch controller;
- microSD card;
- Arduino IDE with the ESP32 board package 3.3.x;
- `TFT_eSPI`, `XPT2046_Touchscreen`, `ESP32-A2DP`, and `minimp3` libraries.

## SD card layout

Create a `/tracks` folder and place MP3 files inside it:

```text
/tracks/
  Linkin Park - Numb.mp3
  Daft Punk - Get Lucky.mp3
```

The `Artist - Title.mp3` naming format is recommended for the best display in the player.

## Installation

1. Configure `TFT_eSPI` for your board and display.
2. Install the required libraries through the Arduino Library Manager or manually.
3. Open `Stream32-ESP-Player.ino` in Arduino IDE.
4. Select your ESP32 board and upload the sketch.
5. Insert the SD card and select a Bluetooth device through **Settings → Bluetooth**.

## User interface

- **Player** opens the current track and playback controls.
- **Tracks** shows the sorted MP3 library.
- **Likes** shows favorite tracks.
- **Settings** contains Bluetooth, screen, playback, theme, information, and reset options.
- The screen timeout can be set to 1, 5, or 10 minutes, or disabled.

## Important notes

The project is designed for a 240×320 portrait display and the author's current board pinout. Other boards may require changes to the SD, touch, and backlight pins.

The Wi-Fi upload page is disabled in this beta release.

## License

This project is released under the MIT License. See [LICENSE](LICENSE).
