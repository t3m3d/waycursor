/* test_xcursor — headless verification for xcursor-lite.
 * Loads a .xcursor file and shape lookups, prints what it finds.
 * Exit 0 = everything parsed; exit 1 = a load that must succeed failed.
 */
#include <stdio.h>
#include <string.h>
#include "X11/Xcursor/Xcursor.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <path-to-.xcursor-file-or-theme-root>\n", argv[0]);
        return 2;
    }

    /* File load */
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }
    XcursorImages *imgs = XcursorFileLoadImages(f, 24);
    fclose(f);
    if (!imgs) { fprintf(stderr, "FAIL: XcursorFileLoadImages returned NULL\n"); return 1; }
    printf("nimage=%d name=%s\n", imgs->nimage, imgs->name ? imgs->name : "(null)");
    for (int i = 0; i < imgs->nimage; i++) {
        XcursorImage *im = imgs->images[i];
        printf("image[%d] size=%u %ux%u hot=(%u,%u) delay=%u firstpx=%08x\n",
               i, im->size, im->width, im->height, im->xhot, im->yhot, im->delay,
               im->pixels ? im->pixels[0] : 0);
    }
    XcursorImagesDestroy(imgs);

    /* Request a bigger size — must still load (closest-size selection). */
    f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }
    imgs = XcursorFileLoadImages(f, 64);
    fclose(f);
    if (!imgs) { fprintf(stderr, "FAIL: closest-size load returned NULL\n"); return 1; }
    printf("closest(64) nimage=%d size=%u\n", imgs->nimage, imgs->images[0] ? imgs->images[0]->size : 0);
    XcursorImagesDestroy(imgs);

    /* Shape lookup via XCURSOR_PATH (caller sets it) */
    const char *path = XcursorLibraryPath();
    printf("library path: %s\n", path);

    imgs = XcursorShapeLoadImages(68 /* left_ptr */, "kryptos-cursors", 24);
    if (imgs) {
        printf("shape(68,kryptos-cursors,24) nimage=%d w=%u h=%u\n",
               imgs->nimage, imgs->images[0]->width, imgs->images[0]->height);
        XcursorImagesDestroy(imgs);
    } else {
        printf("shape(68) not found (expected if no kryptos-cursors theme on path)\n");
    }

    /* A bogus shape id must fail cleanly, not crash */
    imgs = XcursorShapeLoadImages(0xdeadbeef, "kryptos-cursors", 24);
    printf("bogus shape -> %s\n", imgs ? "images?!" : "NULL (correct)");
    if (imgs) XcursorImagesDestroy(imgs);

    return 0;
}
