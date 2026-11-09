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
unsigned transformWrites = 0;
BOOL WINAPI writeView(float level, int x, int y) { ++transformWrites; simulated = {level, x, y}; return TRUE; }
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

std::vector<std::uintptr_t> windowList{1, 2, 3};
HWND foreground = reinterpret_cast<HWND>(1), requestedWindow = nullptr;
unsigned windowRequests = 0;
enum class ActivationMode { Immediate, Deferred, Reject };
ActivationMode activationMode = ActivationMode::Immediate;
BOOL WINAPI enumerateWindows(WNDENUMPROC, LPARAM parameter) {
    *reinterpret_cast<std::vector<std::uintptr_t>*>(parameter) = windowList; return TRUE;
}
HWND WINAPI foregroundWindow() { return foreground; }
HWND WINAPI rootWindow(HWND window, UINT) { return window; }
BOOL WINAPI validWindow(HWND window) {
    return std::find(windowList.begin(), windowList.end(), reinterpret_cast<std::uintptr_t>(window)) != windowList.end();
}
BOOL WINAPI iconicWindow(HWND) { return FALSE; }
BOOL WINAPI showWindow(HWND, int) { return TRUE; }
BOOL WINAPI activateWindow(HWND window) {
    ++windowRequests; requestedWindow = window;
    if (activationMode == ActivationMode::Reject) return FALSE;
    if (activationMode == ActivationMode::Immediate) foreground = window;
    return TRUE;
}
void resetWindows(ActivationMode mode = ActivationMode::Immediate) {
    windowList = {1, 2, 3}; foreground = reinterpret_cast<HWND>(1);
    requestedWindow = nullptr; windowRequests = 0; activationMode = mode;
}

}

namespace runtime {
struct WindowSwitcherTestAccess {
    static void fake() {
        WindowSwitcher::apiEnumerate = enumerateWindows; WindowSwitcher::apiForeground = foregroundWindow;
        WindowSwitcher::apiActivate = ::activateWindow; WindowSwitcher::apiAncestor = rootWindow;
        WindowSwitcher::apiValid = validWindow; WindowSwitcher::apiIconic = iconicWindow; WindowSwitcher::apiShow = showWindow;
    }
};
struct HostTestAccess {
    static void prepare(Host& host, bridge::Record& record) {
        host.bridge_.useTestRecord(&record);
        host.activity_.snapshot(true, true); host.sessionActive_ = true;
        host.wtsRegistered_ = true; host.sessionSnapshot_ = true;
        host.querySession_ = querySession; host.freeSession_ = freeSession;
        host.zoom_.useTestBackend(readView, writeView, readCursor);
    }
    static void nativeBackend(Host& host, bridge::Record& record) {
        host.zoom_.useTestBackend(readView, writeView, readCursor, true);
        host.zoom_.useTestNativeRecord(&record);
    }
    static void input(Host& host, int delta, LONG epoch, ULONGLONG now) { host.wheelInput(delta, epoch, now); }
    static void tick(Host& host, ULONGLONG now) { host.render(now); }
    static void windows(Host& host, WindowSwitcher& backend) { host.windowSwitcher_ = &backend; }
    static int windowPending(Host& host, ULONGLONG now) { return host.windowWheel_.pending(now); }
    static void cancel(Host& host) { host.cancelWheel(); }
    static bool active(Host& host) { return host.wheel_.active(); }
    static double goal(Host& host) { return host.wheel_.goal(); }
    static void freeSpin(Host& host, int rate) { host.config_.wheelFreeSpinMaxRate = rate; }
    static void resume(Host& host, ULONGLONG now) { host.activity_.resume(); host.applySessionState(); host.refreshSession(now, true); }
    static void refresh(Host& host, ULONGLONG now) { host.refreshSession(now); }
    static bool sessionActive(Host& host) { return host.sessionActive_; }
    static void frame(Host& host, HANDLE id, const std::vector<pinch::Contact>& contacts, ULONGLONG now) {
        auto& device = host.devices_[id]; device.valid = true;
        host.frame(id, device, contacts, now, 16);
    }
    static bool seedPending(Host& host, HANDLE id) { return host.devices_[id].coldPinchSeed.has_value(); }
    static void activeDevice(Host& host, HANDLE id) { host.activeDevice_ = id; }
    static void restart(Host& host) { host.restartGestureTracking(); }
    static void lock(Host& host) { host.activity_.lock(); host.applySessionState(); }
};
}
namespace {
// One fake host fixture keeps UI readiness, protocol flags and a live process
// coherent. Every transform remains in simulated; no host window is created.
struct HostFixture {
    bridge::Record record{};
    runtime::Host host{pinch::Config{}};
    explicit HostFixture(LONG revision, pinch::View initial = {2, 0, 0}) {
        simulated = initial; cursor = {200, 150};
        record.magnifierPid = static_cast<LONG>(GetCurrentProcessId());
        record.magnifierReady = 1;
        record.nativeCaps = compatibility::NativeCore | compatibility::NativeWheel;
        record.enabled = 1; record.controlEnabled = 1; record.inputRevision = revision;
        record.nativeLevel = static_cast<LONG>(std::lround(initial.zoom * 10000));
        runtime::HostTestAccess::prepare(host, record);
    }
};
}
namespace {
const HANDLE pinchDevice = reinterpret_cast<HANDLE>(0x11);
std::vector<pinch::Contact> pinchContacts(double scale) {
    return {{1, {50 - 15 * scale, 30}}, {2, {50, 30}}, {3, {50 + 15 * scale, 30}}};
}
void prepareColdPinch(HostFixture& fixture) {
    fixture.record.magnifierReady = 0;
    fixture.record.gateReady = 1; fixture.record.shellPid = static_cast<LONG>(GetCurrentProcessId());
    runtime::HostTestAccess::nativeBackend(fixture.host, fixture.record);
}
void beginColdPinch(HostFixture& fixture) {
    runtime::HostTestAccess::frame(fixture.host, pinchDevice, pinchContacts(1), 1000);
    runtime::HostTestAccess::frame(fixture.host, pinchDevice, pinchContacts(1.1), 1016);
}
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
        HostFixture fixture(4);
        auto& record = fixture.record; auto& host = fixture.host;
        runtime::HostTestAccess::input(host, 120, 4, 100);
        require(runtime::HostTestAccess::active(host) && record.wheelActive,
                "real host starts a wheel trajectory from a current revision");
        const double wheelGoal = runtime::HostTestAccess::goal(host);
        runtime::HostTestAccess::tick(host, 116);
        runtime::HostTestAccess::tick(host, 132);
        require(simulated.zoom > 2 && simulated.zoom < wheelGoal, "real host renders decimal wheel frames through its zoom controller");
        runtime::HostTestAccess::cancel(host);
        const LONG cancelledEpoch = record.inputRevision;
        runtime::HostTestAccess::input(host, 120, 4, 140);
        require(!runtime::HostTestAccess::active(host) && record.inputRevision == cancelledEpoch,
                "a wheel message queued before pinch or reset cannot restart cancelled motion");
        record.shellMask = 1;
        runtime::HostTestAccess::input(host, 120, cancelledEpoch, 150);
        require(runtime::HostTestAccess::active(host), "a fresh wheel request remains available while Task View is displayed");
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
        for (ULONGLONG now = 206; now <= 1000 && runtime::HostTestAccess::active(host); now += 16)
            runtime::HostTestAccess::tick(host, now);
        require(!runtime::HostTestAccess::active(host) && !record.wheelActive,
                "host wheel trajectory stops once its endpoint is delivered");
    }
    {
        HostFixture fixture(0);
        auto& host = fixture.host;
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
    {
        const auto profile = binary_profiles::profiles[0];
        std::vector<BYTE> image(profile.identity.imageSize);
        auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image.data());
        dos->e_magic = IMAGE_DOS_SIGNATURE; dos->e_lfanew = 0x80;
        auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(image.data() + 0x80);
        nt->Signature = IMAGE_NT_SIGNATURE; nt->FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
        nt->FileHeader.NumberOfSections = 2; nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
        nt->FileHeader.TimeDateStamp = profile.identity.timestamp;
        nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
        nt->OptionalHeader.SizeOfImage = profile.identity.imageSize;
        nt->OptionalHeader.SizeOfHeaders = 4096; nt->OptionalHeader.NumberOfRvaAndSizes = 16;
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG] = {0x1200, sizeof(IMAGE_DEBUG_DIRECTORY)};
        auto* sections = IMAGE_FIRST_SECTION(nt);
        sections[0].VirtualAddress = 0x2000; sections[0].Misc.VirtualSize = 0x90000;
        sections[0].Characteristics = IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
        sections[1].VirtualAddress = 0x100000; sections[1].Misc.VirtualSize = 0x10000;
        sections[1].Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
        auto* debug = reinterpret_cast<IMAGE_DEBUG_DIRECTORY*>(image.data() + 0x1200);
        debug->Type = IMAGE_DEBUG_TYPE_CODEVIEW; debug->SizeOfData = 24; debug->AddressOfRawData = 0x1300;
        auto* data = image.data() + 0x1300;
        data[0]='R'; data[1]='S'; data[2]='D'; data[3]='S';
        std::copy(profile.identity.guid.begin(), profile.identity.guid.end(), data + 4); data[20] = 1;
        const auto module = reinterpret_cast<HMODULE>(image.data());
        void* targets[12]{};
        require(binary_profiles::resolve(module, binary_profiles::Kind::Magnifier, targets, 12) &&
                targets[0] == image.data() + profile.rvas[0], "an exact verified image resolves its offline profile without symbols");
        data[4] ^= 1;
        targets[0] = nullptr;
        require(!binary_profiles::resolve(module, binary_profiles::Kind::Magnifier, targets, 12) && !targets[0],
                "a changed CodeView identity never reuses offsets from a previous Windows binary");
        data[4] ^= 1; data[20] = 2;
        require(!binary_profiles::resolve(module, binary_profiles::Kind::Magnifier, targets, 12), "changed PDB age invalidates the whole offline profile");
        data[20] = 1; nt->FileHeader.TimeDateStamp ^= 1;
        require(!binary_profiles::resolve(module, binary_profiles::Kind::Magnifier, targets, 12), "changed PE timestamp invalidates offline offsets");
        nt->FileHeader.TimeDateStamp ^= 1;
        sections[0].Characteristics = IMAGE_SCN_MEM_READ;
        require(!binary_profiles::resolve(module, binary_profiles::Kind::Magnifier, targets, 12), "a function RVA in nonexecutable memory cannot become a hook target");
        require(!binary_profiles::permitted(module, profile.identity.imageSize + 4, false), "profile RVA validation rejects out-of-image addresses");
        require(binary_profiles::permitted(module, 0x100000, true) && !binary_profiles::permitted(module, 0x100001, true),
                "data targets require writable aligned storage");
        debug->AddressOfRawData = profile.identity.imageSize - 8;
        require(!binary_profiles::identify(module), "truncated CodeView records cannot be read outside the image");
    }
    {
        HostFixture fixture(70);
        auto& record = fixture.record; auto& host = fixture.host;
        for (int i=0; i<10000; ++i) bridge::enqueueWheel(&record, 1, 70, 1000);
        require(!runtime::HostTestAccess::active(host), "producer cannot render before the host consumes its bounded mailbox");
        runtime::HostTestAccess::tick(host, 1000);
        require(record.wheelPacket == 0 && runtime::HostTestAccess::active(host) && runtime::HostTestAccess::goal(host) < 3,
                "host consumes a free-spin burst once with bounded lookahead");
        runtime::HostTestAccess::tick(host, 1016); runtime::HostTestAccess::tick(host, 1032);
        bridge::enqueueWheel(&record, -1, 70, 1032);
        runtime::HostTestAccess::tick(host, 1032);
        const double beforeReverse = simulated.zoom;
        runtime::HostTestAccess::tick(host, 1048); runtime::HostTestAccess::tick(host, 1064);
        require(simulated.zoom < beforeReverse, "tiny reversal after a large burst responds without old positive momentum");
        runtime::HostTestAccess::cancel(host);
        require(!record.wheelPacket && record.inputRevision != 70, "cancel invalidates and empties queued wheel data");
        bridge::enqueueWheel(&record, 120, record.inputRevision, 1100);
        runtime::HostTestAccess::tick(host, 1301);
        require(!runtime::HostTestAccess::active(host) && !record.wheelPacket,
                "input older than two hundred milliseconds cannot replay after a stall");
        bridge::enqueueWheel(&record, 120, record.inputRevision, 1320);
        record.shellMask = 1; runtime::HostTestAccess::tick(host, 1320);
        require(runtime::HostTestAccess::active(host) && !record.wheelPacket, "fresh wheel input in Task View uses the bounded mailbox instead of being discarded");
        record.shellMask = 0;
        bridge::enqueueWheel(&record, 120, record.inputRevision, 1340);
        InterlockedIncrement(&record.inputRevision); runtime::HostTestAccess::tick(host, 1340);
        require(!runtime::HostTestAccess::active(host), "a keyboard revision rejects an old producer packet");
    }
    {
        HostFixture fixture(90);
        auto& record = fixture.record; auto& host = fixture.host;
        record.gateCaps = 0; record.gateReady = 0;
        bridge::enqueueWheel(&record, 120, 90, 1000);
        runtime::HostTestAccess::tick(host, 1000); runtime::HostTestAccess::tick(host, 1016); runtime::HostTestAccess::tick(host, 1032);
        require(runtime::HostTestAccess::active(host) && simulated.zoom > 2 && !record.gateReady,
                "a missing shell gesture backend does not disable smooth mouse wheel zoom");
        runtime::HostTestAccess::cancel(host);
        record.magnifierReady = 0; record.nativeCaps = 0; record.resolverReasonNative = ERROR_NOT_SUPPORTED;
        const double beforeFallback = simulated.zoom;
        runtime::HostTestAccess::input(host, 120, record.inputRevision, 1100);
        require(!runtime::HostTestAccess::active(host) && simulated.zoom == beforeFallback,
                "an unverified native backend cannot receive a replacement transform from wheel input");
    }
    {
        bridge::Record record{};
        record.magnifierPid = static_cast<LONG>(GetCurrentProcessId()); record.magnifierReady = 1; record.nativeLevel = 30000;
        record.nativeCaps = compatibility::NativeCore;
        runtime::Zoom zoom; simulated = {3, 0, 0};
        zoom.useTestBackend(readView, writeView, readCursor, true); zoom.useTestNativeRecord(&record);
        require(zoom.current() == 3, "test starts with a working native backend");
        record.magnifierReady = 0; record.resolverReasonNative = ERROR_NOT_SUPPORTED;
        require(!zoom.target(4, true) && !zoom.resetToNormal() && zoom.tick(16, 0) && simulated.zoom == 3,
                "backend loss or pause cannot write a public replacement transform over native Windows zoom");
    }
    {
        HostFixture fixture(123);
        auto& record = fixture.record; auto& host = fixture.host;
        bridge::enqueueWheel(&record, 1, 123, 1001);
        runtime::HostTestAccess::tick(host, 1000);
        require(runtime::HostTestAccess::active(host) && !record.wheelPacket,
                "a fresh producer timestamp one millisecond after the sampled frame is not discarded");
    }
    {
        HostFixture fixture(160);
        auto& record = fixture.record; auto& host = fixture.host;
        record.shellMask = 2; // Showing desktop is telemetry, not Task View.
        runtime::HostTestAccess::input(host, 120, 160, 1000);
        runtime::HostTestAccess::tick(host, 1016); runtime::HostTestAccess::tick(host, 1032);
        require(runtime::HostTestAccess::active(host) && simulated.zoom > 2,
                "showing desktop keeps smooth wheel zoom available");
        record.shellMask = 3;
        InterlockedIncrement(&record.inputRevision); // Entering Task View invalidates motion captured before its native suspension.
        runtime::HostTestAccess::tick(host, 1048);
        require(!runtime::HostTestAccess::active(host) && !record.wheelActive,
                "entering Task View invalidates the previous wheel trajectory through its native epoch");
        runtime::HostTestAccess::input(host, 120, record.inputRevision, 1064);
        require(runtime::HostTestAccess::active(host), "explicit fresh wheel input can take over after Task View suspended the old trajectory");
    }
    {
        bridge::Record record{};
        record.magnifierPid = static_cast<LONG>(GetCurrentProcessId());
        record.magnifierReady = 1; record.nativeCaps = compatibility::NativeCore | compatibility::NativeWheel;
        record.nativeLevel = 10000; record.zoomRevision = 17;
        runtime::Zoom zoom; simulated = {1, 0, 0};
        zoom.useTestBackend(readView, writeView, readCursor, true); zoom.useTestNativeRecord(&record);
        const unsigned writesBefore = transformWrites;
        zoom.tick(0, 0);
        require(zoom.target(1.0005, true, bridge::InputSource::Wheel) && record.requestedLevel == 10005 && record.requestSource == static_cast<LONG>(bridge::InputSource::Wheel),
                "UI readiness at one hundred percent can enqueue the first fine wheel request before a renderer exists");
        const LONG wheelSequence = record.requestedSequence;
        require(zoom.target(1.25, true) && record.requestedSequence != wheelSequence && record.requestSource == static_cast<LONG>(bridge::InputSource::Pinch),
                "switching from wheel to pinch updates the native source instead of retaining the old wheel tag");
        require(zoom.resetToNormal() && record.pendingCommand == 3 && transformWrites == writesBefore && simulated.zoom == 1,
                "reset with a ready native UI stays on IPC and never writes a replacement fullscreen transform");
        record.magnifierReady = 0; record.nativeCaps = 0; record.resolverReasonNative = ERROR_NOT_SUPPORTED;
        const LONG unsupportedSequence = record.requestedSequence;
        require(!zoom.target(2, true) && !zoom.resetToNormal() && record.requestedSequence == unsupportedSequence && transformWrites == writesBefore,
                "unsupported native core neither enqueues private updates nor falls back to a second magnifier");
    }
    {
        bridge::Record record{};
        record.magnifierPid = static_cast<LONG>(GetCurrentProcessId());
        record.magnifierReady = 1; record.nativeCaps = compatibility::NativeCore;
        record.nativeLevel = 10000; record.overviewSavedLevel = 30000; record.shellMask = 1;
        runtime::Zoom zoom; simulated = {1, 0, 0};
        zoom.useTestBackend(readView, writeView, readCursor, true); zoom.useTestNativeRecord(&record);
        const unsigned writesBefore = transformWrites;
        require(zoom.current() == 3,
                "a pinch starting in Task View adopts the saved global zoom rather than its temporary one hundred percent display");
        record.shellMask = 0;
        require(zoom.current() == 3, "a dismiss signal cannot expose the temporary baseline before the UI restores its saved level");
        record.shellMask = 1;
        zoom.tick(0, 0);
        require(zoom.target(3.1, true, bridge::InputSource::Pinch) && record.requestedLevel == 31000 &&
                record.requestSource == static_cast<LONG>(bridge::InputSource::Pinch) && transformWrites == writesBefore,
                "an explicit Task View pinch requests its delta from the saved level through the unified native source");
        record.overviewSavedLevel = 0; record.nativeLevel = 31000; record.shellMask = 0;
        require(zoom.current() == 3.1, "leaving Task View returns the published global level without a stale saved baseline");
    }
    {
        HostFixture fixture(180, {1, 0, 0});
        auto& record = fixture.record; auto& host = fixture.host;
        record.shellMask = 1; record.nativeLevel = 10000; record.overviewSavedLevel = 30000;
        runtime::HostTestAccess::nativeBackend(host, record);
        const unsigned writesBefore = transformWrites;
        runtime::HostTestAccess::input(host, 120, 180, 1000);
        runtime::HostTestAccess::tick(host, 1016); runtime::HostTestAccess::tick(host, 1032);
        require(runtime::HostTestAccess::active(host) && record.requestedLevel > 30000 &&
                record.requestSource == static_cast<LONG>(bridge::InputSource::Wheel) && transformWrites == writesBefore,
                "fresh Task View wheel math starts from the saved native level and enqueues its typed source without a public transform");
    }
    {
        HostFixture fixture(200);
        auto& record = fixture.record; auto& host = fixture.host;
        record.magnifierReady = 0; // This test process stays alive, so no real Magnifier launch is attempted.
        runtime::HostTestAccess::input(host, 120, 200, 1000);
        require(!runtime::HostTestAccess::active(host), "cold wheel input waits for the native UI instead of using a public replacement magnifier");
        record.magnifierReady = 1;
        runtime::HostTestAccess::tick(host, 1016);
        require(runtime::HostTestAccess::active(host) && runtime::HostTestAccess::goal(host) > 2,
                "a bounded first wheel notch is delivered once the native UI becomes ready");
    }
    {
        HostFixture fixture(210);
        auto& record = fixture.record; auto& host = fixture.host;
        record.magnifierReady = 0;
        runtime::HostTestAccess::input(host, 120, 210, 1000);
        runtime::HostTestAccess::input(host, 1, 210, 3000); // No host frame ran while the old intent expired.
        record.magnifierReady = 1;
        runtime::HostTestAccess::tick(host, 3016);
        require(runtime::HostTestAccess::active(host) && runtime::HostTestAccess::goal(host) > 2 && runtime::HostTestAccess::goal(host) < 2.01,
                "fresh fine input after a cold-start stall cannot revive an expired full notch");
    }
    {
        HostFixture fixture(220);
        auto& record = fixture.record; auto& host = fixture.host;
        record.magnifierReady = 0;
        runtime::HostTestAccess::input(host, 120, 220, 1000);
        runtime::HostTestAccess::cancel(host); // A pinch, reset or session transition takes priority.
        record.magnifierReady = 1;
        runtime::HostTestAccess::tick(host, 1016);
        require(!runtime::HostTestAccess::active(host) && !record.wheelActive,
                "a cancelled cold wheel intent cannot reappear when the native UI becomes ready");
    }
    {
        HostFixture fixture(190, {1, 0, 0}); prepareColdPinch(fixture);
        auto& record = fixture.record; auto& host = fixture.host;
        const unsigned writesBefore = transformWrites;
        runtime::HostTestAccess::frame(host, pinchDevice, pinchContacts(1), 1000);
        runtime::HostTestAccess::frame(host, pinchDevice, pinchContacts(1.02), 1016);
        runtime::HostTestAccess::frame(host, pinchDevice, pinchContacts(1.04), 1032);
        require(runtime::HostTestAccess::seedPending(host, pinchDevice) && record.requestedSequence == 0,
                "a first cold pinch stores one intent without writing to an unavailable native backend");
        runtime::HostTestAccess::frame(host, pinchDevice, pinchContacts(1.08), 1048);
        runtime::HostTestAccess::frame(host, pinchDevice, pinchContacts(1.12), 1064);
        record.magnifierReady = 1; runtime::HostTestAccess::tick(host, 1100);
        const LONG seed = record.requestedLevel;
        require(record.requestedSequence == 1 && record.requestSource == static_cast<LONG>(bridge::InputSource::Pinch) &&
                std::abs(seed / 10000.0 - 1.04 / 1.02) < 0.0001 && !runtime::HostTestAccess::seedPending(host, pinchDevice) &&
                transformWrites == writesBefore,
                "READY timer delivers only the first precise cold intent even when fingers stay still after unrendered travel");
        runtime::HostTestAccess::tick(host, 1110);
        require(record.requestedSequence == 1, "an idle timer cannot resend the cold pinch seed");
        record.nativeLevel = seed; record.appliedSequence = record.requestedSequence; InterlockedIncrement(&record.zoomRevision);
        runtime::HostTestAccess::frame(host, pinchDevice, pinchContacts(1.13), 1116);
        require(record.requestedLevel > seed && record.requestedLevel < seed * 1.03,
                "the next live HID delta adopts the seed and does not replay the waiting gesture's total spread");
    }
    {
        HostFixture fixture(191, {1, 0, 0}); prepareColdPinch(fixture); beginColdPinch(fixture);
        fixture.record.magnifierReady = 1; runtime::HostTestAccess::tick(fixture.host, 1100);
        require(fixture.record.requestedLevel == 10300 && fixture.record.requestedSequence == 1,
                "a large first cold pinch is bounded to three percent rather than replayed at full travel");
    }
    {
        HostFixture fixture(192, {2, 0, 0}); prepareColdPinch(fixture);
        runtime::HostTestAccess::frame(fixture.host, pinchDevice, pinchContacts(1), 1000);
        runtime::HostTestAccess::frame(fixture.host, pinchDevice, pinchContacts(0.9), 1016);
        fixture.record.magnifierReady = 1; runtime::HostTestAccess::tick(fixture.host, 1100);
        require(std::abs(fixture.record.requestedLevel / 10000.0 - 2 / 1.03) < 0.0001,
                "a contracting cold pinch retains its direction and the same multiplicative precision bound");
    }
    {
        HostFixture fixture(193, {1, 0, 0}); prepareColdPinch(fixture); beginColdPinch(fixture);
        runtime::HostTestAccess::frame(fixture.host, pinchDevice, {}, 1032);
        fixture.record.magnifierReady = 1; runtime::HostTestAccess::tick(fixture.host, 1100);
        require(!runtime::HostTestAccess::seedPending(fixture.host, pinchDevice) && fixture.record.requestedSequence == 0,
                "lifting the fingers before READY clears the cold intent without zooming later");
    }
    {
        HostFixture fixture(194, {1, 0, 0}); prepareColdPinch(fixture); beginColdPinch(fixture);
        runtime::HostTestAccess::cancel(fixture.host);
        fixture.record.magnifierReady = 1; runtime::HostTestAccess::tick(fixture.host, 1100);
        require(!runtime::HostTestAccess::seedPending(fixture.host, pinchDevice) && fixture.record.requestedSequence == 0,
                "a reset or another input that invalidates wheel ownership also clears pending pinch intent");
    }
    {
        HostFixture fixture(195, {1, 0, 0}); prepareColdPinch(fixture); beginColdPinch(fixture);
        runtime::HostTestAccess::restart(fixture.host);
        fixture.record.magnifierReady = 1;
        runtime::HostTestAccess::frame(fixture.host, pinchDevice, pinchContacts(1.1), 1100);
        runtime::HostTestAccess::tick(fixture.host, 1116);
        require(!runtime::HostTestAccess::seedPending(fixture.host, pinchDevice) && fixture.record.requestedSequence == 0,
                "an exit/reset waits for a genuine finger release and cannot resurrect a cold pinch after Win Esc");
    }
    {
        HostFixture fixture(196, {1, 0, 0}); prepareColdPinch(fixture); beginColdPinch(fixture);
        InterlockedIncrement(&fixture.record.nativeCommands); fixture.record.magnifierReady = 1;
        runtime::HostTestAccess::tick(fixture.host, 1100);
        require(!runtime::HostTestAccess::seedPending(fixture.host, pinchDevice) && fixture.record.requestedSequence == 0,
                "a newer native keyboard command wins over a cold pinch intent");
    }
    {
        HostFixture fixture(197, {1, 0, 0}); prepareColdPinch(fixture); beginColdPinch(fixture);
        fixture.record.magnifierReady = 1; runtime::HostTestAccess::tick(fixture.host, 2517);
        require(!runtime::HostTestAccess::seedPending(fixture.host, pinchDevice) && fixture.record.requestedSequence == 0,
                "cold pinch intent expires after fifteen hundred milliseconds");
    }
    {
        HostFixture fixture(198, {1, 0, 0}); prepareColdPinch(fixture); beginColdPinch(fixture);
        runtime::HostTestAccess::activeDevice(fixture.host, reinterpret_cast<HANDLE>(0x22));
        fixture.record.magnifierReady = 1; runtime::HostTestAccess::tick(fixture.host, 1100);
        require(!runtime::HostTestAccess::seedPending(fixture.host, pinchDevice) && fixture.record.requestedSequence == 0,
                "a cold intent cannot migrate from its original active device to a different touchpad");
    }
    {
        HostFixture fixture(199, {1, 0, 0}); prepareColdPinch(fixture); beginColdPinch(fixture);
        runtime::HostTestAccess::lock(fixture.host);
        fixture.record.magnifierReady = 1; runtime::HostTestAccess::tick(fixture.host, 1100);
        require(!runtime::HostTestAccess::seedPending(fixture.host, pinchDevice) && fixture.record.requestedSequence == 0,
                "locking the session cancels cold pinch intent before any native view can appear");
    }
    {
        HostFixture fixture(200, {1, 0, 0}); prepareColdPinch(fixture); beginColdPinch(fixture);
        fixture.record.nativeViewMode = 3; runtime::HostTestAccess::tick(fixture.host, 1050);
        fixture.record.nativeViewMode = 2; fixture.record.magnifierReady = 1;
        runtime::HostTestAccess::tick(fixture.host, 1100);
        require(!runtime::HostTestAccess::seedPending(fixture.host, pinchDevice) && fixture.record.requestedSequence == 0,
                "a lens-mode transition cannot carry an old cold intent into a later fullscreen view");
    }
    // Fake foreground endpoints exercise asynchronous GUI activation without
    // enumerating, focusing, restoring or sending input to desktop windows.
    runtime::WindowSwitcherTestAccess::fake();
    {
        resetWindows(); runtime::WindowSwitcher backend;
        require(backend.request(-1, 1000).state == runtime::WindowActivation::Confirmed && foreground == reinterpret_cast<HWND>(2), "window next confirms B");
        windowList = {2, 1, 3};
        require(backend.request(-1, 1016).state == runtime::WindowActivation::Confirmed && foreground == reinterpret_cast<HWND>(3), "window next continues C despite MRU reordering");
        windowList = {3, 2, 1};
        backend.request(-1, 1032);
        require(foreground == reinterpret_cast<HWND>(1), "three window forward cycle wraps A");
        backend.request(1, 1048); require(foreground == reinterpret_cast<HWND>(3), "reverse immediately selects previous C");
        backend.request(1, 1064); require(foreground == reinterpret_cast<HWND>(2), "reverse continues B rather than bouncing two apps");
    }
    {
        resetWindows(ActivationMode::Reject); runtime::WindowSwitcher backend;
        require(backend.request(-1, 1000).state == runtime::WindowActivation::Failed, "rejected activation reports failure");
        windowList = {1, 3, 2}; activationMode = ActivationMode::Immediate;
        require(backend.request(-1, 1032).state == runtime::WindowActivation::Confirmed && foreground == reinterpret_cast<HWND>(2), "failed focus never advances cycle or skips candidate");
    }
    {
        resetWindows(ActivationMode::Deferred); runtime::WindowSwitcher backend;
        const auto first = backend.request(-1, 1000);
        require(first.state == runtime::WindowActivation::Pending && foreground == reinterpret_cast<HWND>(1), "accepted asynchronous activation waits for real foreground");
        require(backend.request(-1, 1016).state == runtime::WindowActivation::Busy && windowRequests == 1, "pending focus prevents another activation dispatch");
        foreground = requestedWindow;
        require(backend.status(first.ticket, 1032) == runtime::WindowActivation::Confirmed, "later foreground confirmation commits pending candidate");
        backend.request(-1, 1048); require(requestedWindow == reinterpret_cast<HWND>(3), "confirmed deferred switch proceeds to third window");
    }
    {
        resetWindows(ActivationMode::Deferred); runtime::WindowSwitcher backend;
        const auto first = backend.request(-1, 1000);
        require(backend.status(first.ticket, 1150) == runtime::WindowActivation::Failed, "unconfirmed activation expires after bounded wait");
        backend.request(-1, 1151); require(requestedWindow == reinterpret_cast<HWND>(2), "timeout retries same window without advancing cycle");
    }
    {
        resetWindows(); runtime::WindowSwitcher backend; HostFixture fixture(210);
        runtime::HostTestAccess::windows(fixture.host, backend);
        const auto writes = transformWrites;
        bridge::enqueueWindowWheel(&fixture.record, -240, 1000);
        runtime::HostTestAccess::tick(fixture.host, 1000);
        require(windowRequests == 1 && foreground == reinterpret_cast<HWND>(2) && runtime::HostTestAccess::windowPending(fixture.host, 1000) == 1, "aggregated first notch dispatches one window per frame");
        runtime::HostTestAccess::tick(fixture.host, 1016);
        require(windowRequests == 2 && foreground == reinterpret_cast<HWND>(3) && !runtime::HostTestAccess::windowPending(fixture.host, 1016), "second aggregated notch dispatches without another wheel event");
        require(transformWrites == writes, "window cycling does not write zoom transforms");
    }
    {
        resetWindows(); runtime::WindowSwitcher backend; HostFixture fixture(211);
        runtime::HostTestAccess::windows(fixture.host, backend);
        bridge::enqueueWindowWheel(&fixture.record, -360, 2000); runtime::HostTestAccess::tick(fixture.host, 2000);
        bridge::enqueueWindowWheel(&fixture.record, 120, 2016); runtime::HostTestAccess::tick(fixture.host, 2016);
        require(windowRequests == 2 && foreground == reinterpret_cast<HWND>(1) && !runtime::HostTestAccess::windowPending(fixture.host, 2016), "direction reversal drops older queued steps and switches back immediately");
    }
    {
        resetWindows(ActivationMode::Deferred); runtime::WindowSwitcher backend; HostFixture fixture(212);
        runtime::HostTestAccess::windows(fixture.host, backend);
        bridge::enqueueWindowWheel(&fixture.record, -240, 3000); runtime::HostTestAccess::tick(fixture.host, 3000);
        runtime::HostTestAccess::tick(fixture.host, 3016);
        require(windowRequests == 1 && runtime::HostTestAccess::windowPending(fixture.host, 3016) == 2, "host retains all steps while foreground activation is pending");
        foreground = requestedWindow; runtime::HostTestAccess::tick(fixture.host, 3032);
        require(windowRequests == 2 && requestedWindow == reinterpret_cast<HWND>(3) && runtime::HostTestAccess::windowPending(fixture.host, 3032) == 1, "host confirms first then dispatches second asynchronous step exactly once");
        foreground = requestedWindow; runtime::HostTestAccess::tick(fixture.host, 3048);
        require(!runtime::HostTestAccess::windowPending(fixture.host, 3048) && windowRequests == 2, "second foreground acknowledgement drains two-notch queue");
    }
    {
        HostFixture fixture(210, {1, 0, 0}); auto& record = fixture.record; auto& host = fixture.host;
        runtime::HostTestAccess::nativeBackend(host, record);
        const unsigned writesBefore = transformWrites;
        bridge::enqueueWheel(&record, 120, 210, 1000); runtime::HostTestAccess::tick(host, 1016);
        require(record.requestedSequence == 1 && record.requestedLevel > 10000 && record.requestedLevel <= 10261 &&
                record.requestSource == static_cast<LONG>(bridge::InputSource::Wheel) && transformWrites == writesBefore,
                "the first wheel event waiting one frame sends positive native progress during that frame instead of an empty 100 percent objective");
    }
    {
        HostFixture fixture(211, {1, 0, 0}); auto& record = fixture.record;
        runtime::HostTestAccess::nativeBackend(fixture.host, record);
        bridge::enqueueWheel(&record, 120, 211, 1000); runtime::HostTestAccess::tick(fixture.host, 1100);
        require(record.requestedLevel > 10000 && record.requestedLevel <= 10261,
                "initial wheel scheduling credits at most sixteen milliseconds instead of replaying a stale mailbox delay");
    }
    {
        HostFixture fixture(212, {1, 0, 0}); auto& record = fixture.record;
        runtime::HostTestAccess::nativeBackend(fixture.host, record);
        bridge::enqueueWheel(&record, 120, 212, 1000); runtime::HostTestAccess::tick(fixture.host, 1000);
        require(record.requestedSequence == 0 && runtime::HostTestAccess::active(fixture.host),
                "an event captured in the current millisecond does not invent elapsed time or send an unchanged baseline");
        runtime::HostTestAccess::tick(fixture.host, 1016);
        require(record.requestedSequence == 1 && record.requestedLevel > 10000,
                "a same-millisecond wheel event progresses on the next actual frame");
    }
    {
        HostFixture fixture(213, {1, 0, 0}); auto& record = fixture.record;
        runtime::HostTestAccess::nativeBackend(fixture.host, record);
        bridge::enqueueWheel(&record, 120, 213, 1001); runtime::HostTestAccess::tick(fixture.host, 1000);
        require(record.requestedSequence == 0 && runtime::HostTestAccess::active(fixture.host),
                "a concurrent future capture is retained without manufacturing an early first target");
        runtime::HostTestAccess::tick(fixture.host, 1016);
        require(record.requestedSequence == 1 && record.requestedLevel > 10000,
                "a retained concurrent capture advances at the next actual frame");
    }
    {
        HostFixture fixture(214, {1, 0, 0}); auto& record = fixture.record;
        runtime::HostTestAccess::nativeBackend(fixture.host, record);
        bridge::enqueueWheel(&record, 1, 214, 1000); runtime::HostTestAccess::tick(fixture.host, 1016);
        require(record.requestedLevel > 10000 && record.requestedLevel < 10100,
                "first high-resolution delta keeps sub-percent native precision without a forced one percent kick");
        for (unsigned time=1032; time<=1144; time+=16) runtime::HostTestAccess::tick(fixture.host, time);
        const auto expected = std::llround(std::pow(1 + pinch::Config{}.wheelStep / 100.0, 1.0 / 120) * 10000);
        require(record.requestedLevel == expected && !runtime::HostTestAccess::active(fixture.host),
                "captured-time scheduling retains the exact fractional wheel goal and finite endpoint");
    }
    {
        HostFixture fixture(215, {1, 0, 0}); auto& record = fixture.record;
        runtime::HostTestAccess::nativeBackend(fixture.host, record);
        bridge::enqueueWheel(&record, 120, 215, 1000); runtime::HostTestAccess::tick(fixture.host, 1016);
        bridge::enqueueWheel(&record, 120, 215, 1020); runtime::HostTestAccess::tick(fixture.host, 1048);
        const pinch::Config config{}; pinch::WheelMotion expected;
        expected.add(120, 1, 1000, config.wheelStep, config.wheelDuration, 16, config.wheelMaxRate); expected.tick(1016);
        expected.add(120, 1, 1048, config.wheelStep, config.wheelDuration, 16, config.wheelMaxRate);
        require(record.requestedLevel == std::llround(*expected.tick(1048) * 10000),
                "additional active wheel input keeps the processing clock and cannot rewind or count event residence twice");
    }
    {
        HostFixture fixture(216, {1, 0, 0}); auto& record = fixture.record;
        runtime::HostTestAccess::nativeBackend(fixture.host, record);
        bridge::enqueueWheel(&record, 120, 216, 1000); runtime::HostTestAccess::tick(fixture.host, 1016);
        const LONG forward = record.requestedLevel; record.nativeLevel = forward;
        bridge::enqueueWheel(&record, -1, 216, 1020); runtime::HostTestAccess::tick(fixture.host, 1032);
        require(record.requestedLevel <= forward, "captured scheduling cannot deliver forward momentum after reversing a wheel gesture");
        runtime::HostTestAccess::tick(fixture.host, 1048);
        require(record.requestedLevel < forward, "a tiny reverse wheel delta still responds on its first advancing frame");
    }
    {
        HostFixture fixture(217, {1, 0, 0}); auto& record = fixture.record;
        runtime::HostTestAccess::nativeBackend(fixture.host, record);
        bridge::enqueueWheel(&record, 120, 217, 1000); runtime::HostTestAccess::tick(fixture.host, 1016);
        InterlockedIncrement(&record.inputRevision); InterlockedIncrement(&record.zoomRevision); record.nativeLevel = 20000;
        bridge::enqueueWheel(&record, 1, record.inputRevision, 1050); runtime::HostTestAccess::tick(fixture.host, 1066);
        require(record.requestedLevel > 20000 && record.requestedLevel < 20200,
                "a fresh wheel trajectory after a keyboard epoch adopts its new base and captures only the new event age");
    }
    {
        HostFixture fixture(230,{1,0,0}); auto& record=fixture.record;
        runtime::HostTestAccess::nativeBackend(fixture.host,record);
        bridge::enqueueWheel(&record,1200,230,1000); runtime::HostTestAccess::tick(fixture.host,1016);
        require(record.requestedLevel>10261 && record.requestedLevel<=10339 &&
                record.requestSource==static_cast<LONG>(bridge::InputSource::Wheel),
                "the host opts into the explicit fast cap while retaining a typed native wheel request");
        runtime::HostTestAccess::tick(fixture.host,1032); runtime::HostTestAccess::tick(fixture.host,1048);
        runtime::HostTestAccess::tick(fixture.host,1064);
        require(!runtime::HostTestAccess::active(fixture.host) && !record.wheelActive,
                "a free-spin batch ends without extrapolation within forty milliseconds plus one frame");
    }
    {
        HostFixture fixture(231,{1,0,0}); auto& record=fixture.record;
        runtime::HostTestAccess::nativeBackend(fixture.host,record);
        runtime::HostTestAccess::freeSpin(fixture.host,0);
        bridge::enqueueWheel(&record,1200,231,1000); runtime::HostTestAccess::tick(fixture.host,1016);
        require(record.requestedLevel>10000 && record.requestedLevel<=10261,
                "opting out of burst adaptation retains the configured normal rate cap");
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
