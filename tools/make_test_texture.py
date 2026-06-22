"""Generate a simple checkerboard PNG using only the Python standard library.
Used as a placeholder texture until the real block atlas arrives (M6)."""
import struct
import zlib
import os

W = H = 64
TILE = 8
COLOR_A = (222, 120, 60)   # orange
COLOR_B = (60, 120, 222)   # blue

def main():
    raw = bytearray()
    for y in range(H):
        raw.append(0)  # PNG filter type 0 (None) per scanline
        for x in range(W):
            a = ((x // TILE) + (y // TILE)) % 2 == 0
            r, g, b = COLOR_A if a else COLOR_B
            raw += bytes((r, g, b, 255))

    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", W, H, 8, 6, 0, 0, 0)  # 8-bit RGBA
    idat = zlib.compress(bytes(raw), 9)

    out = os.path.join(os.path.dirname(__file__), "..", "game", "assets", "textures", "test.png")
    out = os.path.normpath(out)
    with open(out, "wb") as f:
        f.write(sig + chunk(b"IHDR", ihdr) + chunk(b"IDAT", idat) + chunk(b"IEND", b""))
    print("wrote", out)

if __name__ == "__main__":
    main()
