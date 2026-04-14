/*
 * ╔══════════════════════════════════════════════╗
 * ║   "LOVE ME NOT" — OLED Lyric Animation      ║
 * ║   ESP32 / ESP32-S3 + SSD1306 128×64         ║
 * ║   Tested on: Waveshare ESP32-S3 Zero         ║
 * ║                                              ║
 * ║   OLED wiring:                               ║
 * ║     VCC  →  3.3V                             ║
 * ║     GND  →  GND                              ║
 * ║     SCL  →  GPIO17                           ║
 * ║     SDA  →  GPIO18                           ║
 * ╚══════════════════════════════════════════════╝
 *
 * Libraries (Arduino Library Manager):
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 *
 * Arduino IDE board selection:
 *   ESP32-S3 Zero  →  "ESP32S3 Dev Module"
 *   Tools → USB CDC On Boot: Enabled   (for Serial.print)
 *   Tools → Flash Size: 4MB
 *
 * Generic ESP32  →  "ESP32 Dev Module"
 */

#include <Wire.h>d:\Downloads\LoveMeNot_OLED\LoveMeNot_OLED.ino
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── I2C pins ─────────────────────────────────────────────
// Change these to whichever GPIO pair you wired to SCL/SDA
#define PIN_SDA  8   // → OLED SDA
#define PIN_SCL  9   // → OLED SCL

// ── Display config ────────────────────────────────────────
#define SCREEN_W  128
#define SCREEN_H   64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ── Timing ────────────────────────────────────────────────
// One beat in ms — tune to match your song's BPM
//   120 BPM → 500  |  100 BPM → 600  |  140 BPM → 428
#define BEAT_MS  500

// ─────────────────────────────────────────────────────────
//  LYRIC TABLE
//  { line1, line2, beats, style }
//  style:
//   0 = GLITCH SLIDE IN from right
//   1 = TYPEWRITER  (letter by letter)
//   2 = HEARTBEAT PULSE (invert flash)
//   3 = SHAKE / TREMBLE
//   4 = SCANLINE WIPE  (top → bottom)
//   5 = SPLIT REVEAL   (top from left / bottom from right)
//   6 = STROBE FLASH
//   7 = MATRIX RAIN then reveal
// ─────────────────────────────────────────────────────────
struct Lyric {
  const char* line1;
  const char* line2;
  uint8_t     beats;
  uint8_t     style;
};

const Lyric lyrics[] = {
  { "LOVE",        "ME NOT",      4, 2 },
  { "DO YOU",      "LOVE ME?",    3, 0 },
  { "OR NOT?",     "",            2, 6 },
  { "I CAN'T",     "TELL...",     4, 1 },
  { "YOUR EYES",   "SAY YES",     4, 5 },
  { "YOUR LIPS",   "SAY NO",      4, 3 },
  { "LOVE",        "ME",          2, 2 },
  { "NOT",         "",            2, 6 },
  { "TEARING",     "ME APART",    4, 4 },
  { "LOVE ME",     "LOVE ME",     3, 7 },
  { "NOT",         "",            2, 6 },
  { "JUST",        "DECIDE!",     4, 3 },
  { "LOVE",        "<3",          3, 2 },
  { "ME",          "NOT",         4, 5 },
  { "LOVE ME NOT", "LOVE ME NOT", 5, 7 },
  { "I STILL",     "LOVE YOU",    5, 1 },
  { "",            "...",         3, 4 },
  { "LOVE",        "ME NOT",      6, 2 },
};
const uint8_t LYRIC_COUNT = sizeof(lyrics) / sizeof(lyrics[0]);

// ── Helpers ───────────────────────────────────────────────
inline void cls()  { display.clearDisplay(); }
inline void show() { display.display(); }

uint8_t autoSize(const char* txt) {
  uint8_t n = strlen(txt);
  if (n <= 4) return 3;
  if (n <= 7) return 2;
  return 1;
}

void centeredText(const char* txt, int y, uint8_t sz) {
  display.setTextSize(sz);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(txt, 0, y, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_W - (int)w) / 2, y);
  display.print(txt);
}

void drawLyrics(const char* l1, const char* l2, int ox = 0, int oy = 0) {
  display.setTextColor(SSD1306_WHITE);
  bool two = strlen(l2) > 0;
  if (two) {
    display.setTextSize(autoSize(l1));
    int16_t x1,y1; uint16_t w,h;
    display.getTextBounds(l1, 0, oy+8, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_W-(int)w)/2 + ox, oy+8);
    display.print(l1);
    display.setTextSize(autoSize(l2));
    display.getTextBounds(l2, 0, oy+36, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_W-(int)w)/2 + ox, oy+36);
    display.print(l2);
  } else {
    display.setTextSize(autoSize(l1));
    int16_t x1,y1; uint16_t w,h;
    display.getTextBounds(l1, 0, oy+20, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_W-(int)w)/2 + ox, oy+20);
    display.print(l1);
  }
}

// ─────────────────────────────────────────────────────────
//  ANIMATION FUNCTIONS
// ─────────────────────────────────────────────────────────

// 0 — GLITCH SLIDE IN
void animGlitch(const char* l1, const char* l2, uint32_t holdMs) {
  for (int x = SCREEN_W; x >= 0; x -= 12) {
    cls();
    for (int i = 0; i < 6; i++) {
      display.drawFastHLine(random(SCREEN_W), random(SCREEN_H), random(40)+4, SSD1306_WHITE);
    }
    drawLyrics(l1, l2, x, 0);
    show(); delay(28);
  }
  uint32_t t = millis();
  while (millis()-t < holdMs) {
    cls();
    for (int i = 0; i < 4; i++)
      display.drawFastHLine(0, random(SCREEN_H), random(SCREEN_W)+8, SSD1306_WHITE);
    drawLyrics(l1, l2, random(7)-3, 0);
    show(); delay(80);
    cls(); drawLyrics(l1, l2); show(); delay(120);
  }
}

// 1 — TYPEWRITER
void animTypewriter(const char* l1, const char* l2, uint32_t holdMs) {
  char buf[32];
  uint8_t s1 = autoSize(l1);
  bool two = strlen(l2) > 0;
  uint8_t s2 = two ? autoSize(l2) : 1;
  int16_t x1,y1; uint16_t w,h;

  for (uint8_t i = 0; i <= strlen(l1); i++) {
    strncpy(buf, l1, i); buf[i] = '\0';
    cls(); display.setTextColor(SSD1306_WHITE);
    display.setTextSize(s1);
    display.getTextBounds(l1, 0, two?8:20, &x1, &y1, &w, &h);
    int cx = (SCREEN_W-(int)w)/2;
    display.setCursor(cx, two?8:20); display.print(buf);
    display.setTextSize(s1);
    display.getTextBounds(buf[0]?buf:"M", 0, 0, &x1, &y1, &w, &h);  // char width
    int curX = cx + i*(int)w/max((uint8_t)1,(uint8_t)strlen(l1));
    // simple cursor block
    display.fillRect(cx + i*6*s1, two?8:20, 3, 8*s1, SSD1306_WHITE);
    show(); delay(80);
  }
  if (two) {
    for (uint8_t i = 0; i <= strlen(l2); i++) {
      strncpy(buf, l2, i); buf[i] = '\0';
      cls(); display.setTextColor(SSD1306_WHITE);
      display.setTextSize(s1);
      int16_t tx,ty; uint16_t tw,th;
      display.getTextBounds(l1, 0, 8, &tx, &ty, &tw, &th);
      display.setCursor((SCREEN_W-(int)tw)/2, 8); display.print(l1);
      display.setTextSize(s2);
      display.getTextBounds(l2, 0, 36, &tx, &ty, &tw, &th);
      int cx2 = (SCREEN_W-(int)tw)/2;
      display.setCursor(cx2, 36); display.print(buf);
      display.fillRect(cx2 + i*6*s2, 36, 3, 8*s2, SSD1306_WHITE);
      show(); delay(80);
    }
  }
  delay(holdMs);
}

// 2 — HEARTBEAT PULSE
void animHeartbeat(const char* l1, const char* l2, uint32_t holdMs) {
  cls(); drawLyrics(l1, l2); show(); delay(150);
  uint32_t t = millis();
  while (millis()-t < holdMs) {
    display.invertDisplay(true);  delay(60);
    display.invertDisplay(false); delay(80);
    display.invertDisplay(true);  delay(40);
    display.invertDisplay(false); delay(40);
    uint32_t rest = BEAT_MS*2 - 220;
    if (millis()-t < holdMs) delay(rest);
  }
  display.invertDisplay(false);
}

// 3 — SHAKE
void animShake(const char* l1, const char* l2, uint32_t holdMs) {
  for (int x = SCREEN_W; x >= 0; x -= 20) {
    cls(); drawLyrics(l1, l2, x, 0); show(); delay(18);
  }
  const int8_t off[] = {-4,4,-3,3,-2,2,-1,1,0};
  uint32_t t = millis();
  while (millis()-t < holdMs) {
    for (uint8_t i = 0; i < 9; i++) {
      cls(); drawLyrics(l1, l2, off[i], off[(i+3)%9]); show(); delay(33);
    }
  }
}

// 4 — SCANLINE WIPE
void animScanline(const char* l1, const char* l2, uint32_t holdMs) {
  for (int sy = 0; sy < SCREEN_H; sy += 2) {
    cls(); drawLyrics(l1, l2);
    display.fillRect(0, sy, SCREEN_W, SCREEN_H-sy, SSD1306_BLACK);
    display.drawFastHLine(0, sy, SCREEN_W, SSD1306_WHITE);
    show(); delay(11);
  }
  cls(); drawLyrics(l1, l2); show();
  delay(holdMs);
}

// 5 — SPLIT REVEAL
void animSplit(const char* l1, const char* l2, uint32_t holdMs) {
  for (int s = SCREEN_W; s >= 0; s -= 8) {
    cls(); drawLyrics(l1, l2);
    if (SCREEN_W-s > 0) display.fillRect(s,       0,       SCREEN_W-s, SCREEN_H/2, SSD1306_BLACK);
    if (s > 0)          display.fillRect(0,        SCREEN_H/2, s,      SCREEN_H/2, SSD1306_BLACK);
    show(); delay(18);
  }
  cls(); drawLyrics(l1, l2); show();
  delay(holdMs);
}

// 6 — STROBE
void animStrobe(const char* l1, const char* l2, uint32_t holdMs) {
  uint32_t t = millis(); bool on = true; int iv = 60;
  while (millis()-t < holdMs) {
    cls();
    if (on) drawLyrics(l1, l2);
    show(); on = !on; delay(iv);
    int diff = abs((int)(millis()-t) - (int)(holdMs/2));
    iv = 40 + diff/8;
  }
  cls(); drawLyrics(l1, l2); show();
}

// 7 — MATRIX RAIN → reveal
void animMatrix(const char* l1, const char* l2, uint32_t holdMs) {
  const char* cs = "01LOVEMENOT!?*#@";
  const uint8_t COLS = SCREEN_W / 6;
  uint8_t drops[22];
  for (uint8_t i = 0; i < COLS; i++) drops[i] = random(SCREEN_H);

  uint32_t rainMs = min((uint32_t)1200, holdMs/2);
  uint32_t t = millis();
  while (millis()-t < rainMs) {
    for (int fy = 0; fy < SCREEN_H; fy += 8)
      if (random(3)==0) display.fillRect(0, fy, SCREEN_W, 8, SSD1306_BLACK);
    display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
    for (uint8_t c = 0; c < COLS; c++) {
      char ch[2] = { cs[random(strlen(cs))], '\0' };
      display.setCursor(c*6, drops[c]); display.print(ch);
      drops[c] = (drops[c]+8) % SCREEN_H;
    }
    show(); delay(55);
  }
  display.invertDisplay(true);  delay(80);
  display.invertDisplay(false);
  cls(); drawLyrics(l1, l2); show();
  delay(holdMs - rainMs);
}

// ─────────────────────────────────────────────────────────
//  INTRO
// ─────────────────────────────────────────────────────────
void playIntro() {
  const char* heart[] = {
    " ** ** ",
    "*     *",
    " *   * ",
    "  * *  ",
    "   *   ",
  };
  for (int f = 0; f < 22; f++) {
    cls();
    display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
    for (uint8_t r = 0; r < 5; r++) {
      display.setCursor((SCREEN_W - 42)/2, 4 + r*8 + (f%2));
      display.print(heart[r]);
    }
    centeredText("LOVE ME NOT", 50, 1);
    show(); delay(75);
  }
  display.invertDisplay(true);  delay(120);
  display.invertDisplay(false); delay(80);
  display.invertDisplay(true);  delay(80);
  display.invertDisplay(false); delay(400);
  cls(); show();
}

// ─────────────────────────────────────────────────────────
//  OUTRO — expanding ripple circles
// ─────────────────────────────────────────────────────────
void playOutro() {
  for (int r = 0; r < 50; r++) {
    cls();
    for (int i = 4; i <= r; i += 4)
      display.drawCircle(SCREEN_W/2, SCREEN_H/2, i, SSD1306_WHITE);
    centeredText("LOVE ME NOT", 28, 1);
    show(); delay(38);
  }
  delay(800);
}

// ─────────────────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // ESP32 allows custom I2C pins via Wire.begin(SDA, SCL)
  Wire.begin(PIN_SDA, PIN_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 not found! Check wiring."));
    for (;;) delay(1000);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  display.cp437(true);   // full CP437 char set

  randomSeed(esp_random()); // hardware RNG on ESP32
  delay(200);
  playIntro();
}

// ─────────────────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────────────────
void loop() {
  for (uint8_t i = 0; i < LYRIC_COUNT; i++) {
    uint32_t holdMs = (uint32_t)lyrics[i].beats * BEAT_MS;
    switch (lyrics[i].style) {
      case 0: animGlitch    (lyrics[i].line1, lyrics[i].line2, holdMs); break;
      case 1: animTypewriter(lyrics[i].line1, lyrics[i].line2, holdMs); break;
      case 2: animHeartbeat (lyrics[i].line1, lyrics[i].line2, holdMs); break;
      case 3: animShake     (lyrics[i].line1, lyrics[i].line2, holdMs); break;
      case 4: animScanline  (lyrics[i].line1, lyrics[i].line2, holdMs); break;
      case 5: animSplit     (lyrics[i].line1, lyrics[i].line2, holdMs); break;
      case 6: animStrobe    (lyrics[i].line1, lyrics[i].line2, holdMs); break;
      case 7: animMatrix    (lyrics[i].line1, lyrics[i].line2, holdMs); break;
    }
    cls(); show(); delay(75);
  }
  playOutro();
  delay(1000);
}
