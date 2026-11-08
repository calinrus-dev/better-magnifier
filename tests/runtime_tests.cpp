// Exercise the real Win32/HID helpers without starting the host or changing zoom.
#define PINCH_ZOOM_STANDALONE
#define PINCH_ZOOM_RUNTIME_TEST
#define wmain diagnostic_main
#include "../mods/kde-pinch-zoom.wh.cpp"
#undef wmain
#include <iostream>

namespace {
pinch::View simulated;
POINT cursor{200, 150};
BOOL WINAPI readView(float* level, int* x, int* y) {
    *level = static_cast<float>(simulated.zoom); *x = simulated.x; *y = simulated.y; return TRUE;
}
BOOL WINAPI writeView(float level, int x, int y) { simulated = {level, x, y}; return TRUE; }
BOOL WINAPI readCursor(POINT* point) { *point = cursor; return TRUE; }
}

int wmain(int argc, wchar_t** argv) {
    int checks = 0;
    const auto require = [&](bool valid, const char* name) {
        if (!valid) {
            std::cerr << "FAIL " << name << '\n';
            std::exit(1);
        }
        ++checks;
    };
    HIDP_VALUE_CAPS caps{};
    caps.LogicalMin = -32768;
    caps.LogicalMax = 32767;
    caps.BitSize = 16;
    require(std::abs(runtime::coordinate(0x8000, caps, 100)) < 0.00001, "signed HID coordinate minimum");
    require(std::abs(runtime::coordinate(0x7FFF, caps, 100) - 100) < 0.00001, "signed HID coordinate maximum");
    caps.LogicalMin = 0;
    caps.LogicalMax = -1;
    caps.BitSize = 32;
    require(runtime::logicalMaximum(caps) == 4294967295.0, "unsigned 32-bit HID maximum");
    caps.PhysicalMin = 0;
    caps.PhysicalMax = 1200;
    caps.Units = 0x11;
    caps.UnitsExp = 0x0E;  // -2, centimetres
    require(std::abs(runtime::physicalMillimeters(caps) - 120) < 0.00001, "HID physical units to millimetres");
    caps.Units = 0;
    require(runtime::physicalMillimeters(caps) == 0, "unknown physical units use fallback");
    caps.Units = 0x13;
    caps.UnitsExp = 0x0F;
    caps.PhysicalMax = 40;
    require(std::abs(runtime::physicalMillimeters(caps) - 101.6) < 0.00001, "English linear HID units");
    {
        runtime::Zoom zoom;
        simulated = {2, 0, 0};
        zoom.useTestBackend(readView, writeView, readCursor);
        require(zoom.target(2.2, true) && zoom.tick(16, 0), "adopt a keyboard zoom before a pinch");
        require(std::abs(simulated.zoom - 2.2) < 0.001, "pinch works above an externally selected zoom");
        const auto before = zoom.revision();
        simulated = {3, 20, 10};
        require(zoom.tick(16, 0) && zoom.revision() != before, "keyboard change rebases rather than locking the controller");
        require(zoom.tick(16, 0) && simulated.x == 20 && simulated.y == 10,
                "external viewport is not overwritten by a stale anchor on the next tick");
        require(zoom.target(2.8, true) && zoom.tick(16, 0), "pinch can zoom out immediately after a keyboard change");
        require(std::abs(simulated.zoom - 2.8) < 0.001, "shared keyboard/pinch scale");
        const auto beforePointer = simulated;
        cursor = {320, 200};
        const auto expected = pinch::anchoredView(3.1, pinch::anchorAt(beforePointer, {320, 200}),
            {static_cast<double>(GetSystemMetrics(SM_XVIRTUALSCREEN)), static_cast<double>(GetSystemMetrics(SM_YVIRTUALSCREEN)),
             static_cast<double>(GetSystemMetrics(SM_CXVIRTUALSCREEN)), static_cast<double>(GetSystemMetrics(SM_CYVIRTUALSCREEN))});
        require(zoom.target(3.1) && zoom.tick(16, 0), "pointer can move while pinching");
        require(simulated.x == expected.x && simulated.y == expected.y,
                "moving pointer updates the pinch focal point without a stale-position jump");
        simulated = {4, 0, 0};
        require(zoom.resetToNormal() && simulated.zoom == 1, "emergency reset works even after external changes");
    }
    if (argc != 2) {
        std::cerr << "Usage: runtime-tests path-to-mod.dll\n";
        return 2;
    }
    // Map only. Never resolve imports or execute DllMain/callbacks in tests.
    const HMODULE mod = LoadLibraryExW(argv[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    require(mod != nullptr, "map compiled x64 mod without executing it");
    for (const char* symbol : {"InternalWhModPtr", "_Z10Wh_ModInitv", "_Z15Wh_ModAfterInitv",
                               "_Z21Wh_ModSettingsChangedPi", "_Z12Wh_ModUninitv"}) {
        require(GetProcAddress(mod, symbol) != nullptr, symbol);
    }
    FreeLibrary(mod);
    std::cout << checks << " runtime/ABI checks passed; desktop unchanged\n";
}
