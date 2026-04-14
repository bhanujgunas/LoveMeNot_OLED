/*
 * ╔══════════════════════════════════════════╗
 * ║   "LOVE ME NOT" — OLED Lyric Animation  ║
 * ║   ESP8266 + SSD1306 128x64               ║
 * ║   SCL → D3   SDA → D2                   ║
 * ╚══════════════════════════════════════════╝
 *
 * Libraries needed (install via Arduino Library Manager):
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 *
 * Each lyric segment has its own crazy animation style.
 * Adjust the BEAT_MS constant to match your song's BPM.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Pin config ────────────────────────────────────────────
#define SDA_PIN D2
#define SCL_PIN D1

// ── Display config ────────────────────────────────────────
#define SCREEN_W  128
#define SCREEN_H   64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ── Timing ────────────────────────────────────────────────
// One "beat" in ms — adjust to match your song BPM
// 120 BPM → 500ms   |   100 BPM → 600ms   |   140 BPM → 428ms
#define BEAT_MS  500

// ─────────────────────────────────────────────────────────
//  LYRIC SEGMENTS
//  Each entry: { "line1", "line2", beats_to_hold, anim_style }
//  anim_style:
//   0 = GLITCH SLIDE IN from right
//   1 = TYPEWRITER (letter by letter)
//   2 = HEARTBEAT PULSE (zoom-like invert flash)
//   3 = SHAKE / TREMBLE
//   4 = SCANLINE WIPE top→bottom
//   5 = SPLIT REVEAL (top half & bottom half come from edges)
//   6 = STROBE FLASH (blink in)
//   7 = MATRIX RAIN then reveal
// ─────────────────────────────────────────────────────────
struct Lyric {
  const char* line1;
  const char* line2;
  uint8_t     beats;
  uint8_t     style;
};

const Lyric lyrics[] = {
  { "LOVE",        "ME NOT",     4,  2 },  // heartbeat pulse
  { "DO YOU",      "LOVE ME?",   3,  0 },  // glitch slide
  { "OR NOT?",     "",           2,  6 },  // strobe
  { "I CAN'T",     "TELL...",    4,  1 },  // typewriter
  { "YOUR EYES",   "SAY YES",    4,  5 },  // split reveal
  { "YOUR LIPS",   "SAY NO",     4,  3 },  // shake
  { "LOVE",        "ME",         2,  2 },  // pulse
  { "NOT",         "",           2,  6 },  // strobe
  { "TEARING",     "ME APART",   4,  4 },  // scanline
  { "LOVE ME",     "LOVE ME",    3,  7 },  // matrix rain
  { "NOT",         "",           2,  6 },  // strobe
  { "JUST",        "DECIDE!",    4,  3 },  // shake
  { "LOVE",        "♥",          3,  2 },  // heartbeat
  { "ME",          "NOT",        4,  5 },  // split reveal
  { "LOVE ME NOT", "LOVE ME NOT",5,  7 },  // matrix rain
  { "I STILL",     "LOVE YOU",   5,  1 },  // typewriter
  { "",            "...",        3,  4 },  // scanline
  { "LOVE",        "ME NOT",     6,  2 },  // final pulse
};
const uint8_t LYRIC_COUNT = sizeof(lyrics) / sizeof(lyrics[0]);

// ── Helpers ───────────────────────────────────────────────
void cls() { display.clearDisplay(); }
void show() { display.display(); }

// Center text on a given Y baseline
void centeredText(const char* txt, int y, uint8_t sz = 1) {
  display.setTextSize(sz);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(txt, 0, y, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_W - w) / 2, y);
  display.print(txt);
}

// Pick text size so it fills the width nicely
uint8_t autoSize(const char* txt) {
  uint8_t len = strlen(txt);
  if (len <= 4)  return 3;
  if (len <= 7)  return 2;
  return 1;
}

// Draw the two lyric lines normally (for compound effects)
void drawLyrics(const char* l1, const char* l2, int offsetX = 0, int offsetY = 0) {
  display.setTextColor(SSD1306_WHITE);
  bool hasTwo = strlen(l2) > 0;
  if (hasTwo) {
    uint8_t s1 = autoSize(l1), s2 = autoSize(l2);
    centeredText(l1, offsetY + 8,  s1);
    centeredText(l2, offsetY + 36, s2);
  } else {
    uint8_t s1 = autoSize(l1);
    centeredText(l1, offsetY + 20, s1);
  }
}

// ── RNG helper ───────────────────────────────────────────
uint16_t rnd(uint16_t maxVal) { return random(maxVal); }

// ─────────────────────────────────────────────────────────
//  ANIMATION FUNCTIONS
// ─────────────────────────────────────────────────────────

// 0 — GLITCH SLIDE IN from right with noise artifacts
void animGlitchSlide(const char* l1, const char* l2, uint32_t holdMs) {
  for (int x = SCREEN_W; x >= 0; x -= 12) {
    cls();
    // glitch noise bars
    for (int i = 0; i < 6; i++) {
      int gy = rnd(SCREEN_H);
      int gx = rnd(SCREEN_W);
      display.drawFastHLine(gx, gy, rnd(40), SSD1306_WHITE);
    }
    drawLyrics(l1, l2, x, 0);
    show();
    delay(30);
  }
  // Glitch flashes while holding
  uint32_t t = millis();
  while (millis() - t < holdMs) {
    cls();
    // random horizontal glitch lines
    for (int i = 0; i < 4; i++) {
      int gy = rnd(SCREEN_H);
      display.drawFastHLine(0, gy, rnd(SCREEN_W), SSD1306_WHITE);
    }
    drawLyrics(l1, l2, rnd(6) - 3, 0);
    show();
    delay(80);
    cls();
    drawLyrics(l1, l2, 0, 0);
    show();
    delay(120);
  }
}

// 1 — TYPEWRITER letter by letter
void animTypewriter(const char* l1, const char* l2, uint32_t holdMs) {
  char buf[32];
  uint8_t s1 = autoSize(l1);
  bool hasTwo = strlen(l2) > 0;
  uint8_t s2 = hasTwo ? autoSize(l2) : 1;

  // Type line1
  for (uint8_t i = 0; i <= strlen(l1); i++) {
    strncpy(buf, l1, i); buf[i] = '\0';
    cls();
    display.setTextColor(SSD1306_WHITE);
    if (hasTwo) {
      centeredText(buf, 8,  s1);
    } else {
      centeredText(buf, 20, s1);
    }
    // cursor blink
    display.fillRect(
      (SCREEN_W / 2) + (i * 6 * s1) / 2 - (strlen(l1) * 6 * s1) / 2,
      hasTwo ? 8 : 20, 3, 8 * s1, SSD1306_WHITE);
    show();
    delay(80);
  }
  // Type line2
  if (hasTwo) {
    for (uint8_t i = 0; i <= strlen(l2); i++) {
      strncpy(buf, l2, i); buf[i] = '\0';
      cls();
      display.setTextColor(SSD1306_WHITE);
      centeredText(l1, 8,  s1);
      centeredText(buf, 36, s2);
      display.fillRect(
        (SCREEN_W / 2) + (i * 6 * s2) / 2 - (strlen(l2) * 6 * s2) / 2,
        36, 3, 8 * s2, SSD1306_WHITE);
      show();
      delay(80);
    }
  }
  delay(holdMs);
}

// 2 — HEARTBEAT PULSE (invert flash, scale bounce illusion)
void animHeartbeat(const char* l1, const char* l2, uint32_t holdMs) {
  // Show normally then INVERT flash × 3 like a heartbeat
  cls(); drawLyrics(l1, l2); show(); delay(150);

  uint32_t t = millis();
  int beat = 0;
  while (millis() - t < holdMs) {
    // THUMP 1
    display.invertDisplay(true);  delay(60);
    display.invertDisplay(false); delay(80);
    // THUMP 2
    display.invertDisplay(true);  delay(40);
    display.invertDisplay(false); delay(40);
    // rest
    delay(BEAT_MS * 2 - 220);
    beat++;
  }
  display.invertDisplay(false);
}

// 3 — SHAKE / TREMBLE
void animShake(const char* l1, const char* l2, uint32_t holdMs) {
  // Slide-in fast
  for (int x = SCREEN_W; x >= 0; x -= 20) {
    cls(); drawLyrics(l1, l2, x, 0); show(); delay(20);
  }
  int8_t offsets[] = {-4,4,-3,3,-2,2,-1,1,0};
  uint32_t t = millis();
  while (millis() - t < holdMs) {
    for (uint8_t i = 0; i < 9; i++) {
      cls(); drawLyrics(l1, l2, offsets[i], offsets[(i+3)%9]); show();
      delay(35);
    }
  }
}

// 4 — SCANLINE WIPE top → bottom
void animScanline(const char* l1, const char* l2, uint32_t holdMs) {
  // First draw full in a buffer effect: reveal one row at a time
  for (int scanY = 0; scanY < SCREEN_H; scanY += 2) {
    cls();
    drawLyrics(l1, l2);
    // black mask below scan line
    display.fillRect(0, scanY, SCREEN_W, SCREEN_H - scanY, SSD1306_BLACK);
    // scanline highlight
    display.drawFastHLine(0, scanY, SCREEN_W, SSD1306_WHITE);
    show();
    delay(12);
  }
  cls(); drawLyrics(l1, l2); show();
  delay(holdMs);
}

// 5 — SPLIT REVEAL (top half from left, bottom half from right)
void animSplitReveal(const char* l1, const char* l2, uint32_t holdMs) {
  for (int step = SCREEN_W; step >= 0; step -= 8) {
    cls();
    // Draw everything then clip with black rectangles
    drawLyrics(l1, l2);
    // top half mask
    display.fillRect(step, 0, SCREEN_W - step, SCREEN_H / 2, SSD1306_BLACK);
    // bottom half mask (mirror)
    display.fillRect(0, SCREEN_H / 2, step, SCREEN_H / 2, SSD1306_BLACK);
    show();
    delay(20);
  }
  cls(); drawLyrics(l1, l2); show();
  delay(holdMs);
}

// 6 — STROBE FLASH
void animStrobe(const char* l1, const char* l2, uint32_t holdMs) {
  uint32_t t = millis();
  bool on = true;
  int interval = 60;
  while (millis() - t < holdMs) {
    if (on) { cls(); drawLyrics(l1, l2); show(); }
    else    { cls(); show(); }
    on = !on;
    delay(interval);
    // speed up then slow down
    interval = 40 + abs((int)(millis() - t - holdMs/2)) / 8;
  }
  cls(); drawLyrics(l1, l2); show();
}

// 7 — MATRIX RAIN then text reveal
void animMatrixRain(const char* l1, const char* l2, uint32_t holdMs) {
  const char charset[] = "01LOVEMENOT!?♥*#@";
  const uint8_t COLS = SCREEN_W / 6;
  uint8_t drops[22];
  for (uint8_t i = 0; i < COLS; i++) drops[i] = rnd(SCREEN_H);

  uint32_t t = millis();
  uint32_t rainTime = min((uint32_t)1200, holdMs / 2);

  while (millis() - t < rainTime) {
    display.dim(false);
    // Fade: draw semi-transparent black (draw black rects over old chars)
    for (int fy = 0; fy < SCREEN_H; fy += 8) {
      if (rnd(3) == 0)
        display.fillRect(0, fy, SCREEN_W, 8, SSD1306_BLACK);
    }
    // Draw new chars
    for (uint8_t col = 0; col < COLS; col++) {
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      uint8_t ci = rnd(strlen(charset));
      char ch[2] = { charset[ci], '\0' };
      display.setCursor(col * 6, drops[col]);
      display.print(ch);
      drops[col] = (drops[col] + 8) % SCREEN_H;
    }
    show();
    delay(60);
  }

  // Reveal the actual lyrics with a flash
  display.invertDisplay(true); delay(80);
  display.invertDisplay(false);
  cls(); drawLyrics(l1, l2); show();
  delay(holdMs - rainTime);
}

// ─────────────────────────────────────────────────────────
//  INTRO ANIMATION — plays once on boot
// ─────────────────────────────────────────────────────────
void playIntro() {
  // Draw a giant heart with ASCII art + title
  const char* heart[] = {
    " ** ** ",
    "*     *",
    " *   * ",
    "  * *  ",
    "   *   ",
  };
  for (int frame = 0; frame < 20; frame++) {
    cls();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    for (uint8_t r = 0; r < 5; r++) {
      display.setCursor((SCREEN_W - 42) / 2, 4 + r * 8 + (frame % 2));
      display.print(heart[r]);
    }
    // Title below heart
    display.setTextSize(1);
    centeredText("LOVE ME NOT", 48, 1);
    show();
    delay(80);
  }
  // Flash reveal
  display.invertDisplay(true); delay(120);
  display.invertDisplay(false); delay(80);
  display.invertDisplay(true); delay(80);
  display.invertDisplay(false); delay(400);
  cls(); show();
}

// ─────────────────────────────────────────────────────────
//  OUTRO — loop ripple effect
// ─────────────────────────────────────────────────────────
void playOutro() {
  for (int r = 0; r < 50; r++) {
    cls();
    for (int i = 0; i < r; i += 4)
      display.drawCircle(SCREEN_W/2, SCREEN_H/2, i, SSD1306_WHITE);
    centeredText("LOVE ME NOT", 28, 1);
    show();
    delay(40);
  }
  delay(800);
}

// ─────────────────────────────────────────────────────────
//  SETUP & LOOP
// ─────────────────────────────────────────────────────────
void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);   // D2=SDA, D3=SCL
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 not found!");
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  randomSeed(analogRead(A0));
  delay(200);
  playIntro();
}

void loop() {
  for (uint8_t i = 0; i < LYRIC_COUNT; i++) {
    const Lyric& ly = lyrics[i];
    uint32_t holdMs = (uint32_t)ly.beats * BEAT_MS;

    switch (ly.style) {
      case 0: animGlitchSlide  (ly.line1, ly.line2, holdMs); break;
      case 1: animTypewriter   (ly.line1, ly.line2, holdMs); break;
      case 2: animHeartbeat    (ly.line1, ly.line2, holdMs); break;
      case 3: animShake        (ly.line1, ly.line2, holdMs); break;
      case 4: animScanline     (ly.line1, ly.line2, holdMs); break;
      case 5: animSplitReveal  (ly.line1, ly.line2, holdMs); break;
      case 6: animStrobe       (ly.line1, ly.line2, holdMs); break;
      case 7: animMatrixRain   (ly.line1, ly.line2, holdMs); break;
    }

    cls(); show();
    delay(80);  // tiny gap between lyrics
  }

  playOutro();
  delay(1000);
  // Then it loops from the beginning!
}
