"""Generate the block texture atlas (atlas.png) using only the stdlib.

Layout: 4x4 grid of 16px tiles. Tile index = row*4 + col, row 0 at the TOP of
the PNG (the mesher's UV math accounts for stb's vertical flip):

    0 grass_top   1 grass_side   2 dirt        3 stone
    4 sand        5 water        6 snow        7 log_side
    8 log_top     9 leaves      10 cactus     11 coal_ore
   12 iron_ore   13 gold_ore
"""
import os
import random
import struct
import zlib

COLS = 4
TS   = 16            # tile size in pixels
W = H = COLS * TS

rng = random.Random(1337)

def speck(base, jitter):
    return tuple(max(0, min(255, c + rng.randint(-jitter, jitter))) for c in base)

def grass_top(lx, ly):  return speck((86, 150, 60), 20) + (255,)
def dirt(lx, ly):       return speck((134, 96, 67), 18) + (255,)
def stone(lx, ly):      return speck((128, 128, 132), 16) + (255,)
def sand(lx, ly):       return speck((214, 200, 140), 14) + (255,)
def water(lx, ly):      return speck((54, 110, 200), 12) + (255,)

def grass_side(lx, ly):
    if ly < 4:      return speck((86, 150, 60), 20) + (255,)   # green cap (PNG top)
    elif ly < 6:    return speck((100, 140, 70), 22) + (255,)  # ragged transition
    else:           return speck((134, 96, 67), 18) + (255,)   # dirt below

def snow(lx, ly):       return speck((236, 240, 248), 8) + (255,)

def log_side(lx, ly):
    # Vertical bark grooves over a brown trunk.
    base = (104, 74, 44) if (lx % 5) in (0, 3) else (122, 88, 54)
    return speck(base, 10) + (255,)

def log_top(lx, ly):
    # Concentric growth rings around the tile centre.
    dx, dy = lx - 7.5, ly - 7.5
    r = (dx * dx + dy * dy) ** 0.5
    ring = (150, 112, 70) if int(r) % 2 == 0 else (120, 86, 52)
    return speck(ring, 8) + (255,)

def leaves(lx, ly):
    # Mottled green; tinted per-biome on the client. Slightly varied for depth.
    return speck((58, 110, 48), 26) + (255,)

def cactus(lx, ly):
    base = (60, 120, 60) if 2 <= lx <= 13 else (44, 96, 48)
    return speck(base, 12) + (255,)

def _ore(speck_color):
    def fn(lx, ly):
        # Speckle ore nuggets over a stone matrix.
        if (lx * 7 + ly * 5) % 11 < 3:
            return speck(speck_color, 14) + (255,)
        return speck((128, 128, 132), 16) + (255,)
    return fn

coal_ore = _ore((40, 40, 44))
iron_ore = _ore((196, 160, 120))
gold_ore = _ore((240, 208, 70))

TILES = {
    (0, 0): grass_top, (1, 0): grass_side, (2, 0): dirt,     (3, 0): stone,
    (0, 1): sand,      (1, 1): water,      (2, 1): snow,     (3, 1): log_side,
    (0, 2): log_top,   (1, 2): leaves,     (2, 2): cactus,   (3, 2): coal_ore,
    (0, 3): iron_ore,  (1, 3): gold_ore,
}

def main():
    # Default every pixel to opaque magenta so any unmapped tile is obvious.
    grid = [[(255, 0, 255, 255)] * W for _ in range(H)]
    for (col, row), fn in TILES.items():
        for ly in range(TS):
            for lx in range(TS):
                grid[row * TS + ly][col * TS + lx] = fn(lx, ly)

    raw = bytearray()
    for y in range(H):
        raw.append(0)  # filter type 0 per scanline
        for x in range(W):
            raw += bytes(grid[y][x])

    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    sig  = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", W, H, 8, 6, 0, 0, 0)  # 8-bit RGBA
    idat = zlib.compress(bytes(raw), 9)

    out = os.path.normpath(os.path.join(
        os.path.dirname(__file__), "..", "client", "assets", "textures", "atlas.png"))
    with open(out, "wb") as f:
        f.write(sig + chunk(b"IHDR", ihdr) + chunk(b"IDAT", idat) + chunk(b"IEND", b""))
    print("wrote", out, f"({W}x{H})")

if __name__ == "__main__":
    main()
