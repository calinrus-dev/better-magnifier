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
bool queryFails = false;
LONG queryFlags = WTS_SESSIONSTATE_UNLOCK;
BOOL WINAPI querySession(HANDLE, DWORD, WTS_INFO_CLASS, LPWSTR* data, DWORD* bytes) {
    static WTSINFOEXW information{}; information.Level = 1;
    information.Data.WTSInfoExLevel1.SessionState = WTSActive;
    information.Data.WTSInfoExLevel1.SessionFlags = queryFlags;
    if (queryFails) return FALSE;
    *data = reinterpret_cast<LPWSTR>(&information); *bytes = sizeof(information); return TRUE;
}
void WINAPI freeSession(PVOID) {}

}

namespace runtime {
struct HostTestAccess {
    static void prepare(Host& host, bridge::Record& record) {
        host.bridge_.useTestRecord(&record);
        host.activity_.snapshot(true, true); host.sessionActive_ = true;
        host.wtsRegistered_ = true; host.sessionSnapshot_ = true;
        host.querySession_ = querySession; host.freeSession_ = freeSession;
        host.zoom_.useTestBackend(readView, writeView, readCursor);
    }
    static void input(Host& host, int delta, LONG epoch, ULONGLONG now) { host.wheelInput(delta, epoch, now); }
    static void tick(Host& host, ULONGLONG now) { host.render(now); }
    static void cancel(Host& host) { host.cancelWheel(); }
    static bool active(Host& host) { return host.wheel_.active(); }
    static double goal(Host& host) { return host.wheel_.goal(); }
    static void resume(Host& host, ULONGLONG now) { host.activity_.resume(); host.applySessionState(); host.refreshSession(now, true); }
    static void refresh(Host& host, ULONGLONG now) { host.refreshSession(now); }
    static bool sessionActive(Host& host) { return host.sessionActive_; }
};
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
    {
        bridge::Record record{};
        record.magnifierPid = static_cast<LONG>(GetCurrentProcessId()); record.magnifierReady = 1;
        record.enabled = 1; record.controlEnabled = 1; record.inputRevision = 4;
        runtime::Host host(pinch::Config{});
        simulated = {2, 0, 0};
        runtime::HostTestAccess::prepare(host, record);
        runtime::HostTestAccess::input(host, 120, 4, 100);
        require(runtime::HostTestAccess::active(host) && record.wheelActive,
                "real host starts a wheel trajectory from a current revision");
        runtime::HostTestAccess::tick(host, 116);
        runtime::HostTestAccess::tick(host, 132);
        require(simulated.zoom > 2 && simulated.zoom < 2.3, "real host renders decimal wheel frames through its zoom controller");
        runtime::HostTestAccess::cancel(host);
        const LONG cancelledEpoch = record.inputRevision;
        runtime::HostTestAccess::input(host, 120, 4, 140);
        require(!runtime::HostTestAccess::active(host) && record.inputRevision == cancelledEpoch,
                "a wheel message queued before pinch or reset cannot restart cancelled motion");
        record.shellMask = 1;
        runtime::HostTestAccess::input(host, 120, cancelledEpoch, 150);
        require(!runtime::HostTestAccess::active(host), "Task View refuses queued wheel input");
        record.shellMask = 0;
        runtime::HostTestAccess::input(host, 120, record.inputRevision, 160);
        InterlockedIncrement(&record.inputRevision);
        runtime::HostTestAccess::tick(host, 176);
        require(!runtime::HostTestAccess::active(host) && !record.wheelActive,
                "an external keyboard revision cancels the host animation before it publishes another wheel frame");
        runtime::HostTestAccess::input(host, 120, record.inputRevision, 180);
        InterlockedIncrement(&record.inputRevision);
        const LONG newEpoch = record.inputRevision;
        runtime::HostTestAccess::input(host, 120, newEpoch, 182);
        const double firstGoal = runtime::HostTestAccess::goal(host);
        runtime::HostTestAccess::input(host, 120, newEpoch, 184);
        require(record.inputRevision == newEpoch && runtime::HostTestAccess::goal(host) > firstGoal,
                "several new wheel events after keyboard are not invalidated by cancelling old motion");
        runtime::HostTestAccess::cancel(host);
        runtime::HostTestAccess::input(host, 120, -1, 186);
        require(!runtime::HostTestAccess::active(host), "a queued wake-only event cannot zoom after READY arrives");
        runtime::HostTestAccess::input(host, -120, record.inputRevision, 190);
        runtime::HostTestAccess::tick(host, 300);
        runtime::HostTestAccess::tick(host, 316);
        require(!runtime::HostTestAccess::active(host) && !record.wheelActive,
                "host wheel trajectory stops once its endpoint is delivered");
    }
    {
        bridge::Record record{}; runtime::Host host(pinch::Config{});
        runtime::HostTestAccess::prepare(host, record);
        queryFails = true;
        runtime::HostTestAccess::resume(host, 1000);
        require(!runtime::HostTestAccess::sessionActive(host), "resume query failure preserves an inactive session");
        queryFails = false;
        runtime::HostTestAccess::refresh(host, 2100);
        require(runtime::HostTestAccess::sessionActive(host), "session query retries after resume even without a later unlock notification");
        queryFlags = -1;
        runtime::HostTestAccess::resume(host, 3000);
        require(!runtime::HostTestAccess::sessionActive(host), "unknown lock flags do not mark the session snapshot valid");
        queryFlags = WTS_SESSIONSTATE_UNLOCK;
        runtime::HostTestAccess::refresh(host, 4100);
        require(runtime::HostTestAccess::sessionActive(host), "unknown lock flags retain the session recovery retry");
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
