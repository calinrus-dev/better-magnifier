// Test the real shell adapter with fake callbacks. No window activation or zoom.
#define PINCH_ZOOM_STANDALONE
#define PINCH_ZOOM_NATIVE_TEST
#define PINCH_ZOOM_GATE_TEST
#define wmain diagnostic_main
#include "../mods/kde-pinch-zoom.wh.cpp"
#undef wmain
#include <iostream>

namespace {
bridge::Record record{};
std::string calls;
UINT32 contacts = 3;
int lastDirection = 0;
HRESULT startResult = S_OK, continueResult = S_OK;
BOOL WINAPI frame(UINT32, UINT32* count, POINTER_INFO*) { *count = contacts; return contacts ? TRUE : FALSE; }
HRESULT start(void*, int, int, unsigned, native_gate::Context) { calls += 'S'; return startResult; }
HRESULT step(void*, int, int, int, int, unsigned) { calls += 'C'; return continueResult; }
HRESULT finish(void*, int, int, int, int, unsigned) { calls += 'F'; return S_OK; }
HRESULT cancel(void*) { calls += 'X'; return S_OK; }
bool activate(int direction) { calls += 'J'; lastDirection = direction; return true; }
void visibility(void*, int) {}
void* const object = reinterpret_cast<void*>(0x1234);
void prepare(pinch::Phase phase, UINT32 count = 3, bool down = true) {
    native_gate::states.clear();
    record.phase = static_cast<LONG>(phase);
    record.down = down;
    record.enabled = 1;
    record.heartbeat = static_cast<LONG64>(GetTickCount64());
    record.instantSwitch = 1;
    record.swipeDirection = -2;
    record.pendingGestures = 0;
    contacts = count;
    startResult = S_OK; continueResult = S_OK;
    calls.clear();
}
void begin() { native_gate::start(object, 10, 20, 1, {}); }
void move() { native_gate::continueSwipe(object, 10, 30, 0, 1, 1); }
void end() { native_gate::finish(object, 10, 30, 0, 1, 1); }
}

int wmain() {
    int checks = 0;
    const auto require = [&](bool valid, const char* name) {
        if (!valid) { std::cerr << "FAIL " << name << ": " << calls << '\n'; std::exit(1); }
        ++checks;
    };
    native_gate::mapping.useTestRecord(&record);
    native_gate::readPointerFrame = frame;
    native_gate::originalStart = start;
    native_gate::originalContinue = step;
    native_gate::originalFinish = finish;
    native_gate::originalCancel = cancel;
    native_gate::switchWindow = activate;
    native_gate::originalShown = visibility;
    native_gate::originalDismissed = visibility;
    prepare(pinch::Phase::Pending);
    begin();
    require(calls == "S" && record.pendingGestures == 1, "native vertical animation starts at its original beginning");
    move();
    require(calls == "S", "undecided continuation does not animate a guessed swipe");
    record.phase = static_cast<LONG>(pinch::Phase::Swipe);
    move(); end();
    require(calls == "SCF" && record.pendingGestures == 0, "vertical transition never replays StartSwipe late");
    prepare(pinch::Phase::Pinch, 0);
    begin(); move(); end();
    require(calls.empty(), "raw pinch blocks ghost shell view when pointer-frame query is empty");
    prepare(pinch::Phase::Pending, 0, false);
    begin();
    require(calls.empty() && record.pendingGestures == 1, "unknown contacts remain provisional instead of opening ghost Task View");
    record.down = 1;
    move();
    require(calls.empty() && record.pendingGestures == 1, "the intervening raw Pending frame cannot animate or release overview suspension");
    record.phase = static_cast<LONG>(pinch::Phase::Pinch);
    move(); move(); end();
    require(calls.empty() && record.pendingGestures == 0, "late raw pinch never opens a native view when contact count began unknown");
    prepare(pinch::Phase::Pending);
    begin();
    record.phase = static_cast<LONG>(pinch::Phase::Pinch);
    move(); move(); end();
    require(calls == "SX", "a late pinch cancels an already started native gesture exactly once");
    prepare(pinch::Phase::Swipe);
    record.swipeDirection = -1;
    begin(); move(); move(); end();
    require(calls == "J" && lastDirection == -1, "direct lateral gesture switches once without starting a native selector");
    prepare(pinch::Phase::Pending);
    begin();
    record.phase = static_cast<LONG>(pinch::Phase::Swipe);
    record.swipeDirection = 1;
    move(); move(); end();
    require(calls == "SXJ" && lastDirection == 1 && record.pendingGestures == 0,
            "late lateral classification cancels provisional native state before switching");
    prepare(pinch::Phase::Pending, 0, false);
    begin();
    contacts = 4;
    move(); end();
    require(calls == "SCF" && record.pendingGestures == 0, "late four-finger pointer information bypasses provisional three-finger classification");
    prepare(pinch::Phase::Pinch, 4);
    begin(); move(); end();
    require(calls == "SCF", "an explicit four-finger gesture remains entirely native");
    prepare(pinch::Phase::Pending);
    begin(); native_gate::cancel(object);
    require(calls == "SX" && record.pendingGestures == 0, "cancel releases provisional shell state");
    prepare(pinch::Phase::Pending);
    startResult = E_FAIL;
    require(FAILED(native_gate::start(object, 0, 0, 1, {})) && record.pendingGestures == 0,
            "failed native start leaves no pending state");
    move(); end();
    require(calls == "S" && native_gate::states.empty(), "failed Start cannot be followed by native Continue or Finish");
    record.shellMask = 0;
    native_gate::shown(object, 4);
    require(record.shellMask == 0, "other shell selectors do not masquerade as Task View");
    native_gate::shown(object, 2);
    require(record.shellMask == 1, "real Task View visibility activates its own reason");
    native_gate::mapping.shellReason(2, true);
    native_gate::dismissed(object, 2);
    require(record.shellMask == 2, "closing Task View preserves a still-visible desktop reason");
    native_gate::mapping.shellReason(2, false);
    require(record.shellMask == 0, "all overview reasons can finish independently");
    prepare(pinch::Phase::Pinch);
    record.nativeCaps = 0; record.resolverReasonNative = ERROR_NOT_SUPPORTED;
    begin(); move(); end();
    require(calls == "SCF", "a definitively unsupported zoom backend cannot consume Windows gestures");
    record.resolverReasonNative = 0;
    prepare(pinch::Phase::Pinch);
    begin();
    record.resolverReasonNative = ERROR_NOT_SUPPORTED;
    move(); end();
    require(calls == "SCF", "a suppression latch is released if compatibility is rejected during the gesture");
    record.resolverReasonNative = 0;
    prepare(pinch::Phase::Pending);
    begin(); record.phase = static_cast<LONG>(pinch::Phase::Pinch); move();
    contacts = 4; move(); end();
    require(calls == "SX" && native_gate::states.empty() && record.pendingGestures == 0,
            "a cancelled pinch cannot resume or finish a native gesture when a fourth contact appears");
    prepare(pinch::Phase::Pending);
    begin(); record.phase = static_cast<LONG>(pinch::Phase::Pinch); move();
    record.resolverReasonNative = ERROR_NOT_SUPPORTED; move(); end();
    require(calls == "SX" && native_gate::states.empty(),
            "backend rejection cannot resurrect a Windows gesture already cancelled by the pinch");
    record.resolverReasonNative = 0;
    prepare(pinch::Phase::Pending, 0, false);
    begin(); record.phase = static_cast<LONG>(pinch::Phase::Swipe); record.down = 1; startResult = E_FAIL;
    move(); move(); end();
    require(calls == "S" && record.pendingGestures == 0 && native_gate::states.empty(),
            "failed deferred Start stays terminal until the native gesture ends");
    prepare(pinch::Phase::Swipe);
    begin(); continueResult = E_FAIL; move(); move(); end();
    require(calls == "SCX" && native_gate::states.empty(), "failed Continue cancels once and cannot continue or finish afterward");
    std::cout << checks << " native shell checks passed; fake callbacks; desktop unchanged\n";
}
