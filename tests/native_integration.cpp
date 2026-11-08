// Opt-in desktop test. Not run by check.ps1 or CI; requires the installed mod.
#define PINCH_ZOOM_STANDALONE
#define wmain diagnostic_main
#include "../mods/kde-pinch-zoom.wh.cpp"
#undef wmain
#include <iostream>

int wmain(int argc, wchar_t** argv) {
    if (argc != 2 || wcscmp(argv[1], L"--live") != 0) {
        std::cerr << "Use --live: temporarily changes desktop zoom and restores its initial level.\n";
        return 2;
    }
    bridge::Mapping mapping;
    runtime::Zoom zoom;
    if (!mapping.open() || !mapping.gateAvailable() || !mapping.magnifierAvailable() || !zoom.initialize()) return 3;
    const double initial = mapping.nativeLevel();
    struct Restore {
        bridge::Mapping& mapping;
        double level;
        ~Restore() { mapping.requestLevel(level); }
    } restore{mapping, initial};
    const auto waitFor = [&](double level) {
        const auto deadline = GetTickCount64() + 1500;
        do {
            if (std::abs(mapping.nativeLevel() - level) < 0.002 && std::abs(zoom.rendered() - level) < 0.005) return true;
            Sleep(5);
        } while (GetTickCount64() < deadline);
        std::cerr << "Native zoom did not settle at " << level << "; rendered=" << zoom.rendered() << '\n';
        return false;
    };
    int checks = 0;
    const auto require = [&](bool result, const char* name) {
        if (!result) { std::cerr << "FAIL " << name << '\n'; return false; }
        std::cout << "PASS " << name << '\n'; ++checks; return true;
    };
    if (!require(zoom.target(1.25, true) && waitFor(1.25), "pinch controller reaches native renderer")) return 1;
    for (const double level : {1.4, 1.7, 1.3, 1.6, 1.15}) {
        if (!zoom.target(level)) return 1;
        Sleep(4);
    }
    if (!require(waitFor(1.15), "rapid reversals coalesce to the final native target")) return 1;
    const LONG before = mapping.get()->nativeCommands;
    InterlockedExchange(&mapping.get()->pendingCommand, 1);
    mapping.syncNative();
    const auto deadline = GetTickCount64() + 1500;
    while (mapping.get()->nativeCommands == before && GetTickCount64() < deadline) Sleep(5);
    if (!require(mapping.get()->nativeCommands != before && mapping.nativeLevel() > 1.15,
                 "native increment starts from the pinch level (IPC, no key injection)")) return 1;
    if (!require(zoom.target(1.35, true) && waitFor(1.35), "pinch resumes after native increment")) return 1;
    if (!require(zoom.resetToNormal() && waitFor(1), "reset reaches 100 percent through native renderer")) return 1;
    std::cout << checks << " live integration checks passed; restoring initial level\n";
}
