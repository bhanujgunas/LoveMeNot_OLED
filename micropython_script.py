# ╔══════════════════════════════════════════╗
# ║   "LOVE ME NOT" — OLED Lyric Animation  ║
# ║   MicroPython · ESP8266 + SSD1306 128x64 ║
# ║   SCL → D3 (GPIO 0)  SDA → D2 (GPIO 4)  ║
# ╚══════════════════════════════════════════╝
#
# Flash MicroPython firmware first, then upload this as main.py
#
# Required library — upload to ESP8266 alongside this file:
#   ssd1306.py  →  https://github.com/micropython/micropython-lib/blob/master/micropython/drivers/display/ssd1306/ssd1306.py
#
# Quick install via mpremote / Thonny / ampy:
#   mpremote mip install ssd1306
#
# ── Timing ────────────────────────────────────────────────
# One beat in ms — change to match your song BPM
#   120 BPM → 500  |  100 BPM → 600  |  140 BPM → 428
BEAT_MS = 500

from machine import Pin, I2C, ADC
import ssd1306
import utime
import urandom

# ── Pin & display setup ───────────────────────────────────
# D2 = GPIO4 (SDA),  D3 = GPIO0 (SCL)
i2c = I2C(scl=Pin(0), sda=Pin(4), freq=400000)
oled = ssd1306.SSD1306_I2C(128, 64, i2c, addr=0x3C)

W = 128
H = 64

# ── Lyric list ────────────────────────────────────────────
# (line1, line2, beats, style)
# style: 0=glitch  1=typewriter  2=heartbeat  3=shake
#        4=scanline  5=split  6=strobe  7=matrix
LYRICS = [
    ("LOVE",        "ME NOT",      4, 2),
    ("DO YOU",      "LOVE ME?",    3, 0),
    ("OR NOT?",     "",            2, 6),
    ("I CAN'T",     "TELL...",     4, 1),
    ("YOUR EYES",   "SAY YES",     4, 5),
    ("YOUR LIPS",   "SAY NO",      4, 3),
    ("LOVE",        "ME",          2, 2),
    ("NOT",         "",            2, 6),
    ("TEARING",     "ME APART",    4, 4),
    ("LOVE ME",     "LOVE ME",     3, 7),
    ("NOT",         "",            2, 6),
    ("JUST",        "DECIDE!",     4, 3),
    ("LOVE",        "<3",          3, 2),
    ("ME",          "NOT",         4, 5),
    ("LOVE ME NOT", "LOVE ME NOT", 5, 7),
    ("I STILL",     "LOVE YOU",    5, 1),
    ("",            "...",         3, 4),
    ("LOVE",        "ME NOT",      6, 2),
]

# ── Helpers ───────────────────────────────────────────────
def cls():
    oled.fill(0)

def show():
    oled.show()

def rnd(n):
    return urandom.randint(0, n - 1) if n > 1 else 0

def sleep(ms):
    utime.sleep_ms(ms)

def ticks():
    return utime.ticks_ms()

def elapsed(t):
    return utime.ticks_diff(ticks(), t)

def auto_size(txt):
    n = len(txt)
    if n <= 4: return 2   # MicroPython ssd1306 supports scale 1-4
    if n <= 7: return 1
    return 1

# MicroPython's ssd1306 uses text(str, x, y) with 8×8 pixel chars.
# Scale 2 = 16×16 per char, scale 1 = 8×8 per char.
def char_w(scale): return 8 * scale
def char_h(scale): return 8 * scale

def centered_text(txt, y, scale=1):
    """Draw text centred horizontally at row y."""
    w = len(txt) * char_w(scale)
    x = max(0, (W - w) // 2)
    # ssd1306.text doesn't support scaling natively — use framebuf trick
    if scale == 1:
        oled.text(txt, x, y)
    else:
        # Draw scaled by blitting a tiny framebuf
        import framebuf
        tmp = framebuf.FrameBuffer(
            bytearray(len(txt) * 8 * 8 // 8 + 8), len(txt) * 8, 8,
            framebuf.MONO_HLSB)
        tmp.fill(0)
        tmp.text(txt, 0, 0, 1)
        # Scale up manually — each pixel becomes scale×scale block
        for cy in range(8):
            for cx in range(len(txt) * 8):
                if tmp.pixel(cx, cy):
                    oled.fill_rect(x + cx * scale, y + cy * scale,
                                   scale, scale, 1)

def draw_lyrics(l1, l2, ox=0, oy=0):
    has_two = len(l2) > 0
    if has_two:
        s1 = auto_size(l1)
        s2 = auto_size(l2)
        centered_text(l1, oy + 8,  s1)
        centered_text(l2, oy + 36, s2)
    else:
        s1 = auto_size(l1)
        centered_text(l1, oy + 20, s1)

# ox shifting version (glitch/shake needs x offset)
def draw_lyrics_ox(l1, l2, ox=0, oy=0):
    """Draw lyrics with a horizontal pixel offset (for slide/shake)."""
    has_two = len(l2) > 0
    if has_two:
        s1 = auto_size(l1)
        s2 = auto_size(l2)
        _centered_text_ox(l1, oy + 8,  s1, ox)
        _centered_text_ox(l2, oy + 36, s2, ox)
    else:
        s1 = auto_size(l1)
        _centered_text_ox(l1, oy + 20, s1, ox)

def _centered_text_ox(txt, y, scale, ox):
    w = len(txt) * char_w(scale)
    x = max(-W, min(W, (W - w) // 2 + ox))
    if scale == 1:
        oled.text(txt, x, y)
    else:
        import framebuf
        tmp = framebuf.FrameBuffer(
            bytearray(len(txt) * 8 * 8 // 8 + 8), len(txt) * 8, 8,
            framebuf.MONO_HLSB)
        tmp.fill(0)
        tmp.text(txt, 0, 0, 1)
        for cy in range(8):
            for cx in range(len(txt) * 8):
                if tmp.pixel(cx, cy):
                    nx = x + cx * scale
                    ny = y + cy * scale
                    if 0 <= nx < W and 0 <= ny < H:
                        oled.fill_rect(nx, ny, scale, scale, 1)

# ── Invert helper (SSD1306 command byte) ─────────────────
def invert(on):
    oled.write_cmd(0xA7 if on else 0xA6)

# ─────────────────────────────────────────────────────────
#  ANIMATION FUNCTIONS
# ─────────────────────────────────────────────────────────

# 0 — GLITCH SLIDE IN from right
def anim_glitch(l1, l2, hold_ms):
    x = W
    while x >= 0:
        cls()
        for _ in range(6):
            gy = rnd(H); gx = rnd(W); gw = rnd(40) + 5
            oled.hline(gx, gy, min(gw, W - gx), 1)
        draw_lyrics_ox(l1, l2, x, 0)
        show()
        sleep(30)
        x -= 12

    t = ticks()
    while elapsed(t) < hold_ms:
        cls()
        for _ in range(4):
            gy = rnd(H)
            oled.hline(0, gy, rnd(W) + 10, 1)
        draw_lyrics_ox(l1, l2, rnd(6) - 3, 0)
        show()
        sleep(80)
        cls()
        draw_lyrics(l1, l2)
        show()
        sleep(120)

# 1 — TYPEWRITER
def anim_typewriter(l1, l2, hold_ms):
    has_two = len(l2) > 0
    s1 = auto_size(l1)
    s2 = auto_size(l2) if has_two else 1

    for i in range(len(l1) + 1):
        buf = l1[:i]
        cls()
        if has_two:
            centered_text(buf, 8, s1)
            cw = len(buf) * char_w(s1)
            cx = (W - len(l1) * char_w(s1)) // 2 + cw
            oled.fill_rect(max(0, cx), 8, 3, char_h(s1), 1)
        else:
            centered_text(buf, 20, s1)
            cw = len(buf) * char_w(s1)
            cx = (W - len(l1) * char_w(s1)) // 2 + cw
            oled.fill_rect(max(0, cx), 20, 3, char_h(s1), 1)
        show()
        sleep(80)

    if has_two:
        for i in range(len(l2) + 1):
            buf = l2[:i]
            cls()
            centered_text(l1, 8, s1)
            centered_text(buf, 36, s2)
            cw = len(buf) * char_w(s2)
            cx = (W - len(l2) * char_w(s2)) // 2 + cw
            oled.fill_rect(max(0, cx), 36, 3, char_h(s2), 1)
            show()
            sleep(80)

    sleep(hold_ms)

# 2 — HEARTBEAT PULSE
def anim_heartbeat(l1, l2, hold_ms):
    cls(); draw_lyrics(l1, l2); show(); sleep(150)
    t = ticks()
    while elapsed(t) < hold_ms:
        invert(True);  sleep(60)
        invert(False); sleep(80)
        invert(True);  sleep(40)
        invert(False); sleep(40)
        rest = BEAT_MS * 2 - 220
        sleep(max(0, rest))
    invert(False)

# 3 — SHAKE / TREMBLE
def anim_shake(l1, l2, hold_ms):
    x = W
    while x >= 0:
        cls(); draw_lyrics_ox(l1, l2, x, 0); show(); sleep(20)
        x -= 20
    offsets = [-4, 4, -3, 3, -2, 2, -1, 1, 0]
    t = ticks()
    while elapsed(t) < hold_ms:
        for i in range(9):
            cls()
            draw_lyrics_ox(l1, l2, offsets[i], offsets[(i + 3) % 9])
            show()
            sleep(35)

# 4 — SCANLINE WIPE
def anim_scanline(l1, l2, hold_ms):
    scan_y = 0
    while scan_y < H:
        cls()
        draw_lyrics(l1, l2)
        oled.fill_rect(0, scan_y, W, H - scan_y, 0)
        oled.hline(0, scan_y, W, 1)
        show()
        sleep(12)
        scan_y += 2
    cls(); draw_lyrics(l1, l2); show()
    sleep(hold_ms)

# 5 — SPLIT REVEAL
def anim_split(l1, l2, hold_ms):
    step = W
    while step >= 0:
        cls()
        draw_lyrics(l1, l2)
        # mask top-right
        if W - step > 0:
            oled.fill_rect(step, 0, W - step, H // 2, 0)
        # mask bottom-left
        if step > 0:
            oled.fill_rect(0, H // 2, step, H // 2, 0)
        show()
        sleep(20)
        step -= 8
    cls(); draw_lyrics(l1, l2); show()
    sleep(hold_ms)

# 6 — STROBE FLASH
def anim_strobe(l1, l2, hold_ms):
    t = ticks()
    on = True
    interval = 60
    while elapsed(t) < hold_ms:
        cls()
        if on:
            draw_lyrics(l1, l2)
        show()
        on = not on
        sleep(interval)
        mid = hold_ms // 2
        diff = abs(elapsed(t) - mid)
        interval = 40 + diff // 8
    cls(); draw_lyrics(l1, l2); show()

# 7 — MATRIX RAIN then reveal
def anim_matrix(l1, l2, hold_ms):
    charset = "01LOVEMENOT!?*#@"
    cols = W // 8
    drops = [rnd(H) for _ in range(cols)]
    rain_time = min(1200, hold_ms // 2)

    t = ticks()
    while elapsed(t) < rain_time:
        # fade rows randomly
        for fy in range(0, H, 8):
            if rnd(3) == 0:
                oled.fill_rect(0, fy, W, 8, 0)
        for col in range(cols):
            ch = charset[rnd(len(charset))]
            oled.text(ch, col * 8, drops[col], 1)
            drops[col] = (drops[col] + 8) % H
        show()
        sleep(60)

    # Flash reveal
    invert(True);  sleep(80)
    invert(False)
    cls(); draw_lyrics(l1, l2); show()
    sleep(hold_ms - rain_time)

# ─────────────────────────────────────────────────────────
#  INTRO
# ─────────────────────────────────────────────────────────
def play_intro():
    heart = [
        " ** ** ",
        "*     *",
        " *   * ",
        "  * *  ",
        "   *   ",
    ]
    for frame in range(20):
        cls()
        for r, row in enumerate(heart):
            oled.text(row, (W - len(row) * 8) // 2,
                      4 + r * 8 + (frame % 2), 1)
        oled.text("LOVE ME NOT", (W - 11 * 8) // 2, 52, 1)
        show()
        sleep(80)
    invert(True);  sleep(120)
    invert(False); sleep(80)
    invert(True);  sleep(80)
    invert(False); sleep(400)
    cls(); show()

# ─────────────────────────────────────────────────────────
#  OUTRO — expanding circles
# ─────────────────────────────────────────────────────────
def play_outro():
    for radius in range(0, 50, 1):
        cls()
        r = 0
        while r < radius:
            # Draw circle using midpoint algorithm
            cx, cy = W // 2, H // 2
            x, y = r, 0
            err = 0
            while x >= y:
                for dx, dy in [(x,y),(-x,y),(x,-y),(-x,-y),(y,x),(-y,x),(y,-x),(-y,-x)]:
                    px, py = cx + dx, cy + dy
                    if 0 <= px < W and 0 <= py < H:
                        oled.pixel(px, py, 1)
                y += 1
                err += 1 + 2 * y
                if 2 * (err - x) + 1 > 0:
                    x -= 1
                    err += 1 - 2 * x
            r += 4
        oled.text("LOVE ME NOT", (W - 11 * 8) // 2, H // 2 - 4, 1)
        show()
        sleep(40)
    sleep(800)

# ─────────────────────────────────────────────────────────
#  DISPATCH TABLE
# ─────────────────────────────────────────────────────────
ANIMS = [
    anim_glitch,
    anim_typewriter,
    anim_heartbeat,
    anim_shake,
    anim_scanline,
    anim_split,
    anim_strobe,
    anim_matrix,
]

# ─────────────────────────────────────────────────────────
#  MAIN
# ─────────────────────────────────────────────────────────
play_intro()

while True:
    for (l1, l2, beats, style) in LYRICS:
        hold = beats * BEAT_MS
        ANIMS[style](l1, l2, hold)
        cls(); show()
        sleep(80)

    play_outro()
    sleep(1000)
