// Opt-in desktop test. Not run by check.ps1 or CI; requires the installed mod.
// It restores the initial zoom factor; it does not inject keyboard input.
#define PINCH_ZOOM_STANDALONE
#define wmain diagnostic_main
#include "../mods/kde-pinch-zoom.wh.cpp"
#undef wmain
#include <iostream>

namespace {
bool waitForNative(bridge::Mapping& mapping, runtime::Zoom& zoom, double level, std::optional<LONG> sequence = {}) {
    const auto deadline = GetTickCount64() + 1500;
    do {
        const bool applied = !sequence || InterlockedCompareExchange(&mapping.get()->appliedSequence, 0, 0) == *sequence;
        if (applied && std::abs(mapping.nativeLevel() - level) < 0.002 && std::abs(zoom.rendered() - level) < 0.005) return true;
        Sleep(5);
    } while (GetTickCount64() < deadline);
    std::cerr << "Native zoom did not settle at " << level << "; rendered=" << zoom.rendered() << '\n';
    return false;
}
struct RestoreNativeLevel {
    bridge::Mapping& mapping;
    runtime::Zoom& zoom;
    double level;
    bool restored = false;
    bool restore() {
        if (!mapping.requestLevel(level)) return false;
        const LONG sequence = mapping.get()->requestedSequence;
        restored = waitForNative(mapping, zoom, level, sequence);
        return restored;
    }
    ~RestoreNativeLevel() {
        if (!restored && !restore()) std::cerr << "Could not confirm restoration of the initial native zoom factor.\n";
    }
};
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 2 || wcscmp(argv[1], L"--live") != 0) {
        std::cerr << "Use --live: temporarily changes desktop zoom and restores its initial factor.\n";
        return 2;
    }
    bridge::Mapping mapping;
    runtime::Zoom zoom;
    if (!mapping.open() || !mapping.hostAvailable() || !mapping.magnifierAvailable() ||
        !(mapping.get()->nativeCaps & compatibility::NativeCore) || (mapping.get()->shellMask & 1) ||
        mapping.get()->overviewSavedLevel > 0 || !zoom.initialize()) {
        std::cerr << "Native host/UI is unavailable or Task View is active/restoring; no zoom changes made.\n";
        return 3;
    }
    RestoreNativeLevel restore{mapping, zoom, mapping.nativeLevel()};
    const auto submit = [&](double level, bridge::InputSource source = bridge::InputSource::Pinch) {
        // Adopt changes from a preceding prime/native command before submitting
        // the next source. A successful enqueue, not a stale no-op, is required.
        zoom.tick(0, 0);
        const LONG sequence = mapping.get()->requestedSequence;
        if (!zoom.target(level, true, source)) return false;
        if (mapping.get()->requestedSequence != sequence) return true;
        zoom.tick(0, 0);
        return zoom.target(level, true, source) && mapping.get()->requestedSequence != sequence;
    };
    const auto move = [&](double level, bridge::InputSource source = bridge::InputSource::Pinch) {
        return submit(level, source) && waitForNative(mapping, zoom, level, mapping.get()->requestedSequence);
    };
    const auto reset = [&] {
        const LONG before = mapping.get()->nativeCommands;
        if (!zoom.resetToNormal()) return false;
        const auto deadline = GetTickCount64() + 1500;
        while (mapping.get()->nativeCommands == before && GetTickCount64() < deadline) Sleep(5);
        return mapping.get()->nativeCommands != before &&
               mapping.get()->currentSource == static_cast<LONG>(bridge::InputSource::Reset) && waitForNative(mapping, zoom, 1);
    };
    int checks = 0;
    const auto require = [&](bool result, const char* name) {
        if (!result) { std::cerr << "FAIL " << name << '\n'; return false; }
        std::cout << "PASS " << name << '\n'; ++checks; return true;
    };
    if (!require(reset(), "native UI prepares one hundred percent for cold input")) return 1;
    if (!require(move(1.005) && mapping.get()->currentSource == static_cast<LONG>(bridge::InputSource::Pinch),
                 "first fine pinch source reaches the native renderer from one hundred percent")) return 1;
    for (const double level : {1.4, 1.7, 1.3, 1.6, 1.15}) {
        if (!submit(level)) return 1;
        Sleep(4);
    }
    if (!require(waitForNative(mapping, zoom, 1.15, mapping.get()->requestedSequence), "rapid reversals coalesce to the final native target")) return 1;
    if (!reset()) return 1;
    if (!require(move(1.005, bridge::InputSource::Wheel) && mapping.get()->currentSource == static_cast<LONG>(bridge::InputSource::Wheel),
                 "first wheel source preserves a sub-percent target from one hundred percent")) return 1;
    if (!reset()) return 1;
    const LONG before = mapping.get()->nativeCommands;
    InterlockedExchange(&mapping.get()->pendingCommand, 1);
    mapping.syncNative();
    const auto deadline = GetTickCount64() + 1500;
    while (mapping.get()->nativeCommands == before && GetTickCount64() < deadline) Sleep(5);
    const double keyboardTarget = mapping.nativeLevel();
    if (!require(mapping.get()->nativeCommands != before && keyboardTarget > 1 &&
                 mapping.get()->currentSource == static_cast<LONG>(bridge::InputSource::Keyboard) && waitForNative(mapping, zoom, keyboardTarget),
                 "keyboard source opens native zoom from one hundred percent (IPC, no key injection)")) return 1;
    if (!require(move(1.35), "pinch resumes after native increment")) return 1;
    if (!require(reset() && mapping.get()->currentSource == static_cast<LONG>(bridge::InputSource::Reset), "reset reaches 100 percent through the native UI with its own source")) return 1;
    if (!require(restore.restore(), "initial native zoom factor is restored and confirmed")) return 1;
    std::cout << checks << " live integration checks passed\n";
}
