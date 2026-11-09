// Pure shortcut state tests: no Windows hooks, input injection or desktop changes.
#define PINCH_ZOOM_CORE_TEST
#include "../mods/kde-pinch-zoom.wh.cpp"
#include <iostream>
#include <cstdlib>

int main() {
    unsigned checks = 0;
    const auto require = [&](bool valid, const char* label) {
        if (!valid) { std::cerr << "FAIL " << label << '\n'; std::exit(1); }
        ++checks;
    };
    {
        pinch::WinMenuMask win;
        require(!win.winDown() && !win.consume(), "mouse use without a physical Win press never asks for a mask");
        win.key(0x5b, true);
        require(win.winOnly() && win.consume(), "first consumed LWin mouse action requests one neutral mask");
        require(!win.consume(), "a free-spin wheel does not inject a mask for each delta");
        win.key(0xe8, true, true); win.key(0xe8, false, true);
        require(win.winOnly() && !win.consume(), "own injected menu masks do not change physical keyboard state");
        win.key(0x5b, false);
        require(!win.winDown() && !win.consume(), "releasing Win ends the consumed mouse session");
        win.key(0x5c, true);
        require(win.winOnly() && win.consume(), "RWin supports the same mouse profile as LWin");
        win.maskFailed();
        require(win.consume() && !win.consume(), "a failed mask can retry without losing the physical Win press");
        win.reset();
        win.key(0x5b, true, true);
        require(!win.winDown(), "an injected Win press is not mistaken for physical user intent");
    }
    for (unsigned shortcut : {0x53u, 0x20u, 0x6bu, 0xbbu, 0x6du, 0xbdu}) {
        pinch::WinMenuMask win;
        win.key(0x5b, true); win.key(shortcut, true);
        require(!win.winOnly(), "a currently held keyboard shortcut prevents a mouse-only Win match");
        win.key(shortcut, false);
        require(win.winOnly() && !win.consume(), "Win+S/Space/plus/minus already use the modifier and need no new mask");
        win.key(0x5b, false); win.key(0x5b, true);
        require(win.consume(), "a later independent Win mouse session can request its own mask");
    }
    {
        pinch::WinMenuMask win;
        win.key(0x5b, true); win.key(0x5c, true);
        require(win.consume() && !win.consume(), "overlapping left and right Win presses share one mask session");
        win.key(0x5b, false);
        require(win.winOnly() && !win.consume(), "releasing only one Win key preserves the remaining session");
        win.key(0x5c, false); win.key(0x5c, true);
        require(win.consume(), "both Win keys must be released before an independent session starts");
        win.key(0x5c, true);
        require(!win.consume(), "modifier repeats do not turn each mouse delta into a new mask request");
        win.reset();
        require(!win.winDown() && !win.winOnly(), "session loss clears modifier bookkeeping without synthesizing key releases");
    }
    {
        pinch::WindowWheel wheel;
        require(!wheel.feed(30, 0) && !wheel.feed(30, 1) && !wheel.feed(30, 2), "fractional wheel messages do not switch one window each");
        require(wheel.feed(30, 3) == -1, "one positive wheel notch selects the previous window");
        require(!wheel.feed(-30, 4) && !wheel.feed(-30, 5) && !wheel.feed(-30, 6), "reversal clears old direction and retains new fractional movement");
        require(wheel.feed(-30, 128) == 1, "one negative notch selects the next window after the switching cooldown");
        wheel.reset();
        require(wheel.feed(120, 1000) == -1, "ordinary discrete wheels switch immediately on their first notch");
        unsigned actions = 0;
        for (unsigned now = 1001; now < 1125; ++now) if (wheel.feed(32767, now)) ++actions;
        require(!actions, "a rapid free-spin burst cannot exceed the window-switch cooldown");
        require(wheel.feed(1, 1125) == -1, "at most one pending window step becomes available on fresh input");
        require(!wheel.feed(0, 3000), "idle time never flushes buffered window switches by itself");
        require(!wheel.feed(1, 3001), "a stale burst is not replayed when later fine input arrives");
        wheel.reset(); wheel.feed(119, 0);
        require(!wheel.feed(-1, 1), "tiny reversal cannot complete an old nearly-full notch");
        require(wheel.feed(-119, 2) == 1, "new direction reaches its own complete notch");
        require(!wheel.feed(119, 1000) && !wheel.feed(1, 1201), "fractional residual expires after an input gap");
        wheel.reset();
        require(wheel.feed(std::numeric_limits<int>::min(), 5) == 1, "extreme signed wheel deltas remain bounded without integer overflow");
        require(wheel.feed(120, 4) == -1, "a clock rollback resets the cooldown rather than blocking future window changes");
    }
    {
        pinch::MouseButtonOwnership buttons;
        require(buttons.press(1, true), "middle-button action takes ownership on its physical down edge");
        require(buttons.pressed(1) && buttons.owned(1), "the broker can identify an already captured down before posting another action");
        require(buttons.press(1, false), "modifier release or repeated down cannot drop ownership halfway through a press");
        require(buttons.release(1) && !buttons.release(1), "exactly the paired middle-button up is consumed");
        require(!buttons.pressed(1) && !buttons.owned(1), "button-up clears both the physical press and its shortcut ownership");
        require(!buttons.press(2, false) && buttons.pressed(2) && !buttons.owned(2), "passthrough down edges are tracked without taking ownership");
        require(!buttons.press(2, true) && !buttons.release(2), "a normal right-button down cannot become a stolen shortcut on release");
        require(buttons.press(1, true) && buttons.press(2, true), "middle and right actions can own independent button pairs");
        require(buttons.release(2) && buttons.release(1), "independent button-up order preserves both pairs");
        require(!buttons.release(0) && !buttons.press(7, true) && !buttons.pressed(7) && !buttons.owned(7), "unmatched or unsupported button events pass through");
        buttons.press(1, true); buttons.reset();
        require(!buttons.release(1), "unloading clears private ownership without retaining a stale swallowed button-up");
    }
    require(compatibility::mouseZoomAllowed(0, 0, 0), "a cold backend may accept one bounded wake intent before capabilities are published");
    require(compatibility::mouseZoomAllowed(compatibility::NativeCore, 0, 0), "a verified native core allows the mouse zoom profile");
    require(!compatibility::mouseZoomAllowed(0, 50, 0), "known missing core symbols leave mouse input to Windows");
    require(!compatibility::mouseZoomAllowed(compatibility::NativeCore, 0, 50), "an unsupported native mode cannot consume zoom mouse actions");
    require(compatibility::mouseZoomAllowed(compatibility::NativeCore, 0, 1237), "a transient native retry does not permanently disable a compatible backend");
    std::cout << checks << " pure mouse shortcut checks passed; no hooks or input injection\n";
}
