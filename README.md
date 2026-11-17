# xcursor-lite — Wayland-native cursor stack (no X11)

Two things:

1. **xcursor-lite** — a clean-room, X11-free implementation of the
   `libXcursor.so.1` ABI. Parses `.xcursor` files directly; needs no X
   server, no libX11, no libxcb. Hyprland 0.45 links against it
   unmodified via `pkg-config xcursor` and `-I/usr/include`.
   Replaces the entire X11 cursor chain (libXcursor → libX11 → libXau,
   libXdmcp, libxcb, xcb-proto) on a fresh KryptOS desktop rebuild.

2. **kryptos-cursors** — minimal theme (`tools/gen-theme.py`) with
   `left_ptr`/`arrow`, `watch`, `text`, `crosshair`, 24 and 48 px.
   The image shipped no theme at all; without one every cursor lookup
   fell back to Hyprland's embedded arrow.

## Layout

    include/X11/Xcursor/Xcursor.h   public header (Xlib-free)
    src/xcursor.c                   implementation
    tools/gen-theme.py              theme generator
    tests/test_xcursor.c            headless smoke test

## Verify

    cmake -B build -DCMAKE_INSTALL_PREFIX=/usr . && cmake --build build
    ./build/test-xcursor /path/to/theme/cursors/left_ptr
    pkg-config --exists xcursor    # after install

## KryptOS integration

`scripts/chroot/ch11-desktop-p5-lcursor.sh` in the kryptos repo builds both
on a fresh rebuild; `ch11-desktop-p5-xcursor.sh` is superseded. Hyprland
(Phase 4) then links `libXcursor.so.1` from this package with no other
source changes.
