/* xcursor-lite — libXcursor replacement
 * Copyright 2026 Brian Thompson (krypton-lang) — Apache 2.0.
 *
 * Wayland-era, X11-free implementation of the libXcursor public ABI subset.
 * Parses the published Xcursor image format directly; needs no X server,
 * no libX11, no libxcb. Hyprland 0.45 links against this unmodified
 * (it resolves `xcursor` via pkg-config, as before).
 *
 * Supported on little-endian Linux (x86_64 / aarch64). The .xcursor file
 * format is native-endian; big-endian is intentionally unsupported.
 */
#ifndef _XCURSOR_H_
#define _XCURSOR_H_

#include <stdint.h>
#include <stdio.h>

typedef int         XcursorBool;
typedef uint32_t    XcursorUInt;
typedef XcursorUInt XcursorDim;
typedef XcursorUInt XcursorPixel;

#define XcursorTrue  1
#define XcursorFalse 0

typedef struct _XcursorImage {
    XcursorUInt     version;
    XcursorDim      size;
    XcursorDim      width;
    XcursorDim      height;
    XcursorDim      xhot;
    XcursorDim      yhot;
    XcursorUInt     delay;
    XcursorPixel    *pixels;
} XcursorImage;

typedef struct _XcursorImages {
    int             nimage;
    XcursorImage    **images;
    char            *name;
} XcursorImages;

#define XCURSOR_MAGIC        0x72756358 /* "Xcur" LSBFirst */
#define XCURSOR_FILE_MAJOR   1
#define XCURSOR_FILE_MINOR   0
#define XCURSOR_FILE_VERSION ((XCURSOR_FILE_MAJOR << 16) | XCURSOR_FILE_MINOR)
#define XCURSOR_FILE_HEADER_LEN (4 * 4)
#define XCURSOR_IMAGE_TYPE   0xfffd0002
#define XCURSOR_IMAGE_VERSION 1
#define XCURSOR_IMAGE_MAX_SIZE 0x7fff
#define XCURSOR_COMMENT_TYPE 0xfffe0001

#ifdef __cplusplus
extern "C" {
#endif

XcursorImage *XcursorImageCreate(int width, int height);
void          XcursorImageDestroy(XcursorImage *image);
XcursorImages *XcursorImagesCreate(int nimage);
void          XcursorImagesDestroy(XcursorImages *images);

/* Primary entry points used by Wayland compositors and toolkits.
 * Load the image group whose nominal size is closest to `size`;
 * returns NULL when nothing usable is found. */
XcursorImages *XcursorShapeLoadImages(unsigned int shape, const char *theme, int size);
XcursorImages *XcursorFileLoadImages(FILE *file, int size);
XcursorImages *XcursorLibraryLoadImages(const char *file, const char *theme, int size);
XcursorImages *XcursorFilenameLoadImages(const char *file, int size);

/* Theme search path (colon-separated; later entries are fallbacks).
 * Honors $XCURSOR_PATH; otherwise the freedesktop default is returned.
 * Returns a pointer to a static buffer — do not free, do not mutate. */
const char   *XcursorLibraryPath(void);

#ifdef __cplusplus
}
#endif

#endif /* _XCURSOR_H_ */
