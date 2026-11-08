#define PINCH_ZOOM_STANDALONE
#define PINCH_ZOOM_NATIVE_TEST
#define wmain diagnostic_main
#include "../mods/kde-pinch-zoom.wh.cpp"
#undef wmain
#include <iostream>

namespace {
bridge::Record state{};
pinch::View rendered{2.125, 37, 49};
long percent = 200, observedStep = 0;
int calls = 0, writes = 0, moves = 0, saved = 0, primes = 0;
bool timerWorks = true, readWorks = true;
bool readWasGuarded = false, expectTimerGuard = false, timerReadsGuarded = true;
bool readerChangesEpoch = false;
bool readerCancels = false;
unsigned postedCancels = 0;
std::uint64_t now = 1000;
LONG timestamp = 1000;
std::array<bool, 256> keys{};
SHORT WINAPI key(int code) {
    if (expectTimerGuard && magnifier::activeUpdates.load() == 0) timerReadsGuarded = false;
    return keys.at(code) ? static_cast<SHORT>(0x8000) : 0;
}
ULONGLONG WINAPI clockNow64() { return now; }
LONG WINAPI messageTime() { return timestamp; }
UINT_PTR WINAPI timer(HWND, UINT_PTR id, UINT, TIMERPROC) { return timerWorks ? id : 0; }
BOOL WINAPI kill(HWND, UINT_PTR) { return TRUE; }
BOOL WINAPI post(HWND, UINT message, WPARAM, LPARAM) { if (message == magnifier::kCancelKeyboardHold) ++postedCancels; return TRUE; }
HRESULT keeper(void** result) { *result = reinterpret_cast<void*>(0x1000); return S_OK; }
long option(void*, int name) { return name == 20 ? 100 : percent; }
void cache(void*, int name, long value, bool save) { if (name == 14) percent = value; if (save) ++saved; }
void change(void*, float value, POINT*) { ++writes; rendered.zoom = value; }
BOOL WINAPI read(float* value, int* x, int* y) {
    readWasGuarded = magnifier::activeUpdates.load() != 0;
    if (readerChangesEpoch) InterlockedIncrement(&state.inputRevision);
    if (readerCancels) magnifier::cancelKeyboardHold();
    *value = static_cast<float>(rendered.zoom); *x = rendered.x; *y = rendered.y; return readWorks;
}
void move(void*, const RECT* rectangle, bool animated) {
    if (!animated) std::exit(4);
    ++moves; rendered.x = rectangle->left; rendered.y = rectangle->top;
}
void prime(void*, long value) { ++primes; percent = value; rendered.x = 0; rendered.y = 0; }
void command(void* self, int identifier) {
    ++calls;
    observedStep = magnifier::readOption(self, 20);
    if (identifier == 0x800a || identifier == 0x800b)
        percent = std::clamp<long>(percent + (identifier == 0x800a ? observedStep : -observedStep), 100, 1600);
}
LRESULT CALLBACK dispatch(HWND, UINT message, WPARAM identifier, LPARAM) {
    if (message == WM_HOTKEY && identifier <= 1) magnifier::zoomCommand(reinterpret_cast<void*>(0x1000), identifier ? 0x800b : 0x800a);
    return 17;
}
void send(unsigned identifier, DWORD time, LPARAM parameters = 0) {
    timestamp = static_cast<LONG>(time); now = std::max<std::uint64_t>(now, time);
    magnifier::uiWindowProc(nullptr, WM_HOTKEY, identifier, parameters);
}
void clear() {
    magnifier::keyboardHold = {}; magnifier::keyboardStepMultiplier = 1;
    magnifier::overviewSuspended.store(false); magnifier::overviewView.reset();
    magnifier::nativeSelf.store(reinterpret_cast<void*>(0x2000));
    keys.fill(false); percent = 200; calls = writes = moves = saved = primes = 0;
    timerWorks = readWorks = true;
    readWasGuarded = false; expectTimerGuard = false; timerReadsGuarded = true; postedCancels = 0;
    readerChangesEpoch = false;
    readerCancels = false;
    rendered = {2.125, 37, 49};
    state.inputRevision = 1; state.zoomRevision = 0; state.wheelActive = 0; state.down = 0;
    state.controlEnabled = 1; state.magnifierReady = 1; state.keyboardHoldMultiplier = 2;
    state.keyboardHoldRepeats = 0; state.keyboardHoldFreezes = 0;
    state.shellMask = 0; state.normalInOverview = 0; state.overviewBypassed = 0;
    state.nativeCaps = compatibility::NativeCore | compatibility::NativeKeyboardObserver |
                       compatibility::NativeViewportRestore | compatibility::NativeBootstrapWindows;
}
}
int wmain() {
    unsigned checks = 0;
    const auto require = [&](bool value, const char* name) {
        if (!value) { std::cerr << "FAIL " << name << '\n'; std::exit(1); } ++checks;
    };
    magnifier::mapping.useTestRecord(&state);
    magnifier::getKeeper = keeper; magnifier::getOption = option; magnifier::originalOption = cache;
    magnifier::originalChange = change; magnifier::readTransform = read; magnifier::moveTo = move;
    magnifier::setLevel = prime;
    magnifier::originalZoomCommand = command; magnifier::originalUiWindowProc = dispatch;
    magnifier::keyboardKeyState = key; magnifier::keyboardClock64 = clockNow64;
    magnifier::keyboardMessageTime = messageTime; magnifier::keyboardSetTimer = timer; magnifier::keyboardKillTimer = kill;
    magnifier::keyboardPostMessage = post;
    magnifier::nativeUi.store(reinterpret_cast<void*>(0x1000)); magnifier::nativeSelf.store(reinterpret_cast<void*>(0x2000));
    magnifier::messageWindow.store(reinterpret_cast<HWND>(0x3000));
    magnifier::uiThread.store(GetCurrentThreadId()); magnifier::rendererThread.store(GetCurrentThreadId());
    clear();
    send(0, 1000);
    require(calls == 1 && observedStep == 100 && !magnifier::keyboardHold.active(), "short tap delivered after release stays native");
    clear(); keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 2000);
    require(calls == 1 && observedStep == 100 && magnifier::keyboardHold.active(), "first physical tap keeps native step");
    send(0, 2250);
    require(calls == 2 && observedStep == 200 && magnifier::keyboardStepMultiplier == 1, "repeat doubles only scoped Option20");
    require(state.keyboardHoldRepeats == 1 && state.keyboardHoldFreezes == 0, "diagnostic counts only accelerated native repeats");
    require(magnifier::readOption(nullptr, 20) == 100 && saved == 0, "step preference is not persisted or globally changed");
    keys[VK_OEM_PLUS] = false; now = 2260;
    expectTimerGuard = true;
    magnifier::messages(nullptr, WM_TIMER, magnifier::kKeyboardHoldTimer, 0);
    expectTimerGuard = false;
    require(writes == 1 && moves == 1 && percent == 213 && state.nativeLevel == 21250 && rendered.x == 37 && rendered.y == 49,
            "release retargets rendered float and source position once");
    require(state.keyboardHoldFreezes == 1, "diagnostic counts the successful release freeze");
    require(!magnifier::keyboardHold.active() && state.inputRevision > 1 && state.zoomRevision > 0, "release invalidates pending input generation");
    require(readWasGuarded && timerReadsGuarded && magnifier::activeUpdates.load() == 0, "timer and snapshot external calls are covered by unload guards");
    send(0, 2255);
    require(calls == 2 && writes == 1, "old queued hotkey cannot move after freeze");
    magnifier::zoomCommand(nullptr, 0x800a);
    require(calls == 3 && observedStep == 100, "UI button remains native after a completed hold");
    send(0, 2400);
    require(calls == 4 && observedStep == 100, "fresh short tap after cutoff remains native");
    clear(); keys[VK_RWIN] = keys[VK_SUBTRACT] = true;
    send(1, 3000); send(1, 3250);
    require(calls == 2 && observedStep == 200, "right Win and numpad subtract accelerate");
    keys[VK_SUBTRACT] = false; now = 3260; timestamp = 3255;
    magnifier::uiWindowProc(nullptr, WM_HOTKEY, 1, 0);
    require(writes == 1 && calls == 2, "a queued hotkey observes release before the lower priority timer");
    clear(); keys[VK_RWIN] = keys[VK_ADD] = true;
    send(0, 4000); send(0, 4250);
    require(calls == 2 && observedStep == 200, "numpad add accelerates");
    InterlockedIncrement(&state.inputRevision); keys[VK_ADD] = false; now = 4260;
    magnifier::pollKeyboardHold();
    require(writes == 0 && !magnifier::keyboardHold.active(), "new input cancels hold without freezing its target");
    clear(); state.keyboardHoldMultiplier = 1; keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 5000); send(0, 5250);
    require(calls == 2 && observedStep == 100 && !magnifier::keyboardHold.active(), "multiplier one leaves native commands intact");
    clear(); state.nativeCaps &= ~compatibility::NativeBootstrapWindows;
    keys[VK_LWIN] = keys[VK_OEM_MINUS] = true;
    send(1, 6000); send(1, 6250);
    require(calls == 2 && observedStep == 100 && !magnifier::keyboardHold.active(), "missing capabilities retain native keyboard behavior");
    pinch::KeyboardHold wrap;
    constexpr std::uint64_t wrappedClock = 0x100000010ULL;
    wrap.command(1, true, 2); wrap.command(1, true, 2); wrap.close(wrappedClock);
    require(wrap.stale(1, 0xfffffff0, wrappedClock) && wrap.stale(1, 0x10, wrappedClock) &&
            !wrap.stale(1, 0x11, wrappedClock), "DWORD timestamps compare correctly across uptime wrap");
    require(!wrap.stale(-1, 0xfffffff0, wrappedClock), "closed-session cutoff applies only to its direction");
    require(!wrap.stale(1, 0xfffffff0, wrappedClock + 60001), "closed sessions expire on a 64-bit clock");
    require(!wrap.stale(1, 0xfffffff0, wrappedClock + 0x100000000ULL), "a later DWORD wrap cannot revive an expired session cutoff");
    clear(); keys[VK_LWIN] = keys[VK_OEM_1] = true;
    const LPARAM nativeSpanishKey = MAKELPARAM(MOD_WIN, VK_OEM_1);
    send(0, 7000, nativeSpanishKey); send(0, 7250, nativeSpanishKey);
    require(calls == 2 && observedStep == 200, "native lParam virtual key supports a different keyboard layout");
    keys[VK_OEM_1] = false; now = 7260; magnifier::pollKeyboardHold();
    require(writes == 1 && !magnifier::keyboardHold.active(), "release watches the native layout key");
    clear(); keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 8000, MAKELPARAM(0, VK_OEM_PLUS)); send(0, 8250, MAKELPARAM(0, VK_OEM_PLUS));
    require(calls == 2 && observedStep == 100 && !magnifier::keyboardHold.active(), "unknown native hotkey signature is never accelerated");
    clear(); keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 9000); keys[VK_OEM_PLUS] = false; now = 9010; magnifier::pollKeyboardHold();
    require(calls == 1 && writes == 0 && moves == 0, "a tap without repeats never freezes native animation");
    clear(); timerWorks = false; keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 10000); send(0, 10250);
    require(calls == 2 && observedStep == 100 && !magnifier::keyboardHold.active(), "timer installation failure preserves original commands");
    clear(); keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 11000); send(0, 11250);
    magnifier::messages(nullptr, magnifier::kCancelKeyboardHold, 0, 0);
    keys[VK_OEM_PLUS] = false; now = 11260; magnifier::pollKeyboardHold();
    require(writes == 0 && !magnifier::keyboardHold.active(), "settings or unload cancellation never freezes a newer target");
    clear(); keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 12000); send(0, 12250); readWorks = false;
    keys[VK_OEM_PLUS] = false; now = 12260; magnifier::pollKeyboardHold();
    require(writes == 0 && moves == 0 && !magnifier::keyboardHold.active(), "failed rendered snapshot cannot write a substitute transform");
    clear(); keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 13000); send(0, 13250);
    magnifier::change(reinterpret_cast<void*>(0x2000), 3.3f, nullptr);
    keys[VK_OEM_PLUS] = false; now = 13260; magnifier::pollKeyboardHold();
    require(writes == 1 && moves == 0 && !magnifier::keyboardHold.active(), "a different native renderer command wins without an extra release freeze");
    clear(); keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 14000); send(0, 14250); state.wheelActive = 1;
    now = 14260; magnifier::pollKeyboardHold();
    require(writes == 0 && !magnifier::keyboardHold.active(), "wheel ownership cancels keyboard hold without freezing");
    clear(); keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 15000); send(0, 15250); state.down = 1; state.phase = static_cast<LONG>(pinch::Phase::Pinch);
    now = 15260; magnifier::pollKeyboardHold();
    require(writes == 0 && !magnifier::keyboardHold.active(), "pinch ownership cancels keyboard hold without freezing");
    clear(); keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 16000); send(0, 16250); state.controlEnabled = 0;
    now = 16260; magnifier::pollKeyboardHold();
    require(writes == 0 && !magnifier::keyboardHold.active(), "disable or inactive session cancels without camera writes");
    clear(); keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 17000); state.wheelActive = 1; now = 17010; magnifier::pollKeyboardHold();
    state.wheelActive = 0; timestamp = 17005; now = 17020;
    magnifier::uiWindowProc(nullptr, WM_HOTKEY, 0, 0);
    require(calls == 1 && writes == 0, "a queued first repeat cannot overwrite wheel input after a one-command session");
    clear(); keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 18000); magnifier::uiThread.store(GetCurrentThreadId() + 1);
    magnifier::cancelKeyboardHold();
    require(postedCancels == 1 && magnifier::keyboardHold.active(), "foreign thread cancellation posts without mutating UI-owned policy");
    magnifier::uiThread.store(GetCurrentThreadId());
    magnifier::messages(nullptr, magnifier::kCancelKeyboardHold, 0, 0);
    require(!magnifier::keyboardHold.active() && writes == 0, "owner processes posted cancellation without a freeze");
    clear(); keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 19000); send(0, 19250); readerChangesEpoch = true;
    keys[VK_OEM_PLUS] = false; now = 19260; magnifier::pollKeyboardHold();
    require(writes == 0 && moves == 0 && state.keyboardHoldFreezes == 0 && magnifier::activeUpdates.load() == 0,
            "reentrant newer input during snapshot prevents every freeze write and drains the guard");
    clear(); keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 20000); send(0, 20250); readerCancels = true;
    const LONG epochBeforeRead = state.inputRevision;
    keys[VK_OEM_PLUS] = false; now = 20260; magnifier::pollKeyboardHold();
    require(writes == 0 && moves == 0 && state.inputRevision == epochBeforeRead && magnifier::activeUpdates.load() == 0,
            "UI cancellation during snapshot prevents freeze even before shared input epoch changes");
    clear(); keys[VK_LWIN] = keys[VK_OEM_MINUS] = true;
    send(1, 21000); send(1, 21250); rendered = {1.08, 37, 49};
    require(percent == 100 && !magnifier::trackingInitialized.load(), "native minus can finish its target before the visible frame reaches 100 percent");
    keys[VK_OEM_MINUS] = false; now = 21260; magnifier::pollKeyboardHold();
    require(primes == 1 && magnifier::trackingInitialized.load() && state.nativeLevel == 10800 &&
            rendered.x == 37 && rendered.y == 49, "freezing above 100 reprimes native tracking and restores the visible position");
    clear(); keys[VK_LWIN] = keys[VK_OEM_PLUS] = true;
    send(0, 22000); send(0, 22250); keys[VK_OEM_PLUS] = false; now = 22260;
    magnifier::nativeCommandDepth = 1; magnifier::pollKeyboardHold();
    require(magnifier::keyboardHold.active() && writes == 0, "reentrant polling waits for the owning native command to complete");
    magnifier::nativeCommandDepth = 0; magnifier::pollKeyboardHold();
    require(!magnifier::keyboardHold.active() && writes == 1, "the next poll finalizes the released hold after native completion");
    clear(); magnifier::messageWindow.store(nullptr);
    std::cout << checks << " isolated keyboard-hold checks passed; no desktop changes\n";
}
