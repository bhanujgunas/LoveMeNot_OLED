# LoveMeNot OLED Lyrical Animation Display

An ESP32 sketch that renders lyrics and visual effects on a 128x64 SSD1306 OLED display but only specific to "Love me not" Song by Ravyn Lenae.
Still you can replace the lyrics with the timestamp format to use it for other songs. This is a native animate generation and not like the reference i used since it is directly converting the video to the image frame, idk how the video was created in the first place so i make this code to produce the animations itself. 😅🥲

## Required Components

- ESP32 development board
- SSD1306 OLED display (128x64)
- I2C connection: SDA to D7, SCL to D6

## Wiring

<b> ESP32  ->  OLED </b>
- 3.3V  -> VCC
- GND   -> GND
- D7    -> SDA
- D6    -> SCL

## Software Requirements

- Arduino IDE or PlatformIO
- ESP32 Arduino core
- Adafruit SSD1306 library
- Adafruit GFX library
- Wire

## Usage

1. Upload the main sketch (`LoveMeNot_OLED.ino`) to the ESP32.
2. Upload the `data` folder via `Tools` → `ESP32 Sketch Data Upload`.

## Credits

- Inspired by [ESP32_Video_Display](https://github.com/bhanujgunas/ESP32_Video_Display)
- ChatGPT, Claude AND Copilot
