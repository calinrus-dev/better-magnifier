// Production native callbacks with a fake backend. No desktop zoom, input or windows.
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
int calls = 0, writes = 0, moves = 0, saved = 0, primes = 0, reads = 0, nativeWheels = 0;
bool timerWorks = true, readWorks = true, lagged = false, createRenderer = false;
bool readWasGuarded = false, expectTimerGuard = false, timerReadsGuarded = true;
bool readerChangesEpoch = false, readerCancels = false, readerCloses = false;
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
long option(void*, int name) { return name == 20 ? 100 : name == 4 ? 2 : percent; }
void cache(void*, int name, long value, bool save) { if (name == 14) percent = value; if (save) ++saved; }
void change(void*, float value, POINT*) { ++writes; if (!lagged) rendered.zoom = value; }
BOOL WINAPI read(float* value, int* x, int* y) {
    ++reads; readWasGuarded = magnifier::activeUpdates.load() != 0;
    if (readerChangesEpoch) InterlockedIncrement(&state.inputRevision);
    if (readerCancels) magnifier::cancelKeyboardHold();
    if (readerCloses) magnifier::messageWindow.store(nullptr);
    *value = static_cast<float>(rendered.zoom); *x = rendered.x; *y = rendered.y; return readWorks;
}
void move(void*, const RECT* rectangle, bool animated) {
    if (!animated) std::exit(4);
    ++moves; rendered.x = rectangle->left; rendered.y = rectangle->top;
}
void prime(void*, long value) { ++primes; percent = value; }
void command(void* self, int identifier) {
    ++calls; observedStep = magnifier::readOption(self, 20);
    if (identifier == 0x800a || identifier == 0x800b)
        percent = std::clamp<long>(percent + (identifier == 0x800a ? observedStep : -observedStep), 100, 1600);
    if (createRenderer) {
        magnifier::nativeSelf.store(reinterpret_cast<void*>(0x2000));
        magnifier::rendererThread.store(GetCurrentThreadId());
    }
}
void nativeWheel(void*, int) { ++nativeWheels; }
LRESULT CALLBACK dispatch(HWND, UINT message, WPARAM identifier, LPARAM) {
    if (message == WM_HOTKEY && identifier <= 1) magnifier::zoomCommand(reinterpret_cast<void*>(0x1000), identifier ? 0x800b : 0x800a);
    if (message == 0x606) magnifier::wheel(reinterpret_cast<void*>(0x1000), static_cast<int>(identifier));
    return 17;
}
void send(unsigned identifier, std::uint64_t time, LPARAM parameters = 0) {
    timestamp = static_cast<LONG>(time); now = std::max(now, time);
    magnifier::uiWindowProc(nullptr, WM_HOTKEY, identifier, parameters);
}
void sendWheel(std::uint64_t time, int delta = 120) {
    timestamp = static_cast<LONG>(time); now = std::max(now, time);
    state.heartbeat = static_cast<LONG64>(GetTickCount64());
    magnifier::uiWindowProc(nullptr, 0x606, static_cast<WPARAM>(delta), 0);
}
void tick(std::uint64_t time) {
    now = time;
    magnifier::messages(reinterpret_cast<HWND>(0x3000), WM_TIMER, magnifier::kKeyboardHoldTimer, 0);
}
void clear() {
    magnifier::releaseManagedTarget();
    magnifier::keyboardHold = {};
    magnifier::keyboardHoldRevision = 0; magnifier::keyboardHoldGeneration = 0;
    magnifier::keyboardHeldVk = 0;
    magnifier::keyboardNativeTimestampSet = false; magnifier::keyboardNativeTimestamp = 0;
    magnifier::keyboardNativeClock = 0; magnifier::keyboardWheelMessage = false; magnifier::keyboardWheelTimestamp = 0;
    magnifier::overviewSuspended.store(false); magnifier::overviewView.reset();
    magnifier::nativeSelf.store(reinterpret_cast<void*>(0x2000));
    magnifier::nativeUi.store(reinterpret_cast<void*>(0x1000));
    magnifier::messageWindow.store(reinterpret_cast<HWND>(0x3000));
    magnifier::uiThread.store(GetCurrentThreadId()); magnifier::rendererThread.store(GetCurrentThreadId());
    magnifier::uiControlReady.store(true); magnifier::uiPrimeBlocked.store(false); magnifier::stopping.store(false);
    keys.fill(false); percent = 200; calls = writes = moves = saved = primes = reads = nativeWheels = 0;
    timerWorks = readWorks = true; lagged = createRenderer = false;
    readWasGuarded = expectTimerGuard = false; timerReadsGuarded = true; postedCancels = 0;
    readerChangesEpoch = readerCancels = readerCloses = false;
    rendered = {2.125, 37, 49}; now = 1000; timestamp = 1000;
    InterlockedExchange64(&state.wheelPacket, 0); InterlockedExchange64(&state.windowWheelPacket, 0);
    state.winWheel = 0; state.wheelEnabled = 1; state.wheelWindowSwitch = 0; state.hostWindow = 0x3000;
    state.inputRevision = 1; state.zoomRevision = 0; state.wheelActive = 0; state.down = 0;
    state.controlEnabled = 1; state.magnifierReady = 1; state.keyboardHoldMultiplier = 2;
    state.keyboardHoldRepeats = 0; state.keyboardHoldFreezes = 0; state.maxZoomPercent = 1600;
    state.shellMask = 0; state.normalInOverview = 0; state.overviewBypassed = 0;
    state.nativeCaps = compatibility::NativeCore | compatibility::NativeKeyboardObserver |
                       compatibility::NativeViewportRestore | compatibility::NativeBootstrapWindows;
}
void plus() { keys[VK_LWIN] = keys[VK_OEM_PLUS] = true; }
void startContinuous(unsigned id = 0, int vk = VK_OEM_PLUS) {
    keys[VK_LWIN] = true; keys[vk] = true;
    send(id, 1000, MAKELPARAM(MOD_WIN, vk)); tick(1200);
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
    magnifier::setLevel = prime; magnifier::originalZoomCommand = command; magnifier::originalUiWindowProc = dispatch;
    magnifier::originalWheel = nativeWheel;
    magnifier::keyboardKeyState = key; magnifier::keyboardClock64 = clockNow64;
    magnifier::keyboardMessageTime = messageTime; magnifier::keyboardSetTimer = timer; magnifier::keyboardKillTimer = kill;
    magnifier::keyboardPostMessage = post;
    clear(); send(0, 1000);
    require(calls == 1 && observedStep == 100 && !magnifier::keyboardHold.active(), "short tap delivered after release remains one native step");
    clear(); plus(); send(0, 1000); tick(1199);
    require(calls == 1 && observedStep == 100 && writes == 0 && !magnifier::keyboardHold.continuous(), "first tap remains native throughout the hold threshold");
    keys[VK_OEM_PLUS] = false; tick(1199);
    require(!magnifier::keyboardHold.active() && writes == 0 && moves == 0, "short tap never freezes its normal native animation");
    clear(); plus(); send(0, 1000);
    const LONG firstTapEpoch = state.inputRevision;
    const LONG firstTapZoomEpoch = state.zoomRevision;
    magnifier::change(reinterpret_cast<void*>(0x2000), 1.5f, nullptr);
    require(writes == 1 && rendered.zoom == 1.5 && state.nativeLevel == 30000 &&
            state.inputRevision == firstTapEpoch && state.zoomRevision == firstTapZoomEpoch && magnifier::keyboardHold.active(),
            "the first native tap keeps animating without its delayed frames cancelling the pending hold");
    tick(1199);
    require(!magnifier::keyboardHold.continuous() && writes == 1,
            "native animation does not bypass the two hundred millisecond hold threshold");
    tick(1200);
    require(magnifier::keyboardHold.continuous() && state.nativeLevel == 15000,
            "hold takeover acquires the visible native factor after the first animation");
    const LONG continuousEpoch = state.inputRevision;
    magnifier::change(reinterpret_cast<void*>(0x2000), 1.8f, nullptr);
    require(writes == 1 && rendered.zoom == 1.5 && state.inputRevision == continuousEpoch,
            "an old native frame cannot retarget the engine after continuous keyboard takeover");
    tick(1216);
    require(writes == 2 && state.nativeLevel > 15000 && state.nativeLevel < 15150 && state.inputRevision == continuousEpoch,
            "the first continuous frame advances smoothly without false renderer rebasing");
    keys[VK_OEM_PLUS] = false; tick(1220);
    const LONG frozenTarget = state.nativeLevel;
    const int frozenWrites = writes;
    magnifier::change(reinterpret_cast<void*>(0x2000), 3.0f, nullptr);
    require(state.nativeLevel == frozenTarget && writes == frozenWrites && state.keyboardHoldFreezes == 1,
            "release freezes once and its post-release lease rejects the former native goal");
    clear(); startContinuous();
    require(calls == 1 && writes == 0 && magnifier::keyboardHold.continuous() && reads == 1,
            "two hundred milliseconds takes ownership from one visible snapshot without another native step");
    const LONG acquiredEpoch = state.inputRevision;
    tick(1216); const double early = state.nativeLevel / 10000.0;
    require(writes == 1 && early > 2.125 && early < 2.15 && state.inputRevision == acquiredEpoch,
            "first continuous frame is precise and does not churn its ownership epoch");
    send(0, 1250); send(0, 1375);
    require(calls == 1 && observedStep == 100 && state.keyboardHoldRepeats == 2,
            "only owned native autorepeats are consumed instead of applying integer jumps");
    require(magnifier::readOption(nullptr, 20) == 100 && saved == 0, "held keyboard zoom never changes or saves the native increment preference");
    const int readsBeforeFrames = reads;
    for (unsigned time=1391; time<2000; time+=16) tick(time);
    require(reads == readsBeforeFrames && state.nativeLevel > early * 10000,
            "continuous integration uses its own last target instead of following rendered feedback");
    require(rendered.x == 37 && rendered.y == 49 && moves == 0, "continuous frames retain native viewport tracking without repositioning it");
    keys[VK_OEM_PLUS] = false; tick(2010);
    const LONG stopped = state.nativeLevel; const int writesAtStop = writes;
    tick(2030); send(0, 2005); tick(2040);
    require(!magnifier::keyboardHold.active() && writes == writesAtStop && state.nativeLevel == stopped && state.keyboardHoldFreezes == 1,
            "release freezes the visible level once and late autorepeats cannot restart it");
    magnifier::zoomCommand(nullptr, 0x800a);
    require(calls == 2 && observedStep == 100, "UI button remains native after a held session");
    send(0, 2200);
    require(calls == 3 && !magnifier::keyboardHold.active(), "a fresh short tap after the cutoff still performs its native step");
    clear(); plus(); send(0, 1000); tick(1200); lagged = true;
    const double fixedRendered = rendered.zoom;
    tick(1216); tick(1232); tick(1248);
    require(rendered.zoom == fixedRendered && state.nativeLevel / 10000.0 > fixedRendered + 0.03,
            "a lagged renderer cannot stall the progressive held-key target");
    keys[VK_OEM_PLUS] = false; tick(1250);
    require(std::abs(state.nativeLevel / 10000.0 - fixedRendered) < 0.0001 && moves == 1,
            "release adopts the visible factor rather than the ahead-of-animation goal");
    clear(); startContinuous(); lagged = true;
    for (unsigned time=1216; time<2500; time+=16) tick(time);
    const double visibleBeforeReverse = rendered.zoom;
    require(state.nativeLevel / 10000.0 > visibleBeforeReverse + 1,
            "lagged keyboard renderer can retain a goal ahead of the visible factor");
    keys[VK_OEM_MINUS] = true;
    send(1, 2510, MAKELPARAM(MOD_WIN, VK_OEM_MINUS));
    require(calls == 2 && state.keyboardHoldFreezes == 1 &&
            state.nativeLevel / 10000.0 < visibleBeforeReverse && percent == 113,
            "opposite keyboard direction freezes its old visible factor before applying one native contrary step");
    require(magnifier::keyboardHold.active() && magnifier::keyboardHold.direction() == -1 &&
            !magnifier::keyboardHold.continuous() && rendered.x == 37 && rendered.y == 49,
            "opposite keyboard key opens a fresh precision ramp while retaining the visible source position");
    clear(); keys[VK_RWIN] = keys[VK_SUBTRACT] = true;
    send(1, 1000, MAKELPARAM(MOD_WIN, VK_SUBTRACT)); tick(1200); tick(1216);
    require(calls == 1 && state.nativeLevel < 21250 && magnifier::keyboardHold.continuous(), "right Win and numpad subtract use continuous native zoom");
    keys[VK_SUBTRACT] = false; send(1, 1210, MAKELPARAM(MOD_WIN, VK_SUBTRACT));
    require(!magnifier::keyboardHold.active() && calls == 1 && state.keyboardHoldFreezes == 1,
            "a queued numpad repeat observes release before the low priority timer");
    clear(); startContinuous(0, VK_ADD); tick(1216);
    require(calls == 1 && state.nativeLevel > 21250, "numpad add receives decimal held-key frames");
    clear(); startContinuous(0, VK_OEM_1); tick(1216); keys[VK_OEM_1] = false; tick(1220);
    require(calls == 1 && state.keyboardHoldFreezes == 1, "the native layout virtual key controls start and release");
    clear(); plus(); send(0, 1000, MAKELPARAM(0, VK_OEM_PLUS)); tick(1400);
    require(calls == 1 && writes == 0 && !magnifier::keyboardHold.active(), "unknown hotkey signatures preserve native handling");
    clear(); state.keyboardHoldMultiplier = 1; plus(); send(0, 1000); send(0, 1250); tick(1500);
    require(calls == 2 && writes == 0 && observedStep == 100, "speed one opts out completely");
    clear(); state.nativeCaps &= ~compatibility::NativeBootstrapWindows; plus(); send(0, 1000); send(0, 1250);
    require(calls == 2 && writes == 0, "missing capabilities preserve native keyboard commands");
    clear(); state.nativeCaps &= ~compatibility::NativeKeyboardObserver; plus(); send(0, 1000); send(0, 1250); tick(1500);
    require(calls == 2 && writes == 0 && !magnifier::keyboardHold.active() && !magnifier::managedTarget,
            "an absent native command observer leaves held Win-plus entirely native without an echo filter");
    clear(); timerWorks = false; plus(); send(0, 1000); send(0, 1250);
    require(calls == 2 && writes == 0 && !magnifier::keyboardHold.active(), "timer installation failure preserves native commands");
    clear(); plus(); magnifier::nativeSelf.store(nullptr); magnifier::rendererThread.store(0); createRenderer = true;
    state.startAtNormal = 1; send(0, 1000); tick(1200);
    require(calls == 1 && state.startAtNormal == 0 && magnifier::keyboardHold.continuous(),
            "the initial native key can create its renderer and arm the hold afterwards");
    clear(); startContinuous(); tick(1216); InterlockedIncrement(&state.inputRevision);
    keys[VK_OEM_PLUS] = false; tick(1220);
    require(state.keyboardHoldFreezes == 0 && !magnifier::keyboardHold.active(), "newer input revision cancels without freezing it");
    clear(); plus(); send(0, 1000); state.wheelActive = 1; tick(1010); state.wheelActive = 0;
    send(0, 1005);
    require(calls == 1 && writes == 0, "queued first repeat cannot overwrite wheel input after the initial native tap");
    clear(); startContinuous(); state.down = 1; state.phase = static_cast<LONG>(pinch::Phase::Pinch); tick(1216);
    require(writes == 0 && !magnifier::keyboardHold.active(), "pinch ownership cancels continuous keyboard control");
    clear(); startContinuous(); state.controlEnabled = 0; tick(1216);
    require(writes == 0 && !magnifier::keyboardHold.active(), "inactive or disabled control cancels without camera writes");
    clear(); startContinuous(); magnifier::messages(nullptr, magnifier::kCancelKeyboardHold, 0, 0); keys[VK_OEM_PLUS] = false; tick(1220);
    require(writes == 0 && !magnifier::keyboardHold.active(), "settings or unload cancellation never freezes a newer target");
    clear(); startContinuous(); readWorks = false; keys[VK_OEM_PLUS] = false; tick(1220);
    require(writes == 0 && moves == 0 && !magnifier::keyboardHold.active(), "failed release snapshot cannot write an invented transform");
    clear(); plus(); send(0, 1000); readerChangesEpoch = true; tick(1200);
    require(writes == 0 && !magnifier::keyboardHold.active() && magnifier::activeUpdates.load() == 0,
            "a newer input arriving during ownership snapshot prevents continuous control");
    clear(); startContinuous(); readerChangesEpoch = true; keys[VK_OEM_PLUS] = false; tick(1220);
    require(writes == 0 && moves == 0 && state.keyboardHoldFreezes == 0 && magnifier::activeUpdates.load() == 0,
            "a newer input arriving during release snapshot prevents every freeze write");
    clear(); startContinuous(); readerCancels = true; keys[VK_OEM_PLUS] = false; tick(1220);
    require(writes == 0 && moves == 0 && magnifier::activeUpdates.load() == 0, "a reentrant cancel invalidates the release snapshot generation");
    clear(); startContinuous(); readerCloses = true; keys[VK_OEM_PLUS] = false; tick(1220);
    require(writes == 0 && moves == 0 && magnifier::activeUpdates.load() == 0, "closing the message window during a snapshot prevents further writes");
    clear(); startContinuous(1, VK_OEM_MINUS); rendered = {1.08, 37, 49};
    percent = 100; state.nativeLevel = 10000; magnifier::trackingInitialized.store(false);
    keys[VK_OEM_MINUS] = false; tick(1220);
    require(primes == 1 && magnifier::trackingInitialized.load() && state.nativeLevel == 10800 && rendered.x == 37 && rendered.y == 49,
            "release above a native 100 percent goal reactivates tracking and restores the exact source position");
    clear(); startContinuous(); keys[VK_OEM_PLUS] = false;
    magnifier::nativeCommandDepth = 1; tick(1220);
    require(magnifier::keyboardHold.active() && writes == 0, "reentrant polling waits for the owning native command");
    magnifier::nativeCommandDepth = 0; tick(1220);
    require(!magnifier::keyboardHold.active() && writes == 1, "the next poll freezes after native completion");
    clear(); plus(); send(0, 1000); magnifier::uiThread.store(GetCurrentThreadId()+1); magnifier::cancelKeyboardHold();
    require(postedCancels == 1 && magnifier::keyboardHold.active(), "foreign thread cancellation posts without mutating owner state");
    magnifier::uiThread.store(GetCurrentThreadId()); magnifier::messages(nullptr, magnifier::kCancelKeyboardHold, 0, 0);
    require(!magnifier::keyboardHold.active(), "owner processes posted cancellation");
    clear(); startContinuous(); expectTimerGuard = true; tick(1216);
    require(timerReadsGuarded && readWasGuarded, "timer and snapshot callbacks are guarded against reentrant unloading");
    clear(); plus(); send(0, 1010); now = 1020; sendWheel(1000);
    require(magnifier::keyboardHold.active() && !state.wheelPacket && nativeWheels == 0 && calls == 1,
            "a native wheel relay overtaken by a newer hotkey cannot enqueue zoom or cancel its session");
    require(!magnifier::keyboardWheelMessage && !magnifier::keyboardWheelTimestamp,
            "native wheel timestamp context is scoped to its original window procedure");
    sendWheel(1030);
    require(!magnifier::keyboardHold.active() && pinch::wheelDelta(state.wheelPacket) == 120,
            "a newer native wheel event still wins over keyboard control");
    clear(); plus(); send(0, 1010); sendWheel(1010);
    require(!magnifier::keyboardHold.active() && pinch::wheelDelta(state.wheelPacket) == 120,
            "equal millisecond timestamps retain delivered order instead of guessing a stale event");
    clear(); plus(); send(0, 1010); now = 1020;
    state.heartbeat = static_cast<LONG64>(GetTickCount64());
    magnifier::wheel(reinterpret_cast<void*>(0x1000), 30);
    require(!magnifier::keyboardHold.active() && pinch::wheelDelta(state.wheelPacket) == 30,
            "an untagged native control call is never filtered by an old private-message timestamp");
    clear(); plus(); send(0, 1010); now = 61011; sendWheel(1000);
    require(!magnifier::keyboardHold.active() && pinch::wheelDelta(state.wheelPacket) == 120,
            "the wheel ordering fence expires using the sixty-second monotonic clock window");
    clear(); plus(); send(0, 0x100000010ULL); sendWheel(0xfffffff0ULL);
    require(magnifier::keyboardHold.active() && !state.wheelPacket,
            "native wheel timestamp ordering handles a DWORD uptime wrap");
    pinch::KeyboardHold wrap;
    constexpr std::uint64_t wrappedClock = 0x100000010ULL;
    wrap.begin(1, wrappedClock-1000); wrap.close(wrappedClock);
    require(wrap.stale(1, 0xfffffff0, wrappedClock) && wrap.stale(1, 0x10, wrappedClock) && !wrap.stale(1, 0x11, wrappedClock),
            "DWORD timestamps compare correctly across uptime wrap");
    require(!wrap.stale(-1, 0xfffffff0, wrappedClock) && !wrap.stale(1, 0xfffffff0, wrappedClock+60001),
            "a closed-session cutoff is direction specific and expires on the 64-bit clock");
    pinch::KeyboardHold model;
    model.begin(1, 1000); require(!model.eligible(1199) && model.eligible(1200), "hold threshold is exact");
    model.activate(1, 1200);
    double first = 0, later = 0, previous = 1;
    for (unsigned time=1216; time<=5000; time+=16) {
        const auto next = model.tick(time, 2, 16);
        if (!next) continue;
        const double rate = std::log(*next/previous)/0.016;
        if (!first) first = rate;
        later = rate;
        require(rate <= std::log(5.0)+0.00001, "a continuous frame exceeds its maximum logarithmic speed");
        previous = *next;
    }
    require(previous == 16 && later > first && !model.tick(6000, 2, 16), "progressive hold reaches its upper bound and does not store hidden travel");
    model.close(6000); model.begin(-1, 6000); model.activate(16, 6200);
    for (unsigned time=6216; time<=11000; time+=16) model.tick(time, 2, 16);
    require(!model.tick(12000, 2, 16), "continuous zoom out reaches its lower bound without stored momentum");
    model.close(12000); model.begin(1, 12000); model.activate(1, 12200);
    const double stalled = *model.tick(20000, 2, 16);
    require(stalled > 1 && stalled <= std::pow(5.0,0.064)+0.00001, "a stalled native UI cannot teleport through old elapsed time");
    clear(); magnifier::messageWindow.store(nullptr);
    std::cout << checks << " progressive keyboard-hold checks passed; no desktop changes\n";
}
