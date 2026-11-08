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
int uiCalls = 0, directWrites = 0, persistedWrites = 0;
bool simulateReentrantClose = false;
LRESULT closeResult = 0;
double lastFloat = 1;
HRESULT keeper(void** result) { *result = reinterpret_cast<void*>(0x1000); return S_OK; }
long optionValue(void*, int name) { return name == 14 ? percent : name == 20 ? 100 : 2; }
void cache(void*, int name, long value, bool save) {
    if (name == 14) percent = value;
    if (save) ++persistedWrites;
}
void apply(void* self, long value) {
    ++uiCalls;
    if (simulateReentrantClose) closeResult = magnifier::messages(nullptr, WM_CLOSE, 0, 0);
    // Native UI requests persistence; the mod must suppress that for a pinch.
    magnifier::option(self, 14, value, true);
}
void direct(void*, float value, POINT*) { ++directWrites; lastFloat = value; }
void command(void* self, int) { magnifier::option(self, 14, percent + 100, true); }
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
    std::cout << checks << " native adapter checks passed; fake backend; desktop unchanged\n";
}
