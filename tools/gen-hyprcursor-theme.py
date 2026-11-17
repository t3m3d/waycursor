#!/usr/bin/env python3
# gen-hyprcursor-theme.py — generate KRYPTOS Minimal as a *hyprcursor* theme.
#
# Hyprland 0.45 loads hyprcursor themes preferentially; without any theme the
# compositor falls back to Xcursor. This generator writes the native format:
#   <out>/<name>/manifest.hl
#   <out>/<name>/hyprcursors/<shape>/meta
#   <out>/<name>/hyprcursors/<shape>/<shape>_<size>.png
#
# Usage: python3 gen-hyprcursor-theme.py OUTDIR [name]
# Pure Python stdlib (zlib + struct); no PIL required.
# KryptOS Minimal = the cursor theme Hyprland loads on KryptOS.

import os
import struct
import sys
import zlib

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

# shape name → hotspot (in 24px grid coords)
HOTSPOT = {"left_ptr": (1, 1), "arrow": (1, 1), "top_left_arrow": (1, 1),
           "watch": (12, 12), "text": (11, 12), "crosshair": (11, 11)}
ALIAS = {"arrow": "left_ptr", "top_left_arrow": "left_ptr"}
SIZES = (24, 48)


def scale_art(rows, factor):
    out = []
    for row in rows:
        big = "".join(c * factor for c in row)
        for _ in range(factor):
            out.append(big)
    return out


def png_bytes(rows):
    h, w = len(rows), len(rows[0])
    # rows may be ragged; normalize to max width
    w = max(len(r) for r in rows)
    rows = [r.ljust(w, ".") for r in rows]
    raw = b""
    for row in rows:
        raw += b"\x00"
        for c in row:
            if c == "X":
                raw += struct.pack("<I", 0xFFE6E6E6)  # RGBA white
            else:
                raw += b"\x00\x00\x00\x00"

    def chunk(tag, payload):
        return (struct.pack(">I", len(payload)) + tag + payload
                + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF))

    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)  # 8-bit RGBA
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b"")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    outdir = sys.argv[1]
    theme = sys.argv[2] if len(sys.argv) > 2 else "kryptos-cursors"
    root = os.path.join(outdir, theme)
    cur = os.path.join(root, "hyprcursors")
    os.makedirs(cur, exist_ok=True)

    with open(os.path.join(root, "manifest.hl"), "w") as f:
        f.write("name = KryptOS Minimal\n"
                "description = Minimal cursor set for KryptOS (Wayland-native)\n"
                "version = 1.0\n"
                "cursors_directory = hyprcursors\n"
                "author = KryptOS\n")

    import zipfile
    for name, rows in ART.items():
        variants = []
        for size in SIZES:
            if size == 24:
                v, hs = rows, HOTSPOT[name]
            else:
                factor = size // 24
                v = scale_art(rows, factor)
                hs = tuple(x * factor for x in HOTSPOT[name])
            variants.append((size, f"{name}_{size}.png", hs, png_bytes(v)))
        size0, _f0, (hx0, hy0) = variants[0][0], variants[0][1], variants[0][2]
        # hyprcursor hotspots are normalized 0..1 floats (x/size, y/size)
        meta = (f"hotspot_x = {hx0 / size0:.6f}\n"
                f"hotspot_y = {hy0 / size0:.6f}\n"
                f"resize_algorithm = nearest\n")
        for size, fname, _hs, _png in variants:
            meta += f"define_size = {size}, {fname}\n"
        # Each shape is one zip: meta.hl + the PNG images.
        with zipfile.ZipFile(os.path.join(cur, f"{name}.hyprcursor"), "w", zipfile.ZIP_DEFLATED) as zf:
            zf.writestr("meta.hl", meta)
            for size, fname, _hs, png in variants:
                zf.writestr(fname, png)
    print(f"hyprcursor theme written to {root}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
