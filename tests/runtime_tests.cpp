// Exercise the real Win32/HID helpers without starting the host or changing zoom.
#define PINCH_ZOOM_STANDALONE
#define wmain diagnostic_main
#include "../mods/kde-pinch-zoom.wh.cpp"
#undef wmain
#include <iostream>

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
