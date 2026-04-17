#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ----------- OPTIONAL FONTS -----------
#include <Fonts/FreeSerifItalic9pt7b.h>   // cursive-like
#include <Fonts/FreeSansBold12pt7b.h>     // bold
#include <Fonts/FreeMono9pt7b.h>          // monospace

// ----------- I2C PINS (ESP8266) -----------
#define SDA_PIN D7
#define SCL_PIN D6

// ----------- DISPLAY CONFIG -----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define FRAME_TIME 1   // ~25 FPS (stable + smooth)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ----------- PARTICLE CONFIG -----------
#define MAX_PARTICLES 12

struct Particle {
  int x, y;
  int speed;
};

Particle particles[MAX_PARTICLES];

// ----------- LYRICS STRUCT -----------
struct LyricLine {
  uint32_t time;
  const char* text;
};

LyricLine lyrics[] = {
  {16830, "See, right now, I need you, I'll meet you somewhere now"},
  {21130, "You up now, I see you, I get you, take care now"},
  {25500, "Slow down, be cool, I miss you, come here now"},
  {29540, "It's yours now, keep it, I'll hold out until now"},
  {33690, "I need you right now, once I leave you, I'm strung out"},
  {37690, "If I get you, I'm slowly breaking down"},

  {41910, "And, oh, it's hard to see you, but I wish you were right here"},
  {46320, "Oh, it's hard to leave you when I get you everywhere"},
  {50500, "All this time, I'm thinking we could never be a pair"},
  {54720, "Oh, no, I don't need you, but I miss you, come here"},

  {58700, "And, oh, it's hard to see you, but I wish you were right here"},
  {63200, "Oh, it's hard to leave you when I get you everywhere"},
  {67360, "All this time, I'm thinking, I'm strong enough to sink it"},
  {71600, "Oh, no, I don't need you, but I miss you, come here"},

  {75850, "He love me not, he loves me"},
  {77890, "He holds me tight, then lets me go"},
  {80050, "He love me not, he loves me"},
  {82130, "He holds me tight, then lets me go"},

  {84660, "Soon as you leave me, we always lose connection"},
  {89010, "It's gettin' messy, I fiend for your affection"},
  {93700, "Don't loosen your grip, got a hold on me"},
  {97180, "Now, forever, let's get back together"},

  {102110, "Lord, take it so far away"},
  {106350, "I pray that, God, we don't break"},
  {110610, "I want you to take me up and down"},
  {114880, "And 'round and 'round again"},

  {117600, "And, oh, it's hard to see you, but I wish you were right here"},
  {122130, "Oh, it's hard to leave you when I get you everywhere"},
  {126300, "All this time, I'm thinking we could never be a pair"},
  {130510, "Oh, no, I don't need you, but I miss you, come here"},

  {134480, "And, oh, it's hard to see you, but I wish you were right here"},
  {139000, "Oh, it's hard to leave you when I get you everywhere"},
  {143140, "All this time, I'm thinking, I'm strong enough to sink it"},
  {147440, "Oh, no, I don't need you, but I miss you, come here"},

  {151620, "He love me not, he loves me"},
  {153690, "He holds me tight, then lets me go"},
  {155820, "He love me not, he loves me"},
  {157950, "He holds me tight, then lets me go"},
  {160040, "He love me not, he loves me"},
  {162160, "He holds me tight, then lets me go"},
  {164240, "He love me not, he loves me"},
  {166370, "He holds me tight, then lets me go"},

  {168890, "You're gonna say that you're sorry at the end of the night"},
  {173740, "Wake up in the morning, everything's alright"},
  {177710, "At the end of the story, you're holdin' me tight"},
  {182200, "I don't need to worry, am I out of my mind?"},

  {185000, "And, oh, it's hard to see you, but I wish you were right here"},
  {188430, "I'm losing my mind"},
  {189500, "Oh, it's hard to leave you when I get you everywhere"},
  {193700, "All this time I'm thinking, I'm strong enough to sink it"},
  {197900, "Oh, no, I don't need you, but I miss you, come here"}
};

int totalLines = sizeof(lyrics) / sizeof(lyrics[0]);

// ----------- STYLE CONFIG -----------
struct Style {
  int align;      // 0=center, 1=left, 2=right
  bool uppercase; // convert text to uppercase
  int fontType;   // font selection
  int mode;       // animation mode
};

// ----------- RANDOM STYLE PICKER -----------
Style pickStyle() {
  Style s;
  s.align = random(3);
  s.uppercase = random(2);
  s.fontType = random(4);
  s.mode = random(4);
  return s;
}

// ----------- APPLY FONT -----------
void applyFont(int type) {
  switch (type) {
    case 0: display.setFont(); break;
    case 1: display.setFont(&FreeSerifItalic9pt7b); break;
    case 2: display.setFont(&FreeSansBold12pt7b); break;
    case 3: display.setFont(&FreeMono9pt7b); break;
  }
}

// ----------- SAFE TEXT DRAW -----------
void drawText(String txt, Style s, bool negative) {

  applyFont(s.fontType);

  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);

  // normalize bounding box (IMPORTANT FIX)
  int textX = -x1;
  int textY = -y1;

  int x;

  if (s.align == 0) x = (SCREEN_WIDTH - w) / 2;
  else if (s.align == 1) x = 0;
  else x = SCREEN_WIDTH - w;

  // clamp to prevent wrapping
  if (x < 0) x = 0;
  if (x > SCREEN_WIDTH - w) x = SCREEN_WIDTH - w;

  int y = (SCREEN_HEIGHT - h) / 2;
  if (y < 0) y = 0;

  // TRUE NEGATIVE MODE (safe)
  if (negative) {
    display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    display.setTextColor(BLACK);
  } else {
    display.setTextColor(WHITE);
  }

  display.setCursor(x + textX, y + textY);
  display.print(txt);
}

// ----------- SPLIT WORDS -----------
int splitWords(String text, String out[], int maxParts) {

  int count = 0;
  String word = "";

  for (size_t i = 0; i < text.length(); i++) {
    if (text[i] == ' ') {
      if (count < maxParts) out[count++] = word;
      word = "";
    } else {
      word += text[i];
    }
  }

  if (word.length() && count < maxParts)
    out[count++] = word;

  return count;
}

// ----------- PARTICLE EFFECTS -----------

// sparkles (random twinkle)
void drawSparkles() {
  for (int i = 0; i < 8; i++) {
    display.drawPixel(random(SCREEN_WIDTH), random(SCREEN_HEIGHT), WHITE);
  }
}

// burst effect
void drawBurst() {
  int cx = random(40, 90);
  int cy = random(20, 50);

  for (int i = 0; i < 6; i++) {
    display.drawPixel(cx + random(-5, 5), cy + random(-5, 5), WHITE);
  }
}

// falling particles
void updateParticles() {
  for (int i = 0; i < MAX_PARTICLES; i++) {

    particles[i].y += particles[i].speed;

    if (particles[i].y > SCREEN_HEIGHT) {
      particles[i].y = 0;
      particles[i].x = random(SCREEN_WIDTH);
    }

    display.drawPixel(particles[i].x, particles[i].y, WHITE);
  }
}

// ----------- FRAME RENDER -----------
// Draws ONE frame (text + particles + effects)
void renderFrame(String txt, Style s) {

  display.clearDisplay();

  bool negative = (s.mode == 1);

  // simple glitch line
  if (s.mode == 2) {
    display.drawFastHLine(random(128), random(64), random(20, 60), WHITE);
  }

  // particle overlay
  updateParticles();
  drawSparkles();

  if (random(5) == 0) drawBurst();

  drawText(txt, s, negative);

  display.display();
}

// ----------- TIMED DISPLAY -----------
// Shows ONE word within exact time window
void showWordTimed(String txt, Style s, uint32_t end) {

  while (millis() < end) {

    uint32_t frameStart = millis();

    renderFrame(txt, s);

    uint32_t elapsed = millis() - frameStart;

    // maintain stable FPS
    if (elapsed < FRAME_TIME) {
      delay(FRAME_TIME - elapsed);
    }
  }
}

// ----------- PLAY LINE -----------
// Splits sentence → words → shows each in time slice
void playLine(String text, uint32_t start, uint32_t end) {

  Style s = pickStyle();

  if (s.uppercase) text.toUpperCase();

  String words[20];
  int count = splitWords(text, words, 20);

  uint32_t total = end - start;
  uint32_t per = total / count;

  for (int i = 0; i < count; i++) {

    uint32_t wEnd = start + (i + 1) * per;

    showWordTimed(words[i], s, wEnd);
  }
}

// ----------- SETUP -----------
void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);

  randomSeed(micros());

  // initialize particles
  for (int i = 0; i < MAX_PARTICLES; i++) {
    particles[i].x = random(SCREEN_WIDTH);
    particles[i].y = random(SCREEN_HEIGHT);
    particles[i].speed = random(1, 3);
  }
}

// ----------- LOOP -----------
void loop() {

  uint32_t baseTime = millis();

  for (int i = 0; i < totalLines - 1; i++) {

    // wait until timestamp
    while (millis() - baseTime < lyrics[i].time) {
      delay(1);
    }

    // play line within time window
    playLine(
      lyrics[i].text,
      baseTime + lyrics[i].time,
      baseTime + lyrics[i + 1].time
    );
  }

  while (1); // stop after one run

}