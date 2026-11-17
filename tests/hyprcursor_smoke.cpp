// hyprcursor-smoke — verify libhyprcursor loads the kryptos-cursors theme.
// Mirrors what Hyprland does at startup. Exit 0 = pass, 1 = fail.
#include <hyprcursor/hyprcursor.hpp>
#include <iostream>
#include <vector>

using namespace Hyprcursor;

static int failures = 0;

static void logFn(eHyprcursorLogLevel level, char* msg) {
    const char* lvl = level == HC_LOG_ERR ? "err " :
                      level == HC_LOG_WARN  ? "warn" :
                      level == HC_LOG_INFO  ? "info" : "trace";
    std::cout << "[hc " << lvl << "] " << msg << std::endl;
}

static void check(bool ok, const char* what) {
    std::cout << (ok ? "PASS" : "FAIL") << "  " << what << std::endl;
    if (!ok) failures++;
}

int main(int argc, const char** argv) {
    const char* themeName = argc > 1 ? argv[1] : "kryptos-cursors";

    // Path Hyprland takes with no HYPRCURSOR_THEME set: first-theme lookup.
    {
        CHyprcursorManager manager(themeName, logFn);
        check(manager.valid(), "theme loads by name");
        check(manager.loadThemeStyle({.size = 24}), "style 24 loads");
        std::vector<const char*> shapes = {"left_ptr", "watch", "text", "crosshair"};
        for (const char* shape : shapes) {
            auto data = manager.getShape(shape, {.size = 24});
            check(!data.images.empty(), shape);
            for (const auto& img : data.images) {
                std::cout << "      " << shape << ": size=" << img.size
                          << " hot=(" << img.hotspotX << "," << img.hotspotY << ")" << std::endl;
            }
        }
        // Raw metadata path
        auto raw = manager.getRawShapeData("left_ptr");
        std::cout << "      raw left_ptr: hotspot=(" << raw.hotspotX << "," << raw.hotspotY
                  << ") sizes=" << raw.images.size() << std::endl;
        check(!raw.images.empty(), "raw shape data has images");
    }

    // Bogus theme name must fail cleanly, not crash.
    {
        CHyprcursorManager bogus("no-such-theme-zzz", logFn);
        check(!bogus.valid(), "bogus theme reports invalid");
    }

    std::cout << (failures == 0 ? "RESULT: PASS" : "RESULT: FAIL") << std::endl;
    return failures == 0 ? 0 : 1;
}
