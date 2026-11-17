/* xcursor-lite — libXcursor replacement, X11-free.
 * Copyright 2026 Brian Thompson (krypton-lang) — Apache 2.0.
 *
 * Reads the published Xcursor image format directly. No X server,
 * no libX11, no libxcb — just the files.
 *
 * File layout (little-endian only, which matches x86_64 / aarch64):
 *   FileHeader  : magic(4) header(4) version(4) ntoc(4)            [16 bytes]
 *   ntoc x FileToc: type(4) subtype(4) position(4)                 [12 bytes each]
 *   Chunks, located via toc.position:
 *     ChunkHeader: header(4) type(4) subtype(4) version(4)         [16 bytes]
 *     Image chunk: width(4) height(4) xhot(4) yhot(4) delay(4)
 *                  width*height*4 bytes of ARGB pixels
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "X11/Xcursor/Xcursor.h"

#define TOC_TYPE_IMAGE XCURSOR_IMAGE_TYPE
#define CHUNK_HEADER_LEN 16

/* ------------------------------------------------------------------ */
/* Shape-id → cursor name, mirroring libXcursor's XC_* enum (even ids) */
/* table index = shape / 2, from freedesktop's cursorfont.h order.    */
/* ------------------------------------------------------------------ */
static const char *XCURSOR_SHAPE_NAMES[77] = {
    "X_cursor",           "arrow",             "based_arrow_down",
    "based_arrow_up",     "boat",              "bogosity",
    "bottom_left_corner", "bottom_right_corner", "bottom_side",
    "bottom_tee",         "box_spiral",        "center_ptr",
    "circle",             "clock",             "coffee_mug",
    "cross",              "cross_reverse",     "crosshair",
    "diamond_cross",      "dot",               "dotbox",
    "double_arrow",       "draft_large",       "draft_small",
    "draped_box",         "exchange",          "fleur",
    "gobbler",            "gumby",             "hand1",
    "hand2",              "heart",             "icon",
    "iron_cross",         "left_ptr",          "left_side",
    "left_tee",           "leftbutton",        "ll_angle",
    "lr_angle",           "man",               "middlebutton",
    "mouse",              "pencil",            "pirate",
    "plus",               "question_arrow",    "right_ptr",
    "right_side",         "right_tee",         "rightbutton",
    "rtl_logo",           "sailboat",          "sb_down_arrow",
    "sb_h_double_arrow",  "sb_left_arrow",     "sb_right_arrow",
    "sb_up_arrow",        "sb_v_double_arrow", "shuttle",
    "sizing",             "spider",            "spraycan",
    "star",               "target",            "tcross",
    "top_left_arrow",     "top_left_corner",   "top_right_corner",
    "top_side",           "top_tee",           "trek",
    "ul_angle",           "umbrella",          "ur_angle",
    "watch",              "xterm",
};

static const char *shapeName(unsigned int shape) {
    unsigned int idx = shape / 2;
    if (idx >= sizeof(XCURSOR_SHAPE_NAMES) / sizeof(XCURSOR_SHAPE_NAMES[0])) return NULL;
    return XCURSOR_SHAPE_NAMES[idx];
}

/* ------------------------------------------------------------------ */
/* Basic image/images constructors                                     */
/* ------------------------------------------------------------------ */
XcursorImage *XcursorImageCreate(int width, int height) {
    XcursorImage *image = calloc(1, sizeof(XcursorImage));
    if (!image) return NULL;
    image->version = XCURSOR_IMAGE_VERSION;
    image->size    = 24;
    image->width   = width;
    image->height  = height;
    image->pixels  = calloc((size_t)width * (size_t)height, sizeof(XcursorPixel));
    if (!image->pixels) { free(image); return NULL; }
    return image;
}

void XcursorImageDestroy(XcursorImage *image) {
    if (!image) return;
    free(image->pixels);
    free(image);
}

XcursorImages *XcursorImagesCreate(int nimage) {
    XcursorImages *images = calloc(1, sizeof(XcursorImages));
    if (!images) return NULL;
    images->nimage = nimage;
    images->images = calloc((size_t)(nimage ? nimage : 1), sizeof(XcursorImage *));
    if (!images->images) { free(images); return NULL; }
    return images;
}

void XcursorImagesDestroy(XcursorImages *images) {
    if (!images) return;
    for (int i = 0; i < images->nimage; i++) XcursorImageDestroy(images->images[i]);
    free(images->images);
    free(images->name);
    free(images);
}

const char *XcursorLibraryPath(void) {
    const char *env = getenv("XCURSOR_PATH");
    if (env && *env) return env;
    return "~/.icons:~/.xcursors:/usr/share/icons:/usr/share/pixmaps:/usr/share/cursors/xcursors";
}

/* ------------------------------------------------------------------ */
/* Theme path resolution (colon-separated; empty token = default)      */
/* ------------------------------------------------------------------ */
#define PATH_MAX_TOKENS 32
typedef struct {
    char tokens[PATH_MAX_TOKENS][4096];
    int  count;
} PathList;

static void pathSplit(const char *raw, PathList *out) {
    out->count = 0;
    if (!raw) return;
    while (*raw && out->count < PATH_MAX_TOKENS) {
        const char *start = raw;
        while (*raw && *raw != ':') raw++;
        int len = (int)(raw - start);
        if (len >= (int)sizeof(out->tokens[0])) len = sizeof(out->tokens[0]) - 1;
        memcpy(out->tokens[out->count], start, len);
        out->tokens[out->count][len] = '\0';
        /* Empty token means the freedesktop default path. */
        if (len == 0) {
            const char *def = "~/.icons:~/.xcursors:/usr/share/icons:/usr/share/pixmaps:/usr/share/cursors/xcursors";
            const char *d = def;
            while (*d && out->count < PATH_MAX_TOKENS) {
                const char *dstart = d;
                while (*d && *d != ':') d++;
                int dlen = (int)(d - dstart);
                memcpy(out->tokens[out->count], dstart, dlen);
                out->tokens[out->count][dlen] = '\0';
                out->count++;
                if (*d == ':') d++;
            }
        } else {
            out->count++;
        }
        if (*raw == ':') raw++;
    }
}

/* Expand a leading "~/" to $HOME. */
static void expandTilde(const char *in, char *out, size_t outLen) {
    if (in[0] == '~' && in[1] == '/') {
        const char *home = getenv("HOME");
        if (home) {
            snprintf(out, outLen, "%s%s", home, in + 1);
            return;
        }
    }
    snprintf(out, outLen, "%s", in);
}

static int buildCursorPath(const char *dir, const char *theme, const char *shape, char *out, size_t outLen) {
    char expanded[4096];
    expandTilde(dir, expanded, sizeof(expanded));
    return (int)snprintf(out, outLen, "%s/%s/cursors/%s", expanded, theme, shape) < 0 ? -1
           : (int)strlen(out) >= (int)outLen ? -1 : 0;
}

static int fileExists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

/* ------------------------------------------------------------------ */
/* Parser                                                              */
/* ------------------------------------------------------------------ */
typedef struct {
    unsigned int size;    /* nominal (subtype) */
    unsigned int width, height, xhot, yhot, delay;
    XcursorPixel *pixels;
} RawImage;

static int readAll(FILE *f, void *buf, size_t n) {
    return fread(buf, 1, n, f) == n;
}

static bool readU32(FILE *f, XcursorUInt *out) {
    unsigned char b[4];
    if (!readAll(f, b, 4)) return false;
    *out = (XcursorUInt)b[0] | ((XcursorUInt)b[1] << 8) | ((XcursorUInt)b[2] << 16) | ((XcursorUInt)b[3] << 24);
    return true;
}

static XcursorImages *parseCursorFile(FILE *f, int wantSize) {
    XcursorUInt magic, header, version, ntoc;
    if (!readU32(f, &magic)) return NULL;
    if (magic != XCURSOR_MAGIC) return NULL;
    if (!readU32(f, &header) || header < XCURSOR_FILE_HEADER_LEN) return NULL;
    if (!readU32(f, &version)) return NULL;          /* version: ignored */
    if (!readU32(f, &ntoc)) return NULL;
    if (ntoc > 4096) return NULL;                    /* sanity */

    long tocPos = ftell(f);
    if (tocPos < 0) return NULL;

    /* Read all image chunks. */
    RawImage imgs[64];
    int imgCount = 0;

    for (unsigned int i = 0; i < ntoc; i++) {
        XcursorUInt type, subtype, position;
        long save = ftell(f);
        if (!readU32(f, &type)) return NULL;
        if (!readU32(f, &subtype)) return NULL;
        if (!readU32(f, &position)) return NULL;
        (void)save;

        if (type != TOC_TYPE_IMAGE) continue;
        if (position >= 0x7fffffff) continue;
        if (imgCount >= (int)(sizeof(imgs) / sizeof(imgs[0]))) continue;

        if (fseek(f, (long)position, SEEK_SET) != 0) continue;
        XcursorUInt chHeader, chType, chSubtype, chVersion;
        if (!readU32(f, &chHeader) || chHeader < CHUNK_HEADER_LEN) continue;
        if (!readU32(f, &chType) || chType != TOC_TYPE_IMAGE) continue;
        if (!readU32(f, &chSubtype)) continue;
        if (!readU32(f, &chVersion)) continue;       /* ignored */
        if (!readU32(f, &imgs[imgCount].width))  continue;
        if (!readU32(f, &imgs[imgCount].height)) continue;
        if (!readU32(f, &imgs[imgCount].xhot))   continue;
        if (!readU32(f, &imgs[imgCount].yhot))   continue;
        if (!readU32(f, &imgs[imgCount].delay))  continue;
        if (imgs[imgCount].width == 0 || imgs[imgCount].height == 0) continue;
        if (imgs[imgCount].width > XCURSOR_IMAGE_MAX_SIZE ||
            imgs[imgCount].height > XCURSOR_IMAGE_MAX_SIZE) continue;
        size_t px = (size_t)imgs[imgCount].width * imgs[imgCount].height;
        imgs[imgCount].pixels = malloc(px * sizeof(XcursorPixel));
        if (!imgs[imgCount].pixels) continue;
        size_t got = fread(imgs[imgCount].pixels, sizeof(XcursorPixel), px, f);
        if (got != px) { free(imgs[imgCount].pixels); continue; }
        imgs[imgCount].size = chSubtype;
        imgCount++;
    }

    if (imgCount == 0) return NULL;

    /* Pick the nominal size closest to wantSize (ties → larger size). */
    int best = 0;
    long bestDiff = 0x7fffffffL * 2L;
    for (int i = 0; i < imgCount; i++) {
        long diff = (long)imgs[i].size - wantSize;
        if (diff < 0) diff = -diff;
        if (diff < bestDiff || (diff == bestDiff && imgs[i].size > imgs[best].size)) {
            bestDiff = diff;
            best = i;
        }
    }
    unsigned int chosenNominal = imgs[best].size;

    int nChosen = 0;
    for (int i = 0; i < imgCount; i++) if (imgs[i].size == chosenNominal) nChosen++;

    XcursorImages *out = XcursorImagesCreate(nChosen);
    if (!out) {
        for (int i = 0; i < imgCount; i++) free(imgs[i].pixels);
        return NULL;
    }
    int j = 0;
    for (int i = 0; i < imgCount; i++) {
        if (imgs[i].size != chosenNominal) { free(imgs[i].pixels); continue; }
        XcursorImage *im = XcursorImageCreate((int)imgs[i].width, (int)imgs[i].height);
        if (!im) { free(imgs[i].pixels); continue; }
        im->size   = imgs[i].size;
        im->xhot   = imgs[i].xhot;
        im->yhot   = imgs[i].yhot;
        im->delay  = imgs[i].delay;
        memcpy(im->pixels, imgs[i].pixels, (size_t)imgs[i].width * imgs[i].height * sizeof(XcursorPixel));
        free(imgs[i].pixels);
        out->images[j++] = im;
    }
    out->nimage = j;
    return out;
}

/* Load a single theme file; returns images or NULL. */
static XcursorImages *loadOneTheme(const char *dir, const char *theme, const char *shape, int size, const char *wantName) {
    char path[4096];
    if (buildCursorPath(dir, theme, shape, path, sizeof(path)) < 0) return NULL;
    if (!fileExists(path)) return NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    XcursorImages *imgs = parseCursorFile(f, size);
    fclose(f);
    if (imgs && wantName) {
        imgs->name = strdup(wantName);
    }
    return imgs;
}

XcursorImages *XcursorFileLoadImages(FILE *file, int size) {
    if (!file) return NULL;
    long save = ftell(file);
    XcursorImages *imgs = parseCursorFile(file, size);
    if (save >= 0) fseek(file, save, SEEK_SET);
    return imgs;
}

XcursorImages *XcursorFilenameLoadImages(const char *file, int size) {
    if (!file) return NULL;
    FILE *f = fopen(file, "rb");
    if (!f) return NULL;
    XcursorImages *imgs = parseCursorFile(f, size);
    fclose(f);
    return imgs;
}

XcursorImages *XcursorLibraryLoadImages(const char *file, const char *theme, int size) {
    (void)theme; /* library path themes are just directories; handled by caller */
    return XcursorFilenameLoadImages(file, size);
}

XcursorImages *XcursorShapeLoadImages(unsigned int shape, const char *theme, int size) {
    const char *shapeNameStr = shapeName(shape);
    if (!shapeNameStr) return NULL;
    if (!theme || !*theme) theme = "default";

    PathList dirs;
    pathSplit(XcursorLibraryPath(), &dirs);

    for (int i = 0; i < dirs.count; i++) {
        XcursorImages *imgs = loadOneTheme(dirs.tokens[i], theme, shapeNameStr, size, shapeNameStr);
        if (imgs) return imgs;
    }
    /* Fall back to the "default" theme inside each path (inherits). */
    for (int i = 0; i < dirs.count; i++) {
        XcursorImages *imgs = loadOneTheme(dirs.tokens[i], "default", shapeNameStr, size, shapeNameStr);
        if (imgs) return imgs;
    }
    return NULL;
}
