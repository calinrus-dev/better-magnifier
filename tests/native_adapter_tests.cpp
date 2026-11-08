// Exercise the production native WndProc with an in-memory protocol and fake
// Windows Magnifier callbacks. Never opens Magnifier or changes desktop zoom.
#define PINCH_ZOOM_STANDALONE
#define PINCH_ZOOM_NATIVE_TEST
#define wmain diagnostic_main
#include "../mods/kde-pinch-zoom.wh.cpp"
#undef wmain
#include <iostream>

namespace {
bridge::Record state{};
long percent = 100;
int uiCalls = 0, directWrites = 0, persistedWrites = 0, cursorWarps = 0;
bool simulateReentrantClose = false;
LRESULT closeResult = 0;
double lastFloat = 1;
pinch::View visibleView{};
int cameraMoves = 0;
bool restoredTarget = false;
int clipLimits = 0, clipReleases = 0;
BOOL WINAPI clipBackend(const RECT* rectangle) { if (rectangle) ++clipLimits; else ++clipReleases; return TRUE; }
BOOL WINAPI readView(float* level, int* x, int* y) { *level = static_cast<float>(visibleView.zoom); *x = visibleView.x; *y = visibleView.y; return TRUE; }
void moveView(void*, const RECT* rectangle, bool animated) { restoredTarget = animated; ++cameraMoves; visibleView.x = rectangle->left; visibleView.y = rectangle->top; }
void rendererCommand(void* self, int) { magnifier::change(self, 3.7f, nullptr); magnifier::option(self, 14, 100, false); }
int originalWheelCalls = 0;
void nativeWheel(void*, int) { ++originalWheelCalls; }
LRESULT CALLBACK windowBackend(HWND, UINT, WPARAM, LPARAM) { return 321; }
LONG_PTR WINAPI existingObject(HWND, int index) { return index == GWLP_USERDATA ? 0x6789 : 0; }
HRESULT keeper(void** result) { *result = reinterpret_cast<void*>(0x1000); return S_OK; }
long optionValue(void*, int name) { return name == 14 ? percent : name == 20 ? 100 : 2; }
void cache(void*, int name, long value, bool save) {
    if (name == 14) percent = value;
    if (save) ++persistedWrites;
}
void apply(void* self, long value) {
    ++uiCalls;
    visibleView.zoom = value / 100.0;
    if (simulateReentrantClose) closeResult = magnifier::messages(nullptr, WM_CLOSE, 0, 0);
    // Native UI requests persistence; the mod must suppress that for a pinch.
    magnifier::option(self, 14, value, true);
}
void direct(void*, float value, POINT*) { ++directWrites; lastFloat = value; visibleView.zoom = value; if (value <= 1) { visibleView.x = 0; visibleView.y = 0; } }
void command(void* self, int) { magnifier::moveMouse(self, POINT{10, 20}); magnifier::option(self, 14, percent + 100, true); }
void cursorWarp(void*, POINT) { ++cursorWarps; }
void deliver(LONG target, LONG sequence) {
    state.requestRevision = state.zoomRevision;
    state.requestedLevel = target;
    state.requestedSequence = sequence;
    magnifier::messages(nullptr, magnifier::updateMessage, 0, 0);
}
}

int wmain() {
    int checks = 0;
    const auto require = [&](bool valid, const char* name) {
        if (!valid) { std::cerr << "FAIL " << name << '\n'; std::exit(1); }
        ++checks;
    };
    magnifier::mapping.useTestRecord(&state);
    magnifier::getKeeper = keeper;
    magnifier::getOption = optionValue;
    magnifier::originalOption = cache;
    magnifier::setLevel = apply;
    magnifier::originalChange = direct;
    magnifier::originalZoomCommand = command;
    magnifier::originalMoveMouse = cursorWarp;
    magnifier::originalClipCursor = clipBackend;
    magnifier::nativeUi.store(reinterpret_cast<void*>(0x1000));
    magnifier::nativeSelf.store(reinterpret_cast<void*>(0x2000));
    state.controlEnabled = 1;
    state.magnifierReady = 1;
    state.maxZoomPercent = 1600;
    state.nativeLevel = 10000;
    magnifier::rendererThread.store(GetCurrentThreadId());
    magnifier::uiThread.store(GetCurrentThreadId());
    deliver(10010, 1);
    require(uiCalls == 1 && directWrites == 0 && percent == 101, "sub-percent first pinch primes tracking once above 100 percent");
    require(state.nativeLevel == 10100 && state.zoomRevision == 1, "priming publishes its actual target and requests one rebase");
    state.requestedLevel = 10040;
    state.requestedSequence = 2;
    state.requestRevision = 0;
    magnifier::messages(nullptr, magnifier::updateMessage, 0, 0);
    require(uiCalls == 1 && directWrites == 0 && state.nativeLevel == 10100, "pre-priming baseline cannot invert the next frame");
    deliver(12345, 3);
    require(directWrites == 1 && uiCalls == 1 && std::abs(lastFloat - 1.2345) < 0.00001,
            "intermediate frame uses one continuous native float update");
    require(state.nativeLevel == 12345 && state.appliedSequence == 3, "float target is published without integer quantization");
    deliver(12350, 4);
    require(directWrites == 2 && uiCalls == 1 && std::abs(lastFloat - 1.235) < 0.00001,
            "nearby fractional levels remain distinct");
    deliver(12350, 5);
    require(directWrites == 2 && uiCalls == 1 && state.appliedSequence == 5, "identical float target does not restart native work");
    require(persistedWrites == 0 && state.nativeCommands == 0, "pinch neither persists per-frame settings nor imitates keyboard commands");
    magnifier::option(nullptr, 14, 120, false);
    require(state.nativeCommands == 0 && state.zoomRevision == 1, "internal Windows option updates cannot rebase the pinch");
    magnifier::zoomCommand(nullptr, 0x800A);
    require(state.nativeCommands == 1 && state.nativeLevel == 22000 && state.zoomRevision == 2,
            "real native command publishes its target and rebases fingers");
    require(persistedWrites == 1, "real native commands keep their persistence behavior");
    state.requestRevision = 1;
    state.requestedSequence = 6;
    state.requestedLevel = 13000;
    magnifier::messages(nullptr, magnifier::updateMessage, 0, 0);
    require(directWrites == 2 && state.nativeLevel == 22000, "old touch baseline cannot overwrite a keyboard change");
    deliver(19000, 7);
    require(directWrites == 3 && uiCalls == 1, "touch resumes continuously after native keyboard command");
    state.controlEnabled = 0;
    deliver(15000, 8);
    require(directWrites == 3 && uiCalls == 1, "disabled controller cannot apply pending pinch work");
    state.pendingCommand = 3;
    magnifier::messages(nullptr, magnifier::updateMessage, 0, 0);
    require(uiCalls == 2 && directWrites == 3 && state.nativeLevel == 10000 && !magnifier::trackingInitialized.load(),
            "reset uses only native UI and clears tracking priming");
    state.controlEnabled = 1;
    deliver(10040, 9);
    require(uiCalls == 3 && directWrites == 3 && state.nativeLevel == 10100, "a later pinch primes once again without a second renderer write");
    magnifier::rendererThread.store(GetCurrentThreadId() + 1);
    deliver(14000, 10);
    require(uiCalls == 3 && directWrites == 3 && state.nativeError == ERROR_INVALID_THREAD_ID,
            "renderer is never called from an unexpected thread");
    magnifier::rendererThread.store(GetCurrentThreadId());
    deliver(14000, 11);
    require(directWrites == 4 && uiCalls == 3, "float updates recover on the correct thread");
    simulateReentrantClose = true;
    state.pendingCommand = 3;
    magnifier::messages(nullptr, magnifier::updateMessage, 0, 0);
    require(closeResult == 1 && magnifier::activeUpdates.load() == 0,
            "reentrant close reports an outstanding update before DLL unload");
    magnifier::stopping.store(true);
    deliver(18000, 12);
    require(uiCalls == 4 && directWrites == 4, "shutdown rejects later native messages");
    magnifier::stopping.store(false);
    magnifier::applyingPinch = true;
    magnifier::moveMouse(nullptr, POINT{10, 20});
    require(cursorWarps == 1, "synchronous pinch omits Magnifier cursor injection");
    magnifier::applyingPinch = false;
    state.enabled = 1;
    state.down = 1;
    state.phase = static_cast<LONG>(pinch::Phase::Pinch);
    state.heartbeat = static_cast<LONG64>(GetTickCount64());
    magnifier::moveMouse(nullptr, POINT{10, 20});
    require(cursorWarps == 1, "deferred cursor restoration is omitted while fingers remain down");
    magnifier::zoomCommand(nullptr, 0x800A);
    require(cursorWarps == 2, "native keyboard cursor behavior is preserved even while fingers remain down");
    state.down = 0;
    magnifier::moveMouse(nullptr, POINT{10, 20});
    require(cursorWarps == 3, "native cursor behavior resumes after lifting the fingers");
    state.down = 1;
    state.phase = static_cast<LONG>(pinch::Phase::Swipe);
    magnifier::moveMouse(nullptr, POINT{10, 20});
    require(cursorWarps == 4, "ordinary three-finger swipes are outside pointer protection");
    state.phase = static_cast<LONG>(pinch::Phase::Pinch);
    state.heartbeat = 0;
    magnifier::moveMouse(nullptr, POINT{10, 20});
    require(cursorWarps == 5, "a stale host cannot block native pointer behavior");
    state.heartbeat = static_cast<LONG64>(GetTickCount64());
    magnifier::keepPointerStable.store(false);
    magnifier::moveMouse(nullptr, POINT{10, 20});
    require(cursorWarps == 6, "pointer protection can be disabled independently");
    // A rebuilding renderer is not a process exit and must not lock the reader.
    state.magnifierPid = static_cast<LONG>(GetCurrentProcessId());
    state.magnifierReady = 0;
    require(magnifier::mapping.magnifierProcessAlive() && !magnifier::mapping.magnifierAvailable(),
            "renderer unavailability is distinct from native process exit");
    state.magnifierReady = 1;
    require(magnifier::mapping.magnifierAvailable(), "rebuilt renderer becomes available without restarting the gesture host");
    const LONG revisionBefore = state.zoomRevision;
    magnifier::change(reinterpret_cast<void*>(0x2000), 2.4f, nullptr);
    require(state.nativeLevel == 24000 && state.zoomRevision != revisionBefore,
            "native renderer changes rebase touch even when keyboard bypasses UI hooks");
    magnifier::originalZoomCommand = rendererCommand;
    magnifier::zoomCommand(reinterpret_cast<void*>(0x2000), 0x800A);
    require(state.nativeLevel == 37000, "a stale options cache cannot overwrite the renderer's native keyboard target");
    magnifier::originalZoomCommand = command;
    magnifier::readTransform = readView;
    magnifier::moveTo = moveView;
    state.nativeCaps = compatibility::NativeCore | compatibility::NativeViewportRestore | compatibility::NativeKeyboardObserver;
    state.gateCaps = compatibility::GateSwipe | compatibility::GateVisibility;
    state.normalInOverview = 1;
    state.controlEnabled = 1;
    state.pendingGestures = 0;
    state.shellMask = 1;
    state.shellRevision = 1;
    visibleView = {2.75, 123, 45};
    state.nativeLevel = 27500;
    require(magnifier::updateOverview() && magnifier::overviewSuspended.load() && visibleView.zoom == 1,
            "Task View temporarily shows the global desktop at 100 percent");
    const int enteredWrites = directWrites;
    state.pendingGestures = 1;
    magnifier::updateOverview();
    require(magnifier::overviewSuspended.load() && directWrites == enteredWrites && magnifier::overviewView->zoom == 2.75,
            "a new undecided gesture cannot restore zoom while Task View remains visible");
    state.pendingGestures = 0;
    state.shellMask = 3;
    state.shellRevision = 2;
    magnifier::updateOverview();
    require(directWrites == enteredWrites && magnifier::overviewView->zoom == 2.75,
            "nested desktop and Task View reasons never overwrite the saved view");
    deliver(45000, 100);
    require(visibleView.zoom == 1 && directWrites == enteredWrites,
            "overview remains at 100 percent while stale touch arrives");
    state.shellMask = 2;
    state.shellRevision = 3;
    magnifier::updateOverview();
    require(magnifier::overviewSuspended.load() && visibleView.zoom == 1, "closing only one overview reason does not restore too early");
    state.shellMask = 0;
    state.shellRevision = 4;
    magnifier::updateOverview();
    require(!magnifier::overviewSuspended.load() && visibleView.zoom == 2.75 && visibleView.x == 123 && visibleView.y == 45 && cameraMoves == 1 && restoredTarget,
            "closing the last overview restores the same global level and position");
    state.pendingGestures = 1;
    state.shellMask = 1;
    state.shellRevision = 5;
    const int beforeAmbiguous = directWrites;
    magnifier::updateOverview();
    require(directWrites == beforeAmbiguous && !magnifier::overviewSuspended.load(),
            "an undecided gesture cannot flash zoom through a provisional shell surface");
    state.shellMask = 0;
    state.pendingGestures = 0;
    magnifier::updateOverview();
    require(directWrites == beforeAmbiguous, "cancelling a provisional pinch leaves the global view untouched");
    magnifier::keepPointerStable.store(true);
    magnifier::applyingPinch = true;
    const int releasesBefore = clipReleases;
    RECT limiting{1, 2, 3, 4};
    magnifier::clipCursor(&limiting);
    require(clipLimits == 0 && clipReleases == releasesBefore + 1,
            "slow pinch clipping cannot nudge the pointer into shrinking bounds");
    magnifier::applyingPinch = false;
    state.enabled = 1;
    state.down = 1;
    state.phase = static_cast<LONG>(pinch::Phase::Pinch);
    state.heartbeat = static_cast<LONG64>(GetTickCount64());
    magnifier::clipCursor(&limiting);
    require(clipLimits == 0, "deferred clip updates are released while fingers remain down");
    { magnifier::NativeCommandScope commandScope; magnifier::clipCursor(&limiting); }
    require(clipLimits == 1, "native keyboard clipping is preserved during a pinch hold");
    state.down = 0;
    magnifier::clipCursor(&limiting);
    require(clipLimits == 2, "ordinary native cursor bounds resume after the pinch");
    magnifier::nativeUi.store(nullptr);
    magnifier::uiLevel(reinterpret_cast<void*>(0x1000), 150);
    require(magnifier::nativeUi.load() == reinterpret_cast<void*>(0x1000),
            "a native keyboard or UI command can recover a context injected after startup");
    magnifier::originalWheel = nativeWheel;
    state.hostWindow = 0x1234;
    state.wheelEnabled = 1; state.controlEnabled = 1; state.enabled = 1;
    state.heartbeat = static_cast<LONG64>(GetTickCount64()); state.shellMask = 0;
    state.inputRevision = 42;
    magnifier::wheel(reinterpret_cast<void*>(0x1000), -30);
    require(pinch::wheelDelta(state.wheelPacket) == -30 && pinch::wheelRevision(state.wheelPacket) == 42 && originalWheelCalls == 0,
            "native wheel stores signed high-resolution input without a second native zoom");
    state.enabled = 0;
    magnifier::wheel(nullptr, 30);
    require(pinch::wheelDelta(state.wheelPacket) == 30 && originalWheelCalls == 0,
            "mouse-only wheel input works after reset and discards the previous queue direction");
    state.enabled = 1; state.wheelEnabled = 0;
    magnifier::wheel(nullptr, 120);
    require(originalWheelCalls == 1 && pinch::wheelDelta(state.wheelPacket) == 30, "wheel opt-out restores native shortcut behavior");
    state.wheelEnabled = 1; state.hostWindow = 0;
    magnifier::wheel(nullptr, 120);
    require(originalWheelCalls == 2, "an absent host window preserves native behavior");
    state.hostWindow = 0x1234; state.heartbeat = static_cast<LONG64>(GetTickCount64() - 500);
    magnifier::wheel(nullptr, 120);
    require(originalWheelCalls == 3, "an unavailable host cannot swallow the native wheel shortcut");
    state.heartbeat = static_cast<LONG64>(GetTickCount64());
    const auto queuedBeforeOverview = state.wheelPacket;
    state.shellMask = 1; magnifier::wheel(nullptr, 120);
    require(state.wheelPacket == queuedBeforeOverview && originalWheelCalls == 3, "wheel cannot magnify the overview");
    state.shellMask = 0;
    state.wheelActive = 1; state.down = 0;
    const int warpsBeforeWheel = cursorWarps, clipsBeforeWheel = clipLimits;
    magnifier::moveMouse(nullptr, POINT{10, 20}); magnifier::clipCursor(&limiting);
    require(cursorWarps == warpsBeforeWheel && clipLimits == clipsBeforeWheel,
            "deferred pointer warps and clipping are suppressed between wheel animation ticks");
    const LONG beforeWheelKey = state.inputRevision;
    magnifier::zoomCommand(reinterpret_cast<void*>(0x1000), 0x800A);
    require(state.inputRevision != beforeWheelKey && !state.wheelActive && cursorWarps == warpsBeforeWheel + 1,
            "native keyboard cancels pending wheel epoch and retains native pointer behavior");
    const LONG beforeExternal = state.inputRevision;
    magnifier::change(reinterpret_cast<void*>(0x2000), 4.2f, nullptr);
    require(state.inputRevision != beforeExternal, "external renderer changes invalidate old wheel input");
    magnifier::originalUiWindowProc = windowBackend;
    magnifier::originalFullWindowProc = windowBackend;
    magnifier::windowData = existingObject;
    magnifier::nativeUi.store(nullptr); magnifier::nativeSelf.store(nullptr);
    magnifier::messageWindow.store(reinterpret_cast<HWND>(0x5678));
    visibleView = {2.731, 50, 60};
    const int writesBeforeRecovery = directWrites;
    require(magnifier::uiWindowProc(nullptr, WM_NULL, 0, 0) == 321 &&
            magnifier::nativeUi.load() == reinterpret_cast<void*>(0x6789),
            "a pre-existing UI window recovers its context without a synthetic zoom command");
    require(magnifier::fullWindowProc(nullptr, WM_NULL, 0, 0) == 321 &&
            magnifier::nativeSelf.load() == reinterpret_cast<void*>(0x6789) && state.magnifierReady && state.nativeLevel == 27310 &&
            directWrites == writesBeforeRecovery && visibleView.x == 50 && visibleView.y == 60,
            "a pre-existing renderer is adopted without rounding its factor or moving its viewport");
    const LONG recoveredEpoch = state.inputRevision;
    magnifier::fullWindowProc(nullptr, WM_NULL, 0, 0);
    require(state.inputRevision == recoveredEpoch, "repeated harmless window messages do not reset wheel or pinch motion");
    magnifier::messageWindow.store(nullptr);
    state.magnifierReady = 1; state.controlEnabled = 1; state.requestWheel = 1;
    state.nativeLevel = 10000; percent = 100; visibleView = {1, 0, 0};
    magnifier::trackingInitialized.store(false); magnifier::nativeSelf.store(reinterpret_cast<void*>(0x2000));
    const int uiBeforeFine = uiCalls, writesBeforeFine = directWrites;
    deliver(10001, 500);
    require(uiCalls == uiBeforeFine + 1 && directWrites == writesBeforeFine + 1 && state.nativeLevel == 10001 &&
            std::abs(visibleView.zoom - 1.0001) < 0.000001 && percent == 100,
            "first tiny wheel delta primes native tracking once and delivers its fine target in the same UI turn");
    deliver(10002, 501);
    require(uiCalls == uiBeforeFine + 1 && directWrites == writesBeforeFine + 2 && state.nativeLevel == 10002,
            "a second tiny wheel delta does not prime native UI again");
    state.requestWheel = 0;
    InterlockedExchange64(&state.wheelPacket, 0);
    for (int i=0; i<10000; ++i) bridge::enqueueWheel(&state, 1, state.inputRevision, 1000);
    require(pinch::wheelDelta(state.wheelPacket) == 1200, "producer burst remains bounded without posting a message per delta");
    bridge::enqueueWheel(&state, -1, state.inputRevision, 1001);
    require(pinch::wheelDelta(state.wheelPacket) == -1, "producer reversal discards its old accumulated direction");
    const auto queued = state.wheelPacket;
    bridge::enqueueWheel(&state, 120, state.inputRevision - 1, 1002);
    require(state.wheelPacket == queued, "stale producer revision cannot enter the mailbox");
    bridge::enqueueWheel(&state, 30, state.inputRevision, 1303);
    require(pinch::wheelDelta(state.wheelPacket) == 30, "a new producer event cannot revive stale accumulated input");
    visibleView = {2.75, 20, 30}; state.shellMask = 1; state.overviewBypassed = 0;
    magnifier::updateOverview();
    require(magnifier::overviewSuspended.load(), "overview starts suspended before native keyboard override");
    const LONG nativeKeysBefore = state.nativeCommands;
    magnifier::zoomCommand(reinterpret_cast<void*>(0x6789), 0x800A);
    require(!magnifier::overviewSuspended.load() && state.nativeCommands > nativeKeysBefore && state.overviewBypassed,
            "an explicit native keyboard command cannot be swallowed by a stale overview signal");
    magnifier::updateOverview();
    require(!magnifier::overviewSuspended.load(), "overview cannot immediately recapture the keyboard override");
    state.shellMask = 0; magnifier::updateOverview();
    require(!state.overviewBypassed, "closing overview clears its temporary native-keyboard override");
    state.shellMask = 1; state.overviewBypassed = 0;
    const int writesBeforeQueuedOverview = directWrites;
    magnifier::zoomCommand(reinterpret_cast<void*>(0x6789), 0x800A);
    require(state.overviewBypassed, "explicit native key overrides even an overview suspension still pending in the worker");
    magnifier::updateOverview();
    require(!magnifier::overviewSuspended.load() && directWrites == writesBeforeQueuedOverview,
            "a queued overview update cannot undo a native keyboard command");
    state.shellMask = 0; magnifier::updateOverview();
    std::cout << checks << " native adapter checks passed; fake backend; desktop unchanged\n";
}
