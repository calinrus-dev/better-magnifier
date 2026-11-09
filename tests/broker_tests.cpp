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
std::vector<SendCall> sends;
std::vector<UINT> sendResults;
std::size_t nextSendResult = 0;
std::vector<Posted> posts;
unsigned passed = 0, keyReads = 0;
bool postSuccess = true;
SHORT WINAPI keyState(int key) { ++keyReads; return key >= 0 && key < 256 && keys[static_cast<unsigned>(key)] ? static_cast<SHORT>(0x8000) : 0; }
UINT WINAPI sendInput(UINT count, LPINPUT input, int size) {
    if (size != sizeof(INPUT)) return 0;
    sends.push_back({std::vector<INPUT>(input, input + count)});
    return nextSendResult < sendResults.size() ? sendResults[nextSendResult++] : count;
}
BOOL WINAPI post(HWND target, UINT message, WPARAM w, LPARAM l) {
    if (!postSuccess) return FALSE;
    posts.push_back({target, message, w, l}); return TRUE;
}
LRESULT WINAPI next(HHOOK, int, WPARAM, LPARAM) { ++passed; return kPassed; }
}

namespace runtime {
struct InputBrokerTestAccess {
    static void attach(MouseInputBroker& broker, bridge::Record& record) {
        broker.state_ = &record; broker.target_ = reinterpret_cast<HWND>(0x1234);
        MouseInputBroker::owner_ = &broker;
        MouseInputBroker::apiReadKey = keyState;
        MouseInputBroker::apiSendInput = sendInput;
        MouseInputBroker::apiPost = post;
        MouseInputBroker::apiNext = next;
    }
    static void flushResync(MouseInputBroker& broker) { broker.consumeResync(); }
    static void detach(MouseInputBroker& broker) { if (MouseInputBroker::owner_ == &broker) MouseInputBroker::owner_ = nullptr; }
    static LRESULT key(unsigned vk, bool down, bool injected = false, int code = HC_ACTION) {
        KBDLLHOOKSTRUCT event{}; event.vkCode = vk;
        event.flags = injected ? LLKHF_INJECTED : 0;
        if (!down) event.flags |= LLKHF_UP;
        const auto result = MouseInputBroker::keyboard(code, down ? WM_KEYDOWN : WM_KEYUP, reinterpret_cast<LPARAM>(&event));
        if (code == HC_ACTION && vk < keys.size()) keys[vk] = down;
        return result;
    }
    static LRESULT mouse(UINT message, int delta = 0, bool injected = false, int code = HC_ACTION) {
        MSLLHOOKSTRUCT event{};
        event.mouseData = static_cast<DWORD>(static_cast<std::uint16_t>(delta)) << 16;
        event.flags = injected ? LLMHF_INJECTED : 0;
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
        keys.fill(false); sends.clear(); sendResults.clear(); nextSendResult = 0; posts.clear(); passed = 0; postSuccess = true;
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
        require(f.mouse(WM_MOUSEWHEEL, 1) == 1 && f.wheel() == 1 && f.record.wheelEvents == 1,
                "Win wheel captures one precise unit in the bounded zoom mailbox");
        require(sends.size() == 1 && neutralMask(sends[0]), "a consumed first Win mouse action sends only a tagged neutral down/up pair");
        require(f.mouse(WM_MOUSEWHEEL, 30) == 1 && f.wheel() == 31 && sends.size() == 1,
                "later high-resolution deltas do not repeatedly inject a menu mask");
        require(f.key(VK_LWIN, false) == kPassed, "Win up remains native after mouse zoom");
        f.key(VK_RWIN, true);
        require(f.mouse(WM_MOUSEWHEEL, -1) == 1 && f.wheel() == -1 && sends.size() == 2,
                "RWin opens an independent mouse session and reverses old wheel debt");
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
        require(f.mouse(WM_MOUSEWHEEL, 120) == kPassed && !f.wheel() && posts.empty(), "a rejected mask leaves wheel input to Windows rather than consuming a broken shortcut");
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
        require(f.mouse(WM_MOUSEMOVE) == kPassed && f.mouse(WM_LBUTTONDOWN) == kPassed &&
                f.mouse(WM_LBUTTONUP) == kPassed && f.mouse(WM_MOUSEHWHEEL, 120) == kPassed && keyReads == 0,
                "ordinary mouse movement and unbound buttons take the fast path without querying modifier state");
    }
    {
        BrokerFixture f; f.key(VK_LWIN, true); keys[0xe8] = true;
        f.broker.requestResync();
        require(f.mouse(WM_MOUSEWHEEL, 1) == 1 && f.wheel() == 1,
                "a neutral injected mask still down during resynchronization cannot become a stuck physical chord");
    }
    std::cout << checks << " real broker callback checks passed; fake Win32 endpoints; no hooks or input injection\n";
}
