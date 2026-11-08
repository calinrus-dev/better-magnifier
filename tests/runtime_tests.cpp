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
        runtime::HostTestAccess::tick(host, 348);
        runtime::HostTestAccess::tick(host, 380);
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
        bridge::Record record{};
        record.magnifierPid = static_cast<LONG>(GetCurrentProcessId()); record.magnifierReady = 1;
        record.enabled = 1; record.controlEnabled = 1; record.inputRevision = 70;
        runtime::Host host(pinch::Config{}); simulated = {2, 0, 0};
        runtime::HostTestAccess::prepare(host, record);
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
        require(!runtime::HostTestAccess::active(host) && !record.wheelPacket, "overview drops pending wheel input without retaining it");
        record.shellMask = 0;
        bridge::enqueueWheel(&record, 120, record.inputRevision, 1340);
        InterlockedIncrement(&record.inputRevision); runtime::HostTestAccess::tick(host, 1340);
        require(!runtime::HostTestAccess::active(host), "a keyboard revision rejects an old producer packet");
    }
    {
        bridge::Record record{};
        record.magnifierPid = static_cast<LONG>(GetCurrentProcessId()); record.magnifierReady = 1;
        record.nativeCaps = compatibility::NativeCore | compatibility::NativeWheel;
        record.gateCaps = 0; record.gateReady = 0; record.inputRevision = 90;
        runtime::Host host(pinch::Config{}); simulated = {2, 0, 0};
        runtime::HostTestAccess::prepare(host, record);
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
        runtime::Zoom zoom; simulated = {3, 0, 0};
        zoom.useTestBackend(readView, writeView, readCursor, true); zoom.useTestNativeRecord(&record);
        require(zoom.current() == 3, "test starts with a working native backend");
        record.magnifierReady = 0; record.resolverReasonNative = ERROR_NOT_SUPPORTED;
        require(!zoom.target(4, true) && !zoom.resetToNormal() && zoom.tick(16, 0) && simulated.zoom == 3,
                "backend loss or pause cannot write a public replacement transform over native Windows zoom");
    }
    {
        bridge::Record record{};
        record.magnifierPid = static_cast<LONG>(GetCurrentProcessId()); record.magnifierReady = 1; record.inputRevision = 123;
        runtime::Host host(pinch::Config{}); simulated = {2, 0, 0}; runtime::HostTestAccess::prepare(host, record);
        bridge::enqueueWheel(&record, 1, 123, 1001);
        runtime::HostTestAccess::tick(host, 1000);
        require(runtime::HostTestAccess::active(host) && !record.wheelPacket,
                "a fresh producer timestamp one millisecond after the sampled frame is not discarded");
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
