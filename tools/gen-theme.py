#!/usr/bin/env python3
# gen-theme.py — generate the KRYPTOS minimal cursor theme (.xcursor format).
#
# The image ships no cursor theme at all today; this is the minimal fallback
# the Wayland Xcursor path loads when hyprcursor has no theme. One .xcursor
# file per shape, each file carrying every nominal size (24, 32), so
# xcursor-lite's closest-size selection is exercised for real.
#
# Usage: python3 gen-theme.py OUTDIR [name]    # writes OUTDIR/<name>/cursors/*
# Pixel art is authored at 24x24 and nearest-neighbor scaled up.

import os
import struct
import sys

ART = {
    # classic diagonal arrow, tip top-left
    "left_ptr": [
        "X......................",
        "XX.....................",
        "XXX....................",
        "XXXX...................",
        "XXXXX..................",
        "XXXXXX.................",
        "XXXXXXX................",
        "XXXXXXXX...............",
        "XXXXXXXXX..............",
        "XXXXXXXXXX.............",
        "XXXXXXXXXXX............",
        "XXXXXXXXXXXX...........",
        "XXXXXXXXXXX............",
        "XXXXXX................",
        "XXXX..XX..............",
        "XX.X...XX.............",
        "X..X....X.............",
        "...X..................",
        "......................",
        "......................",
        "......................",
        "......................",
        "......................",
        "......................",
    ],
    # watch: circle with two hands
    "watch": [
        ".......XXXXXXXX.......",
        ".....XX........XX.....",
        "....X............X....",
        "...X......X.......X...",
        "..X.......X.......X...",
        "..X.......X.......X...",
        ".X........X........X..",
        ".X........X........X..",
        ".X........XXXXX....X..",
        ".X.................X..",
        ".X.................X..",
        ".X.................X..",
        ".X.................X..",
        ".X.................X..",
        "..X...............X...",
        "..X......X........X...",
        "...X......X.......X...",
        "....X......XX....X....",
        ".....XX........XX.....",
        ".......XXXXXXXX.......",
        "......................",
        "......................",
        "......................",
        "......................",
    ],
    # I-beam for text fields
    "text": [
        "......XXXXXXXX........",
        "..XXXXXXXXXXXXXXXX....",
        "......XXXXXXXX........",
        "........X.............",
        "........X.............",
        "........X.............",
        "........X.............",
        "........X.............",
        "........X.............",
        "........X.............",
        "........X.............",
        "........X.............",
        "........X.............",
        "........X.............",
        "........X.............",
        "........X.............",
        "......XXXXXXXX........",
        "..XXXXXXXXXXXXXXXX....",
        "......XXXXXXXX........",
        "......................",
        "......................",
        "......................",
        "......................",
        "......................",
    ],
    # crosshair
    "crosshair": [
        "......................",
        "......................",
        "........X...X.........",
        "........X...X.........",
        "........X...X.........",
        "........X...X.........",
        "XXX.....XXXXX.....XXX.",
        "XXX....XXXXXXX....XXX.",
        "XXX.....XXXXX.....XXX.",
        "........X...X.........",
        "........X...X.........",
        "........X...X.........",
        "........X...X.........",
        "........X...X.........",
        "......................",
        "......................",
        "......................",
        "......................",
        "......................",
        "......................",
        "......................",
        "......................",
        "......................",
        "......................",
    ],
}

# shape id (freedesktop cursorfont.h order, even ids) → art
SHAPE_IDS = {"left_ptr": 66, "arrow": 2, "top_left_arrow": 68,
             "watch": 150, "text": 152, "crosshair": 34}
ALIAS = {"arrow": "left_ptr", "top_left_arrow": "left_ptr"}
HOTSPOT = {"left_ptr": (1, 1), "arrow": (1, 1), "top_left_arrow": (1, 1),
           "watch": (12, 12), "text": (11, 12), "crosshair": (11, 11)}

MAGIC = 0x72756358
TOC_TYPE_IMAGE = 0xFFFD0002
SIZES = (24, 48)


def scale_art(rows, factor):
    out = []
    for row in rows:
        big = "".join(c * factor for c in row)
        for _ in range(factor):
            out.append(big)
    return out


def pixels_from_art(rows):
    px = []
    for row in rows:
        for c in row:
            px.append(0xFFE6E6E6 if c != "." else 0x00000000)
    return px


def write_cursor_file(path, variants):
    """variants: list of (nominal_size, rows, xhot, yhot, delay_ms)."""
    toc = []
    chunks = []
    offset = 16 + len(variants) * 12
    for size, rows, xhot, yhot, delay_ms in variants:
        h = len(rows)
        w = len(rows[0])
        px = pixels_from_art(rows)
        header = [36, TOC_TYPE_IMAGE, size, 1]          # ChunkHeader
        image  = [w, h, xhot, yhot, delay_ms] + px
        chunk  = header + image
        toc.append([TOC_TYPE_IMAGE, size, offset])
        chunks.append(chunk)
        offset += len(chunk) * 4
    header = [MAGIC, 16, 0x00010000, len(variants)]
    data = header + [v for entry in toc for v in entry]
    for chunk in chunks:
        data += chunk
    with open(path, "wb") as f:
        f.write(b"".join(struct.pack("<I", v) for v in data))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    outdir = sys.argv[1]
    theme = sys.argv[2] if len(sys.argv) > 2 else "kryptos-cursors"
    curdir = os.path.join(outdir, theme, "cursors")
    os.makedirs(curdir, exist_ok=True)

    for name, rows in ART.items():
        variants = []
        for size in SIZES:
            if size == 24:
                variants.append((24, rows,) + HOTSPOT[name] + (0,))
            else:
                factor = size // 24
                rows2 = scale_art(rows, factor)
                assert len(rows2) == size
                variants.append((size, rows2, HOTSPOT[name][0] * factor, HOTSPOT[name][1] * factor, 0))
        write_cursor_file(os.path.join(curdir, name), variants)
    with open(os.path.join(outdir, theme, "index.theme"), "w") as f:
        f.write("[Icon Theme]\nName=KryptOS Minimal\nInherits=default\n")
    print(f"theme written to {os.path.join(outdir, theme)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
