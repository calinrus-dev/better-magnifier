// Exercise the production native WndProc with an in-memory protocol and fake
// Windows Magnifier callbacks. Never opens Magnifier or changes desktop zoom.
#define PINCH_ZOOM_STANDALONE
#define PINCH_ZOOM_NATIVE_TEST
#define wmain diagnostic_main
#include "../mods/kde-pinch-zoom.wh.cpp"
#undef wmain
#include <iostream>
#include <cstring>

namespace {
bridge::Record state{};
long percent = 100;
long currentMode = 2;
bool captureRendererFromUi = false;
int nullRendererWrites = 0;
int nativeReadPointCalls = 0, nativeReadRegistryCalls = 0, nativeReadDefaultCalls = 0;
const void* readingObject = nullptr;
void nativeReadPoint(void* self) { ++nativeReadPointCalls; readingObject = self; }
bool nativeReadRegistry(const void* self) { ++nativeReadRegistryCalls; readingObject = self; return false; }
bool nativeReadDefault(const void* self) { ++nativeReadDefaultCalls; readingObject = self; return true; }
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
long optionValue(void*, int name) { return name == 14 ? percent : name == 20 ? 100 : name == 4 ? currentMode : 2; }
void cache(void*, int name, long value, bool save) {
    if (name == 14) percent = value;
    if (name == 4) currentMode = value;
    if (save) ++persistedWrites;
}
void apply(void* self, long value) {
    ++uiCalls;
    visibleView.zoom = value / 100.0;
    if (simulateReentrantClose) closeResult = magnifier::messages(nullptr, WM_CLOSE, 0, 0);
    // Native UI requests persistence; the mod must suppress that for a pinch.
    magnifier::option(self, 14, value, true);
    if (captureRendererFromUi) magnifier::change(reinterpret_cast<void*>(0x2000), value / 100.0f, nullptr);
}
void direct(void* self, float value, POINT*) { ++directWrites; if (!self) ++nullRendererWrites; lastFloat = value; visibleView.zoom = value; if (value <= 1) { visibleView.x = 0; visibleView.y = 0; } }
void command(void* self, int) { magnifier::moveMouse(self, POINT{10, 20}); magnifier::option(self, 14, percent + 100, true); }
void cursorWarp(void*, POINT) { ++cursorWarps; }
void deliver(LONG target, LONG sequence, bridge::InputSource source = bridge::InputSource::Pinch) {
    state.requestRevision = state.zoomRevision;
    state.requestInputRevision = state.inputRevision;
    state.requestSource = static_cast<LONG>(source);
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
    magnifier::messageWindow.store(reinterpret_cast<HWND>(0x5678));
    magnifier::keyboardKillTimer = [](HWND, UINT_PTR) -> BOOL { return TRUE; };
    magnifier::keyboardKeyState = [](int) -> SHORT { return 0; };
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
    deliver(10100, 1);
    require(uiCalls == 1 && directWrites == 0 && percent == 101, "first pinch primes native tracking once above 100 percent");
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
    deliver(10100, 9);
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
    require(state.overviewSavedLevel == 27500,
            "overview publishes the previous global zoom for all explicit input producers");
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
    state.requestedLevel = 45000; state.requestedSequence = 100;
    state.requestSource = static_cast<LONG>(bridge::InputSource::Pinch);
    state.requestInputRevision = state.inputRevision;
    state.requestRevision = state.zoomRevision - 1;
    magnifier::messages(nullptr, magnifier::updateMessage, 0, 0);
    require(visibleView.zoom == 1 && directWrites == enteredWrites,
            "overview remains at 100 percent while stale touch arrives");
    state.shellMask = 2;
    state.shellRevision = 3;
    magnifier::updateOverview();
    require(!magnifier::overviewSuspended.load() && visibleView.zoom == 2.75,
            "closing Task View restores zoom even while the static desktop remains raised");
    state.shellMask = 0;
    state.shellRevision = 4;
    magnifier::updateOverview();
    require(!magnifier::overviewSuspended.load() && visibleView.zoom == 2.75 && visibleView.x == 123 && visibleView.y == 45 && cameraMoves == 1 && restoredTarget,
            "desktop telemetry cannot replace the same global level and position");
    require(state.overviewSavedLevel == 0, "restoring the actual native view clears its published overview baseline");
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
    state.winWheel = 1; state.wheelWindowSwitch = 1; state.mouseHooksReady = 0;
    state.wheelPacket = 0; state.windowWheelPacket = 0;
    magnifier::wheel(nullptr, -30);
    require(pinch::wheelDelta(state.wheelPacket) == -30 && !state.windowWheelPacket && originalWheelCalls == 0,
            "an unavailable Win mouse broker retains Ctrl Alt wheel zoom instead of stealing it for window switching");
    state.winWheel = 0; state.wheelWindowSwitch = 0;
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
    require(state.wheelPacket != queuedBeforeOverview && state.overviewBypassed && originalWheelCalls == 3,
            "an explicit managed wheel event can override a pending overview without a duplicate native zoom");
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
    magnifier::messageWindow.store(reinterpret_cast<HWND>(0x5678));
    state.magnifierReady = 1; state.controlEnabled = 1;
    state.nativeLevel = 10000; percent = 100; visibleView = {1, 0, 0};
    magnifier::trackingInitialized.store(false); magnifier::nativeSelf.store(reinterpret_cast<void*>(0x2000));
    const int uiBeforeFine = uiCalls, writesBeforeFine = directWrites;
    deliver(10001, 500, bridge::InputSource::Wheel);
    require(uiCalls == uiBeforeFine + 1 && directWrites == writesBeforeFine + 1 && state.nativeLevel == 10001 &&
            std::abs(visibleView.zoom - 1.0001) < 0.000001 && percent == 100,
            "first tiny wheel delta primes native tracking once and delivers its fine target in the same UI turn");
    deliver(10002, 501, bridge::InputSource::Wheel);
    require(uiCalls == uiBeforeFine + 1 && directWrites == writesBeforeFine + 2 && state.nativeLevel == 10002,
            "a second tiny wheel delta does not prime native UI again");
    state.requestSource = static_cast<LONG>(bridge::InputSource::None);
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
    state.shellMask = 2; state.overviewBypassed = 0;
    const int writesOnStaticDesktop = directWrites;
    visibleView = {2.25, 45, 60}; state.nativeLevel = 22500;
    magnifier::updateOverview();
    require(!magnifier::overviewSuspended.load() && directWrites == writesOnStaticDesktop && visibleView.zoom == 2.25,
            "static desktop state does not suspend or reset its global zoom");

    const auto prepareCold = [&] {
        state = {};
        state.controlEnabled = 1; state.maxZoomPercent = 1600; state.nativeLevel = 10000;
        magnifier::nativeUi.store(reinterpret_cast<void*>(0x1000));
        magnifier::nativeSelf.store(nullptr);
        magnifier::messageWindow.store(reinterpret_cast<HWND>(0x5678));
        magnifier::uiThread.store(GetCurrentThreadId()); magnifier::rendererThread.store(0);
        magnifier::uiControlReady.store(false); magnifier::uiPrimeBlocked.store(false);
        magnifier::trackingInitialized.store(false); magnifier::overviewSuspended.store(false);
        magnifier::overviewView.reset(); magnifier::stopping.store(false);
        magnifier::setLevel = apply; magnifier::originalChange = direct;
        currentMode = 2; percent = 100; visibleView = {1, 0, 0};
        captureRendererFromUi = false; simulateReentrantClose = false;
        uiCalls = directWrites = persistedWrites = nullRendererWrites = 0;
        lastFloat = 1;
    };
    prepareCold(); magnifier::ensureUi(reinterpret_cast<void*>(0x1000));
    require(state.magnifierReady && !magnifier::nativeSelf,
            "cold fullscreen UI accepts native control before any renderer callback");
    captureRendererFromUi = true; state.startAtNormal = 1;
    deliver(10010, 600, bridge::InputSource::Wheel);
    require(uiCalls == 1 && !nullRendererWrites && magnifier::nativeSelf == reinterpret_cast<void*>(0x2000),
            "cold tiny wheel primes UI once and captures its real fullscreen renderer");
    require(state.nativeLevel == 10010 && std::abs(lastFloat - 1.001) < 0.000001 && !state.startAtNormal,
            "cold normalization cannot swallow the first precise wheel target");
    require(!persistedWrites && state.appliedSequence == 600 && !state.nativeError,
            "cold target is acknowledged only after native delivery without registry writes");
    require(state.currentSource == static_cast<LONG>(bridge::InputSource::Wheel),
            "successful cold wheel delivery publishes its actor without relying on a previous pinch");
    deliver(12500, 601, bridge::InputSource::Wheel);
    require(uiCalls == 1 && std::abs(lastFloat - 1.25) < 0.000001,
            "later wheel updates retain the established continuous float renderer path");
    prepareCold(); captureRendererFromUi = true; magnifier::ensureUi(reinterpret_cast<void*>(0x1000));
    deliver(10010, 610);
    require(uiCalls == 1 && state.nativeLevel == 10010 && magnifier::trackingInitialized &&
            std::abs(visibleView.zoom - 1.001) < 0.000001,
            "cold pinch uses native tracking priming while retaining its precise first target");
    require(state.currentSource == static_cast<LONG>(bridge::InputSource::Pinch),
            "cold pinch owns its first successful native objective");
    prepareCold(); magnifier::ensureUi(reinterpret_cast<void*>(0x1000));
    deliver(12000, 620);
    require(uiCalls == 1 && !directWrites && !nullRendererWrites && !state.magnifierReady && state.nativeError == ERROR_RETRY,
            "a cold UI without renderer capture fails closed without dereferencing a null object");
    require(state.appliedSequence == 0, "failed cold prime leaves its sequence unacknowledged");
    magnifier::ensureUi(reinterpret_cast<void*>(0x1000)); deliver(12500, 621);
    require(uiCalls == 1 && !state.appliedSequence && !state.magnifierReady,
            "harmless UI rediscovery and pending messages never create an infinite prime retry loop");
    magnifier::change(reinterpret_cast<void*>(0x2000), 1.2f, nullptr);
    require(state.magnifierReady && !magnifier::uiPrimeBlocked && !state.nativeError,
            "a later genuine renderer callback recovers readiness after the first cold prime");
    prepareCold(); currentMode = 3; magnifier::ensureUi(reinterpret_cast<void*>(0x1000));
    deliver(12000, 630);
    require(!state.magnifierReady && !uiCalls && !directWrites && state.nativeError == ERROR_NOT_SUPPORTED,
            "Lens mode cannot advertise fullscreen readiness or receive a replacement transform");
    prepareCold(); magnifier::ensureUi(reinterpret_cast<void*>(0x1000)); currentMode = 1;
    magnifier::ensureUi(reinterpret_cast<void*>(0x1000));
    require(!state.magnifierReady, "existing UI endpoint is revalidated when the native mode becomes docked");
    currentMode = 2; magnifier::ensureUi(reinterpret_cast<void*>(0x1000));
    require(state.magnifierReady, "returning to fullscreen permits a fresh owner-thread UI prime");
    prepareCold(); magnifier::ensureUi(reinterpret_cast<void*>(0x1000)); state.pendingCommand = 3;
    magnifier::messages(nullptr, magnifier::updateMessage, 0, 0);
    require(uiCalls == 1 && percent == 100 && !nullRendererWrites && !state.nativeError,
            "native UI reset at cold 100 percent is safe without a fullscreen renderer");
    require(state.currentSource == static_cast<LONG>(bridge::InputSource::Reset),
            "explicit reset publishes a reset actor even without a fullscreen renderer");
    prepareCold(); magnifier::uiThread.store(GetCurrentThreadId() + 1); deliver(12000, 640);
    require(!uiCalls && !directWrites && state.nativeError == ERROR_INVALID_THREAD_ID,
            "cold UI priming cannot execute from a foreign thread");
    prepareCold(); magnifier::nativeSelf.store(reinterpret_cast<void*>(0x2000));
    magnifier::rendererThread.store(GetCurrentThreadId() + 1); deliver(12000, 650);
    require(!uiCalls && !directWrites && state.nativeError == ERROR_INVALID_THREAD_ID,
            "a captured foreign renderer is never used as a bootstrap shortcut");
    prepareCold(); captureRendererFromUi = true; magnifier::ensureUi(reinterpret_cast<void*>(0x1000));
    deliver(27500, 700);
    state.nativeCaps = compatibility::NativeCore | compatibility::NativeViewportRestore | compatibility::NativeKeyboardObserver;
    state.gateCaps = compatibility::GateSwipe | compatibility::GateVisibility;
    state.normalInOverview = 1; state.pendingGestures = 0; state.shellMask = 1;
    visibleView = {2.75, 123, 45};
    magnifier::updateOverview();
    require(magnifier::overviewSuspended && state.overviewSavedLevel == 27500 && visibleView.zoom == 1,
            "manual-override fixture starts with one saved global view and native 100-percent overview");
    state.currentSource = static_cast<LONG>(bridge::InputSource::Keyboard);
    const int beforeStaleEpoch = directWrites;
    state.requestedLevel = 45000; state.requestedSequence = 701;
    state.requestSource = static_cast<LONG>(bridge::InputSource::Pinch);
    state.requestRevision = state.zoomRevision; state.requestInputRevision = state.inputRevision - 1;
    magnifier::messages(nullptr, magnifier::updateMessage, 0, 0);
    require(magnifier::overviewSuspended && state.overviewSavedLevel == 27500 && directWrites == beforeStaleEpoch &&
            state.currentSource == static_cast<LONG>(bridge::InputSource::Keyboard),
            "an old input epoch cannot dismiss overview or steal the newer actor despite matching zoom revision");
    deliver(31000, 702);
    require(!magnifier::overviewSuspended && state.overviewBypassed && state.nativeLevel == 31000 &&
            std::abs(visibleView.zoom - 3.1) < 0.000001 && state.appliedSequence == 702,
            "fresh pinch overrides overview and survives epochs advanced by restoring its saved baseline");
    require(state.overviewSavedLevel == 0 && state.currentSource == static_cast<LONG>(bridge::InputSource::Pinch) &&
            visibleView.x == 123 && visibleView.y == 45,
            "manual pinch retains global viewport position and publishes its actor after the restore");
    state.overviewBypassed = 0; visibleView = {2.5, 78, 90}; state.nativeLevel = 25000;
    magnifier::updateOverview();
    require(magnifier::overviewSuspended && state.overviewSavedLevel == 25000,
            "a later overview can save a newer global view");
    const int cameraBeforeExplicitReset = cameraMoves;
    state.pendingCommand = 3;
    magnifier::messages(nullptr, magnifier::updateMessage, 0, 0);
    require(!magnifier::overviewSuspended && !state.overviewSavedLevel && state.nativeLevel == 10000 && visibleView.zoom == 1 &&
            cameraMoves == cameraBeforeExplicitReset && state.currentSource == static_cast<LONG>(bridge::InputSource::Reset),
            "explicit reset cancels the temporary overview without flashing back to its old magnified camera");
    state.overviewBypassed = 0; visibleView = {2.5, 78, 90}; state.nativeLevel = 25000;
    magnifier::updateOverview();
    state.wheelActive = 1;
    const LONG managedWheelEpoch = state.inputRevision;
    deliver(30000, 703, bridge::InputSource::Wheel);
    require(!magnifier::overviewSuspended && state.inputRevision == managedWheelEpoch && state.wheelActive && state.nativeLevel == 30000,
            "a validated managed wheel objective restores overview without cancelling its own trajectory epoch");
    deliver(32000, 704, bridge::InputSource::Wheel);
    require(state.inputRevision == managedWheelEpoch && state.nativeLevel == 32000 &&
            state.currentSource == static_cast<LONG>(bridge::InputSource::Wheel),
            "the second managed wheel frame continues after overview restoration");
    state.overviewBypassed = 0; visibleView = {3.2, 78, 90}; state.nativeLevel = 32000;
    magnifier::updateOverview();
    const LONG automaticRestoreEpoch = state.inputRevision;
    state.wheelActive = 1; state.shellMask = 0;
    magnifier::updateOverview();
    require(state.inputRevision != automaticRestoreEpoch && !state.wheelActive,
            "automatic shell restoration still invalidates earlier managed trajectories");

    magnifier::originalRegisterReadPoint = nativeReadPoint;
    magnifier::originalReadPointRegistry = nativeReadRegistry;
    magnifier::originalReadPointDefault = nativeReadDefault;
    void* const focusObject = reinterpret_cast<void*>(0x9876);
    state.disableReading = 1; magnifier::readingHooksReady.store(true);
    magnifier::registerReadPoint(focusObject);
    require(!nativeReadPointCalls && !magnifier::readPointRegistry(focusObject) && !magnifier::readPointDefault(focusObject) &&
            !nativeReadRegistryCalls && !nativeReadDefaultCalls,
            "enabled read-from-point suppression never arms capture or asks Windows to replace the cursor");
    state.disableReading = 0;
    magnifier::registerReadPoint(focusObject);
    require(nativeReadPointCalls == 1 && !magnifier::readPointRegistry(focusObject) && magnifier::readPointDefault(focusObject) &&
            nativeReadRegistryCalls == 1 && nativeReadDefaultCalls == 1 && readingObject == focusObject,
            "turning the setting off forwards native reading calls, return values and object pointers unchanged");
    state.disableReading = 1; magnifier::readingHooksReady.store(false);
    magnifier::registerReadPoint(focusObject);
    require(nativeReadPointCalls == 2 && magnifier::readPointDefault(focusObject) && nativeReadDefaultCalls == 2,
            "an absent or incomplete optional group preserves native reading behavior");
    magnifier::readingHooksReady.store(true); magnifier::stopping.store(true);
    magnifier::registerReadPoint(focusObject);
    require(nativeReadPointCalls == 3 && magnifier::readPointDefault(focusObject),
            "unloading returns control to native reading without restoring bindings or replacing cursors");
    magnifier::stopping.store(false);
    const LONG commandsBeforeReadingGuard = state.nativeCommands;
    magnifier::zoomCommand(reinterpret_cast<void*>(0x1000), 0x800a);
    require(state.nativeCommands > commandsBeforeReadingGuard,
            "reading suppression does not intercept native zoom commands");
    magnifier::readingHooksReady.store(false);
    std::array<void*, 3> readingTargets{};
    require(!magnifier::resolveReadingTargets(nullptr, readingTargets), "missing optional reading image fails closed without hooks");
    const auto& readingProfile = binary_profiles::profiles[0];
    std::vector<BYTE> readingImage(readingProfile.identity.imageSize);
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(readingImage.data());
    dos->e_magic = IMAGE_DOS_SIGNATURE; dos->e_lfanew = 0x80;
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(readingImage.data() + dos->e_lfanew);
    nt->Signature = IMAGE_NT_SIGNATURE; nt->FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
    nt->FileHeader.NumberOfSections = 1; nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
    nt->FileHeader.TimeDateStamp = readingProfile.identity.timestamp;
    nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    nt->OptionalHeader.SizeOfImage = readingProfile.identity.imageSize; nt->OptionalHeader.SizeOfHeaders = 0x1000;
    nt->OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG] = {0x100000, sizeof(IMAGE_DEBUG_DIRECTORY)};
    auto* section = IMAGE_FIRST_SECTION(nt);
    section->VirtualAddress = 0x1000; section->Misc.VirtualSize = 0xc0000; section->Characteristics = IMAGE_SCN_MEM_EXECUTE;
    auto* debug = reinterpret_cast<IMAGE_DEBUG_DIRECTORY*>(readingImage.data() + 0x100000);
    debug->Type = IMAGE_DEBUG_TYPE_CODEVIEW; debug->SizeOfData = 24; debug->AddressOfRawData = 0x100080;
    auto* codeView = readingImage.data() + 0x100080;
    std::memcpy(codeView, "RSDS", 4);
    std::copy(readingProfile.identity.guid.begin(), readingProfile.identity.guid.end(), codeView + 4);
    std::memcpy(codeView + 20, &readingProfile.identity.age, sizeof(DWORD));
    const auto readingModule = reinterpret_cast<HMODULE>(readingImage.data());
    require(magnifier::resolveReadingTargets(readingModule, readingTargets) && readingTargets[0] && readingTargets[1] && readingTargets[2],
            "reading entrypoints require the exact PDB and PE identity plus executable addresses");
    ++codeView[20];
    require(!magnifier::resolveReadingTargets(readingModule, readingTargets) && !readingTargets[0] && !readingTargets[1] && !readingTargets[2],
            "a changed reading ABI identity cannot silently reuse the private offsets");
    --codeView[20]; section->VirtualAddress = 0x72840; section->Misc.VirtualSize = 0x10;
    require(!magnifier::resolveReadingTargets(readingModule, readingTargets) && !readingTargets[0],
            "a partial optional reading group never activates capture-only or cursor-only interception");
    currentMode = 3; state.winWheel = 1; state.wheelWindowSwitch = 1;
    InterlockedExchange64(&state.windowWheelPacket, 0);
    const int wheelBeforeLens = originalWheelCalls;
    magnifier::wheel(reinterpret_cast<void*>(0x1000), 120);
    require(state.nativeViewMode == 3 && originalWheelCalls == wheelBeforeLens + 1 && !state.windowWheelPacket,
            "native Lens wheel zoom is preserved instead of becoming a window-switch action");
    magnifier::uiThread.store(GetCurrentThreadId()); magnifier::rendererThread.store(GetCurrentThreadId());
    magnifier::messageWindow.store(nullptr);
    std::cout << checks << " native adapter checks passed; fake backend; desktop unchanged\n";
}
