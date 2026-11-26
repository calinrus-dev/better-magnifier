// Exercise the actual broker callbacks with fake Win32 endpoints.
// No thread is started, no hook is installed, and SendInput never reaches Windows.
#define PINCH_ZOOM_STANDALONE
#define PINCH_ZOOM_RUNTIME_TEST
#define wmain diagnostic_main
#include "../mods/kde-pinch-zoom.wh.cpp"
#undef wmain
#include <iostream>

namespace {
constexpr LRESULT kPassed = 17;
std::array<bool, 256> keys{};
struct SendCall { std::vector<INPUT> inputs; };
struct Posted { HWND target; UINT message; WPARAM w; LPARAM l; };
struct ThreadPost { UINT message; WPARAM w; LPARAM l; };
std::vector<SendCall> sends;
std::vector<UINT> sendResults;
std::size_t nextSendResult = 0;
std::vector<Posted> posts;
std::vector<ThreadPost> threadPosts;
unsigned passed = 0, keyReads = 0;
unsigned wheelWakes = 0;
struct ReleaseMask { BYTE key; BYTE scan; DWORD flags; ULONG_PTR tag; unsigned passedBefore; };
std::vector<ReleaseMask> releaseMasks;
void WINAPI keyEvent(BYTE key, BYTE scan, DWORD flags, ULONG_PTR tag) {
    releaseMasks.push_back({key, scan, flags, tag, passed});
}
bool postSuccess = true;
bool postThreadSuccess = true;
bool timerAvailable = true;
unsigned timers = 0, cancelledTimers = 0;
UINT_PTR WINAPI setTimer(HWND, UINT_PTR, UINT, TIMERPROC) {
    return timerAvailable ? ++timers : 0;
}
BOOL WINAPI killTimer(HWND, UINT_PTR) { ++cancelledTimers; return TRUE; }
SHORT WINAPI keyState(int key) { ++keyReads; return key >= 0 && key < 256 && keys[static_cast<unsigned>(key)] ? static_cast<SHORT>(0x8000) : 0; }
UINT WINAPI sendInput(UINT count, LPINPUT input, int size) {
    if (size != sizeof(INPUT)) return 0;
    sends.push_back({std::vector<INPUT>(input, input + count)});
    return nextSendResult < sendResults.size() ? sendResults[nextSendResult++] : count;
}
BOOL WINAPI post(HWND target, UINT message, WPARAM w, LPARAM l) {
    if (!postSuccess) return FALSE;
    if (message == runtime::kWheelMessage) { ++wheelWakes; return TRUE; }
    posts.push_back({target, message, w, l}); return TRUE;
}
BOOL WINAPI postThread(DWORD, UINT message, WPARAM w, LPARAM l) {
    if (!postThreadSuccess) return FALSE;
    threadPosts.push_back({message, w, l});
    return TRUE;
}
BOOL WINAPI cursorPosition(LPPOINT point) { *point = {}; return TRUE; }
LRESULT WINAPI next(HHOOK, int, WPARAM, LPARAM) { ++passed; return kPassed; }
}

namespace runtime {
struct InputBrokerTestAccess {
    static void attach(MouseInputBroker& broker, bridge::Record& record) {
        broker.state_ = &record; broker.target_ = reinterpret_cast<HWND>(0x1234);
        MouseInputBroker::owner_ = &broker;
        MouseInputBroker::apiReadKey = keyState;
        MouseInputBroker::apiSendInput = sendInput;
        MouseInputBroker::apiKeyEvent = keyEvent;
        MouseInputBroker::apiPost = post;
        MouseInputBroker::apiPostThread = postThread;
        MouseInputBroker::apiCursor = cursorPosition;
        MouseInputBroker::apiNext = next;
        MouseInputBroker::apiSetTimer = setTimer;
        MouseInputBroker::apiKillTimer = killTimer;
    }
    static void flushResync(MouseInputBroker& broker) { broker.consumeResync(); }
    static void expireChord(MouseInputBroker& broker) { broker.replayButton(); }
    static void replaySideScroll(MouseInputBroker& broker, const ThreadPost& posted, DWORD age = 0) {
        if (posted.message == MouseInputBroker::kSideScrollMessage)
            broker.replaySideScroll(static_cast<SHORT>(posted.w), static_cast<unsigned>(posted.l), 100, 100 + age);
    }
    static bool runWithControllerAlreadyStopping(MouseInputBroker& broker) {
        broker.ready_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        broker.stop_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        broker.controllerStop_ = CreateEventW(nullptr, TRUE, TRUE, nullptr);
        const bool eventsReady = broker.ready_ && broker.stop_ && broker.controllerStop_;
        if (eventsReady) MouseInputBroker::run(&broker);
        if (broker.controllerStop_) CloseHandle(std::exchange(broker.controllerStop_, nullptr));
        broker.stop();
        return eventsReady && !broker.installed_.load() && !broker.pendingButton_;
    }
    static void detach(MouseInputBroker& broker) { if (MouseInputBroker::owner_ == &broker) MouseInputBroker::owner_ = nullptr; }
    static LRESULT key(unsigned vk, bool down, bool injected = false, int code = HC_ACTION) {
        KBDLLHOOKSTRUCT event{}; event.vkCode = vk;
        event.flags = injected ? LLKHF_INJECTED : 0;
        if (!down) event.flags |= LLKHF_UP;
        const auto result = MouseInputBroker::keyboard(code, down ? WM_KEYDOWN : WM_KEYUP, reinterpret_cast<LPARAM>(&event));
        if (code == HC_ACTION && vk < keys.size()) keys[vk] = down;
        return result;
    }
    static LRESULT mouse(UINT message, int delta = 0, bool injected = false, int code = HC_ACTION,
                         DWORD time = 100, POINT point = {}, ULONG_PTR extraInfo = 0) {
        MSLLHOOKSTRUCT event{};
        event.mouseData = static_cast<DWORD>(static_cast<std::uint16_t>(delta)) << 16;
        event.flags = injected ? LLMHF_INJECTED : 0;
        event.dwExtraInfo = extraInfo;
        event.time = time;
        event.pt = point;
        return MouseInputBroker::mouse(code, message, reinterpret_cast<LPARAM>(&event));
    }
};
}

namespace {
struct BrokerFixture {
    bridge::Record record{};
    runtime::MouseInputBroker broker;
    pinch::Config config;
    BrokerFixture() {
        keys.fill(false); releaseMasks.clear(); sends.clear(); sendResults.clear(); nextSendResult = 0; posts.clear(); threadPosts.clear();
        passed = 0; postSuccess = postThreadSuccess = true;
        timerAvailable = true; timers = cancelledTimers = wheelWakes = 0;
        record.nativeCaps = compatibility::NativeCore | compatibility::NativeWheel;
        record.magnifierReady = 1; record.nativeViewMode = 2;
        record.hostWindow = 0x1234; record.controlEnabled = 1; record.wheelEnabled = 1; record.inputRevision = 17;
        config.winWheel = true; config.wheelWindowSwitch = true; config.windowButtons = false;
        runtime::InputBrokerTestAccess::attach(broker, record); broker.configure(config, true);
        runtime::InputBrokerTestAccess::flushResync(broker); keyReads = 0;
    }
    ~BrokerFixture() { runtime::InputBrokerTestAccess::detach(broker); }
    void configure(bool interactive = true) { broker.configure(config, interactive); }
    LRESULT key(unsigned vk, bool down, bool injected = false) { return runtime::InputBrokerTestAccess::key(vk, down, injected); }
    LRESULT mouse(UINT message, int delta = 0, bool injected = false) { return runtime::InputBrokerTestAccess::mouse(message, delta, injected); }
    LRESULT pointer(UINT message, DWORD time, POINT point = {}) {
        return runtime::InputBrokerTestAccess::mouse(message, 0, false, HC_ACTION, time, point);
    }
    void ctrlAlt() { key(VK_CONTROL, true); key(VK_MENU, true); }
    int wheel() const { return pinch::wheelDelta(static_cast<std::uint64_t>(record.wheelPacket)); }
    int windows() const { return pinch::wheelDelta(static_cast<std::uint64_t>(record.windowWheelPacket)); }
};
bool neutralMask(const SendCall& call) {
    if (call.inputs.size() != 2) return false;
    const auto& down = call.inputs[0]; const auto& up = call.inputs[1];
    return down.type == INPUT_KEYBOARD && up.type == INPUT_KEYBOARD && down.ki.wVk == 0xe8 && up.ki.wVk == 0xe8 &&
           down.ki.dwFlags == 0 && up.ki.dwFlags == KEYEVENTF_KEYUP && down.ki.dwExtraInfo != 0 && down.ki.dwExtraInfo == up.ki.dwExtraInfo;
}
}

int wmain() {
    unsigned checks = 0;
    const auto require = [&](bool valid, const char* label) {
        if (!valid) { std::cerr << "FAIL " << label << '\n'; std::exit(1); }
        ++checks;
    };
    {
        BrokerFixture f;
        require(f.key(VK_LWIN, true) == kPassed, "Win down remains native");
        require(f.mouse(WM_MOUSEWHEEL, 1) == 1 && f.wheel() == 1 && f.record.wheelEvents == 1 && wheelWakes == 1,
                "first Win wheel captures one precise unit and wakes the host immediately");
        require(sends.size() == 1 && neutralMask(sends[0]), "a consumed first Win mouse action sends only a tagged neutral down/up pair");
        require(f.mouse(WM_MOUSEWHEEL, 30) == 1 && f.wheel() == 31 && sends.size() == 1,
                "later high-resolution deltas do not repeatedly inject a menu mask");
        require(f.key(VK_LWIN, false) == kPassed, "Win up remains native after mouse zoom");
        f.key(VK_RWIN, true);
        require(f.mouse(WM_MOUSEWHEEL, -1) == 1 && f.wheel() == 30 && sends.size() == 2,
                "RWin opens an independent modifier session while wheel units combine with their signs");
    }
    for (unsigned key : {static_cast<unsigned>(0x53), static_cast<unsigned>(VK_SPACE), static_cast<unsigned>(VK_ADD), static_cast<unsigned>(VK_OEM_PLUS)}) {
        BrokerFixture f;
        f.key(VK_LWIN, true);
        require(f.key(key, true) == kPassed && f.mouse(WM_MOUSEWHEEL, 120) == kPassed && f.wheel() == 0,
                "Win keyboard shortcuts pass through and are not simultaneously treated as mouse-only zoom");
        require(f.key(key, false) == kPassed && f.mouse(WM_MOUSEWHEEL, 1) == 1 && f.wheel() == 1 && sends.empty(),
                "wheel works after releasing the shortcut key while Win remains held, without an extra mask");
        require(f.key(VK_LWIN, false) == kPassed, "native Win shortcut release is never swallowed");
    }
    {
        BrokerFixture f;
        require(f.key(VK_LWIN, true) == kPassed && f.key(VK_LWIN, false) == kPassed && sends.empty() && !f.wheel(),
                "an ordinary Win press creates no synthetic input and can still open Start");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true);
        require(f.mouse(WM_MBUTTONDOWN) == 1 && posts.size() == 1 && posts[0].message == runtime::kMouseActionMessage && posts[0].w == 1,
                "Win middle click posts one reset action");
        require(f.mouse(WM_MBUTTONDOWN) == 1 && posts.size() == 1, "duplicate down cannot post a second action during one owned click");
        f.key(VK_LWIN, false);
        require(f.mouse(WM_MBUTTONUP) == 1 && f.mouse(WM_MBUTTONUP) == kPassed,
                "the captured middle up remains owned after Win is released, exactly once");
    }
    {
        BrokerFixture f;
        require(f.mouse(WM_MBUTTONDOWN) == kPassed, "a normal middle down remains native");
        f.key(VK_LWIN, true);
        require(f.mouse(WM_MBUTTONDOWN) == kPassed && f.mouse(WM_MBUTTONUP) == kPassed && posts.empty() && sends.empty(),
                "pressing Win after a normal down cannot steal a duplicate down or leave its native button stuck");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true); f.mouse(WM_MBUTTONDOWN); f.configure(false);
        require(f.mouse(WM_MBUTTONUP) == 1 && posts.size() == 1, "pausing mid-click preserves the matching owned up without posting another action");
    }
    {
        BrokerFixture f;
        f.key(VK_LWIN, true, true);
        require(f.mouse(WM_MOUSEWHEEL, 120) == kPassed && sends.empty() && !f.wheel(), "injected Win input does not create a physical mouse shortcut");
        f.key(VK_LWIN, true);
        require(f.mouse(WM_MOUSEWHEEL, 120, true) == kPassed && sends.empty() && !f.wheel(), "injected mouse events are never recaptured");
        require(runtime::InputBrokerTestAccess::mouse(WM_MOUSEWHEEL, 120, false, -1) == kPassed && sends.empty(), "negative hook codes pass through without processing input");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true); sendResults = {0};
        require(f.mouse(WM_MOUSEWHEEL, 120) == kPassed && !f.wheel() && posts.empty() && f.record.menuMaskInitialFailed == 1, "a rejected mask leaves wheel input to Windows rather than consuming a broken shortcut");
        require(f.mouse(WM_MOUSEWHEEL, 1) == 1 && f.wheel() == 1 && sends.size() == 2, "failed menu masking can recover on a later physical event");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true); sendResults = {1, 1};
        require(f.mouse(WM_MOUSEWHEEL, 120) == kPassed && !f.wheel(), "partial menu-mask insertion does not enqueue zoom");
        require(sends.size() == 2 && neutralMask(sends[0]) && sends[1].inputs.size() == 1 &&
                sends[1].inputs[0].ki.wVk == 0xe8 && sends[1].inputs[0].ki.dwFlags == KEYEVENTF_KEYUP,
                "partial insertion cleans up only the neutral key and never synthesizes a Win modifier");
    }
    {
        BrokerFixture f; f.record.nativeCaps = 0; f.record.resolverReasonNative = ERROR_NOT_SUPPORTED; f.configure(); f.key(VK_LWIN, true);
        require(f.mouse(WM_MOUSEWHEEL, 120) == kPassed && f.mouse(WM_MBUTTONDOWN) == kPassed && !f.wheel() && sends.empty() && posts.empty(),
                "an unsupported native core cannot consume Win zoom or reset actions");
    }
    for (LONG mode : {1L, 3L}) {
        BrokerFixture f; f.record.nativeViewMode = mode; f.record.nativeError = 0; f.configure(); f.key(VK_LWIN, true);
        require(f.mouse(WM_MOUSEWHEEL, 120) == kPassed && sends.empty() && !f.wheel(), "Lens and Docked modes preserve Win mouse input even before an error flag catches up");
        f.key(VK_LWIN, false); f.ctrlAlt();
        require(f.mouse(WM_MOUSEWHEEL, 120) == kPassed && !f.windows(), "Lens and Docked modes retain the native CtrlAlt wheel shortcut");
    }
    {
        BrokerFixture f; f.ctrlAlt();
        require(f.mouse(WM_MOUSEWHEEL, 30) == 1 && f.windows() == 30 && !f.wheel() && sends.empty(), "CtrlAlt wheel routes only to window cycling in the Win profile");
    }
    {
        BrokerFixture f; f.config.winWheel = false; f.configure(); f.ctrlAlt();
        require(f.mouse(WM_MOUSEWHEEL, 30) == 1 && f.wheel() == 30 && !f.windows() && sends.empty(), "the legacy profile routes CtrlAlt wheel only to zoom");
    }
    {
        BrokerFixture f; f.record.nativeCaps = 0; f.record.magnifierReady = 0; f.record.nativeViewMode = 0; f.configure(); f.key(VK_LWIN, true);
        require(f.mouse(WM_MOUSEWHEEL, 1) == 1 && f.wheel() == 1, "a cold unclassified backend retains a fine physical zoom intent for bounded startup handling");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true);
        f.record.nativeCaps = 0; f.record.resolverReasonNative = ERROR_NOT_SUPPORTED;
        require(f.mouse(WM_MOUSEWHEEL, 120) == kPassed && !f.wheel() && sends.empty(),
                "a backend decline after configuration is checked again before consuming a mouse event");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true);
        f.record.nativeViewMode = 1;
        require(f.mouse(WM_MOUSEWHEEL, 120) == kPassed && !f.wheel() && sends.empty(),
                "a native view-mode transition after configuration cannot consume the first incompatible wheel event");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true); f.mouse(WM_MOUSEWHEEL, 1); f.key(0x4c, true); // Win+L
        f.configure(false); keys.fill(false); // Key UP events happened on the secure desktop, outside this hook.
        f.configure(true);
        require(f.key(VK_LWIN, true) == kPassed && f.mouse(WM_MOUSEWHEEL, 1) == 1 && sends.size() == 2,
                "a lost Win+L key-up sequence cannot block the first physical Win wheel session after unlock");
        require(f.key(VK_LWIN, false) == kPassed, "resynchronizing the broker does not swallow the later native Win release");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true); f.key(0x4c, true);
        f.configure(false); keys.fill(false); f.configure(true);
        require(f.key(VK_LWIN, true) == kPassed && f.key(VK_LWIN, false) == kPassed && sends.empty(),
                "an ordinary Win press after unlock still passes through without a synthetic menu mask");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true); f.mouse(WM_MBUTTONDOWN);
        f.configure(false); f.configure(true);
        require(f.mouse(WM_MBUTTONUP) == 1 && posts.size() == 1,
                "pause-and-resume key resynchronization preserves ownership of a still-held mouse button");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true); f.mouse(WM_MBUTTONDOWN);
        f.configure(false); keys.fill(false); f.broker.requestResync(true); f.configure(true);
        require(f.mouse(WM_MBUTTONDOWN) == kPassed && f.mouse(WM_MBUTTONUP) == kPassed && posts.size() == 1,
                "a session reset forgets a mouse release lost on another desktop instead of swallowing the next ordinary click");
    }
    {
        BrokerFixture f;
        require(f.mouse(WM_MOUSEMOVE) == kPassed && f.mouse(WM_XBUTTONDOWN) == kPassed &&
                f.mouse(WM_XBUTTONUP) == kPassed && f.mouse(WM_MOUSEHWHEEL, 120) == kPassed && keyReads == 0,
                "ordinary mouse movement and unbound buttons take the fast path without querying modifier state");
    }
    for (const bool leftFirst : {false, true}) {
        BrokerFixture f;
        const UINT first = leftFirst ? WM_LBUTTONDOWN : WM_RBUTTONDOWN;
        const UINT last = leftFirst ? WM_RBUTTONDOWN : WM_LBUTTONDOWN;
        const UINT releaseFirst = leftFirst ? WM_LBUTTONUP : WM_RBUTTONUP;
        const UINT releaseLast = leftFirst ? WM_RBUTTONUP : WM_LBUTTONUP;
        require(f.pointer(first, 100) == 1 && f.pointer(last, 130) == 1 && posts.size() == 1 &&
                posts[0].w == runtime::kToggleZoomAction && sends.empty() && cancelledTimers == 1,
                "a physical left-right pair toggles once in either order without replay or middle click");
        require(f.pointer(releaseFirst, 140) == 1 && f.pointer(first, 141) == 1 &&
                f.pointer(releaseLast, 150) == 1 && f.pointer(releaseFirst, 151) == 1 && posts.size() == 1,
                "a chord owns both ups and ignores pressed-again edges until both buttons are up");
        require(f.mouse(WM_MBUTTONDOWN) == kPassed && f.mouse(WM_MBUTTONUP) == kPassed,
                "a physical middle click still goes to Windows");
    }
    for (const bool left : {false, true}) {
        BrokerFixture f;
        const UINT down = left ? WM_LBUTTONDOWN : WM_RBUTTONDOWN;
        const UINT up = left ? WM_LBUTTONUP : WM_RBUTTONUP;
        const DWORD downFlag = left ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN;
        const DWORD upFlag = left ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP;
        require(f.pointer(down, 100) == 1 && f.pointer(up, 115) == 1 && sends.size() == 1 &&
                sends[0].inputs.size() == 2 && sends[0].inputs[0].type == INPUT_MOUSE &&
                sends[0].inputs[0].mi.dwFlags == downFlag && sends[0].inputs[1].mi.dwFlags == upFlag && posts.empty(),
                "a quick single click replays exactly its own down and up");
        f.pointer(down, 200);
        require(f.pointer(WM_MOUSEMOVE, 201, {5, 0}) == kPassed && sends.size() == 2 &&
                sends[1].inputs.size() == 1 && sends[1].inputs[0].mi.dwFlags == downFlag &&
                f.pointer(up, 230) == kPassed && posts.empty(), "movement releases one down for a normal drag");
        f.pointer(down, 300);
        runtime::InputBrokerTestAccess::expireChord(f.broker);
        require(sends.size() == 3 && sends[2].inputs.size() == 1 && f.pointer(up, 400) == kPassed,
                "the chord deadline releases a held down and its eventual up passes normally");
    }
    {
        BrokerFixture f;
        require(f.pointer(WM_LBUTTONDOWN, 100) == 1 && f.pointer(WM_RBUTTONDOWN, 160) == kPassed &&
                posts.empty() && sends.size() == 1 && sends[0].inputs[0].mi.dwFlags == MOUSEEVENTF_LEFTDOWN,
                "a stale pending button cannot toggle even if its timer message was delayed");
        require(f.pointer(WM_LBUTTONUP, 161) == kPassed && f.pointer(WM_RBUTTONUP, 162) == kPassed,
                "both regular ups pass following a slow overlap");
    }
    {
        BrokerFixture f; timerAvailable = false;
        require(f.mouse(WM_RBUTTONDOWN) == kPassed && f.mouse(WM_RBUTTONUP) == kPassed &&
                sends.empty() && posts.empty(), "timer failure leaves physical clicks alone");
        f.configure(false); timerAvailable = true;
        require(f.mouse(WM_LBUTTONDOWN) == kPassed && f.mouse(WM_RBUTTONDOWN) == kPassed &&
                f.mouse(WM_LBUTTONUP) == kPassed && f.mouse(WM_RBUTTONUP) == kPassed &&
                sends.empty() && posts.empty(), "paused input never delays or owns a new click");
    }
    {
        BrokerFixture f;
        require(f.mouse(WM_MOUSEWHEEL, 1) == kPassed && !f.wheel(), "plain wheel scrolls normally while zoom mode is off");
        f.broker.setPlainWheel(true);
        keyReads = 0;
        require(f.mouse(WM_MOUSEWHEEL, 1) == 1 && f.wheel() == 1 && !keyReads && sends.empty(),
                "an active mode consumes fractional wheel deltas without consulting modifier APIs");
        f.record.down = 1; f.record.phase = static_cast<LONG>(pinch::Phase::Pinch);
        require(f.mouse(WM_MOUSEWHEEL, 30) == 1 && f.wheel() == 31,
                "live pinch wheel input joins the zoom packet without leaking ordinary scroll");
        f.record.down = 0; f.record.magnifierReady = 0;
        require(f.mouse(WM_MOUSEWHEEL, -120) == kPassed && f.wheel() == 31,
                "a stale mode flag cannot steal ordinary scrolling after the magnifier closes");
        f.record.magnifierReady = 1; f.key(VK_LWIN, true);
        require(f.key(VK_ESCAPE, true) == kPassed && f.key(VK_ESCAPE, true) == kPassed &&
                posts.size() == 1 && posts[0].w == runtime::kExitZoomAction,
                "Win+Escape stays native while noticing one exit at its first down");
        f.key(VK_ESCAPE, false); f.key(VK_LWIN, false);
        require(f.mouse(WM_MOUSEWHEEL, -120) == kPassed && f.wheel() == 31,
                "Win+Escape immediately restores ordinary wheel handling even before the host processes its message");
    }
    {
        BrokerFixture f;
        f.record.down = 1; f.record.phase = static_cast<LONG>(pinch::Phase::Pinch);
        f.record.pinchFrameTime = static_cast<LONG64>(GetTickCount64());
        require(f.mouse(WM_MOUSEWHEEL, 30) == 1 && f.wheel() == 30,
                "a fresh live pinch accepts wheel input before any manual latch");
        f.record.pinchFrameTime = 1;
        require(f.mouse(WM_MOUSEWHEEL, 30) == kPassed && f.wheel() == 30,
                "a lost stale touchpad stream cannot keep capturing scroll on its own");
    }
    {
        BrokerFixture f;
        f.broker.setPlainWheel(true);
        f.key('A', true);
        require(f.mouse(WM_MOUSEWHEEL, -30) == 1 && f.wheel() == -30,
                "a non-modifier key held while armed cannot leak the vertical wheel into menus");
        f.key(VK_LSHIFT, true);
        require(f.mouse(WM_MOUSEWHEEL, 120) == kPassed && f.wheel() == -30,
                "an actual modifier still preserves its native scrolling shortcut");
    }
    {
        BrokerFixture f;
        require(f.mouse(WM_MOUSEHWHEEL, -30) == kPassed && sends.empty(),
                "the side wheel remains horizontal while the mode is off");
        f.broker.setPlainWheel(true);
        for (int delta : {-30, 75}) {
            const auto before = sends.size();
            require(f.mouse(WM_MOUSEHWHEEL, delta) == 1 && !f.wheel() && sends.size() == before,
                    "armed side-wheel movement queues conversion outside the native mouse callback");
            runtime::InputBrokerTestAccess::replaySideScroll(f.broker, threadPosts.back());
            require(sends.size() == before + 1 && sends.back().inputs.size() == 1,
                    "each horizontal delta generates one ordinary vertical mouse event");
            const auto& scroll = sends.back().inputs.front();
            require(scroll.type == INPUT_MOUSE && scroll.mi.dwFlags == MOUSEEVENTF_WHEEL && scroll.mi.dwExtraInfo &&
                    static_cast<int>(scroll.mi.mouseData) == -delta,
                    "side wheel matches the configured left-up and right-down mapping");
            require(f.mouse(WM_MOUSEWHEEL, -delta, true) == kPassed && !f.wheel(),
                    "the injected vertical scroll reaches Windows without recursive zoom");
            require(runtime::InputBrokerTestAccess::mouse(WM_MOUSEWHEEL, -delta, false, HC_ACTION, 100, {}, scroll.mi.dwExtraInfo) ==
                    kPassed && !f.wheel(), "the side-scroll tag also bypasses zoom when another hook removes the injected flag");
        }
        postThreadSuccess = false;
        require(f.mouse(WM_MOUSEHWHEEL, 30) == kPassed && sends.size() == 2,
                "failure to queue conversion forwards the original physical event");
        postThreadSuccess = true;
        sendResults = {0, 1};
        require(f.mouse(WM_MOUSEHWHEEL, 30) == 1, "queued side scroll owns just one horizontal event");
        runtime::InputBrokerTestAccess::replaySideScroll(f.broker, threadPosts.back());
        require(sends.size() == 4 && sends.back().inputs.front().mi.dwFlags == MOUSEEVENTF_HWHEEL &&
                sends.back().inputs.front().mi.mouseData == 30 && sends.back().inputs.front().mi.dwExtraInfo &&
                !f.wheel(), "failed vertical insertion falls back to the original tagged horizontal delta");
        f.key(VK_LWIN, true);
        require(f.mouse(WM_MOUSEHWHEEL, 30) == kPassed && sends.size() == 4,
                "horizontal modifier shortcuts continue to reach Windows");
    }
    {
        BrokerFixture f;
        f.broker.setPlainWheel(true);
        require(f.mouse(WM_MOUSEHWHEEL, 30) == 1, "scroll starts in one armed mode activation");
        f.broker.setPlainWheel(false);
        f.broker.setPlainWheel(true);
        runtime::InputBrokerTestAccess::replaySideScroll(f.broker, threadPosts.back());
        require(sends.empty(), "queued scroll from a previous activation is discarded even after immediate rearming");
        require(f.mouse(WM_MOUSEHWHEEL, 30) == 1, "the new activation accepts its own horizontal delta");
        runtime::InputBrokerTestAccess::replaySideScroll(f.broker, threadPosts.back(), 101);
        require(sends.empty(), "a stale pending side-wheel burst cannot scroll after a message loop stall");
    }
    {
        BrokerFixture f;
        require(f.mouse(WM_MBUTTONDOWN) == kPassed && f.mouse(WM_MBUTTONUP) == kPassed && posts.empty(),
                "middle button remains an ordinary click while the manual zoom mode is off");
        f.broker.setPlainWheel(true);
        require(f.mouse(WM_MBUTTONDOWN) == 1 && f.mouse(WM_MBUTTONUP) == 1 && posts.size() == 1 &&
                posts[0].w == runtime::kResetArmedZoomAction, "armed middle button posts one keep-mode reset and owns the matching up");
        require(f.mouse(WM_MOUSEWHEEL, 1) == 1 && f.wheel() == 1, "armed middle click does not disable the wheel");
        f.key(VK_LWIN, true);
        require(f.mouse(WM_MBUTTONDOWN) == 1 && f.mouse(WM_MBUTTONUP) == 1 && posts.size() == 2 && posts[1].w == 1,
                "the modified middle-button reset retains its original action");
    }
    {
        BrokerFixture f; sendResults = {1, 0};
        require(f.mouse(WM_LBUTTONDOWN) == 1 && f.mouse(WM_LBUTTONUP) == kPassed &&
                sends.size() == 2 && sends[1].inputs[0].mi.dwFlags == MOUSEEVENTF_LEFTUP,
                "if a short replay sends only down, a failed up retry leaves the physical up available");
    }
    {
        BrokerFixture f;
        require(f.mouse(WM_LBUTTONDOWN) == 1 && sends.empty(), "a deferred click is pending before shutdown");
        require(runtime::InputBrokerTestAccess::runWithControllerAlreadyStopping(f.broker) &&
                cancelledTimers == 1 && sends.empty() && !f.record.mouseHooksReady,
                "controller shutdown discards the pending click without SendInput or installing global hooks");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true); keys[0xe8] = true;
        f.broker.requestResync();
        require(f.mouse(WM_MOUSEWHEEL, 1) == 1 && f.wheel() == 1,
                "a neutral injected mask still down during resynchronization cannot become a stuck physical chord");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true); f.mouse(WM_MOUSEWHEEL, 1);
        f.key(VK_LWIN, true); // Physical modifier autorepeat can re-arm Start.
        const auto before = passed;
        require(f.key(VK_LWIN, false) == kPassed && releaseMasks.size() == 2 && f.record.menuMaskInitialSent == 1 && f.record.menuMaskReleaseSent == 1, "consumed Win wheel masks physical release after autorepeat without swallowing Win up");
        require(releaseMasks[0].key == 0xe8 && releaseMasks[1].key == 0xe8 && !releaseMasks[0].scan && !releaseMasks[1].scan &&
                !releaseMasks[0].flags && releaseMasks[1].flags == KEYEVENTF_KEYUP && releaseMasks[0].tag == releaseMasks[1].tag &&
                releaseMasks[0].passedBefore == before && releaseMasks[1].passedBefore == before, "only tagged neutral down/up runs before Win up reaches the next hook");
        f.key(VK_LWIN, true); f.key(VK_LWIN, false);
        require(releaseMasks.size() == 2, "a later plain Win press remains able to open Start");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true); f.mouse(WM_MOUSEWHEEL, 1); f.key(0x53, true); f.key(0x53, false); f.key(VK_LWIN, false);
        require(releaseMasks.empty(), "physical Win S cancels unnecessary release masking and remains native");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true); sendResults = {0}; f.mouse(WM_MOUSEWHEEL, 1); f.key(VK_LWIN, false);
        require(releaseMasks.empty(), "rejected mouse shortcut never acquires a release mask debt");
    }
    {
        BrokerFixture f; f.key(VK_RWIN, true); f.mouse(WM_MBUTTONDOWN); f.configure(false); f.key(VK_RWIN, false);
        require(releaseMasks.size() == 2 && f.mouse(WM_MBUTTONUP) == 1, "pause preserves consumed Win reset release masking and paired mouse up");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true); f.key(VK_RWIN, true); f.mouse(WM_MOUSEWHEEL, 1);
        f.key(VK_LWIN, false);
        require(releaseMasks.size() == 2 && f.mouse(WM_MOUSEWHEEL, 1) == 1, "releasing one consumed Win key masks it and preserves the other physical Win session");
        f.key(VK_RWIN, true); f.key(VK_RWIN, false);
        require(releaseMasks.size() == 4, "remaining Win release retains mouse debt across modifier repeat");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true); postSuccess = false;
        require(f.mouse(WM_MBUTTONDOWN) == kPassed, "a rejected reset post remains a native middle press");
        f.key(VK_LWIN, false);
        require(releaseMasks.empty(), "a reset that was not consumed cannot acquire release mask debt");
    }
    std::cout << checks << " real broker callback checks passed; fake Win32 endpoints; no hooks or input injection\n";
}
