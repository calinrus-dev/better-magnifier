#define PINCH_ZOOM_CORE_TEST
#include "../mods/kde-pinch-zoom.wh.cpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
int passed = 0;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void near(double a, double b, double epsilon = 0.00001) {
    require(std::abs(a - b) < epsilon, "unexpected numeric result");
}
template<class F> void test(const char* name, F run) {
    try {
        run();
        ++passed;
        std::cout << "PASS " << name << '\n';
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << name << ": " << e.what() << '\n';
        std::exit(1);
    }
}
std::vector<pinch::Contact> fingers(double scale = 1, double dx = 0, double dy = 0) {
    return {{11, {50 - 15 * scale + dx, 30 + dy}},
            {22, {50 + dx, 30 + dy}},
            {33, {50 + 15 * scale + dx, 30 + dy}}};
}
pinch::Config config = [] { pinch::Config value; value.acceleration = 0; value.maxZoom = 4; return value; }();
}

int main() {
    test("spread produces continuous proportional zoom", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        auto result = r.update(fingers(1.1), 16, 1, config);
        require(result.began && result.zoom.has_value(), "pinch not activated");
        near(*result.zoom, 1);  // Crossing the threshold must not jump the view.
        result = r.update(fingers(1.25), 32, *result.zoom, config);
        require(!result.began, "pinch began twice");
        near(*result.zoom, 1.25 / 1.1);
    });
    test("contracting zooms out from current magnification", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 2, config);
        near(*r.update(fingers(0.9), 16, 2, config).zoom, 2);
        near(*r.update(fingers(0.8), 32, 2, config).zoom, 2 * 0.8 / 0.9);
    });
    test("zoom is bounded at 100 percent", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        near(*r.update(fingers(0.8), 16, 1, config).zoom, 1);
    });
    test("zoom respects configured maximum", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 3.9, config);
        r.update(fingers(1.1), 16, 3.9, config);
        near(*r.update(fingers(1.2), 32, 3.9, config).zoom, 4);
    });
    test("horizontal swipe does not produce zoom", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        const auto result = r.update(fingers(1, 5), 16, 1, config);
        require(result.phase == pinch::Phase::Swipe && !result.zoom, "swipe became zoom");
    });
    test("all four swipe directions are rejected by zoom", [] {
        for (const auto point : {pinch::Point{6, 0}, {-6, 0}, {0, 6}, {0, -6}}) {
            pinch::Recognizer r;
            r.update(fingers(), 0, 1, config);
            require(r.update(fingers(1, point.x, point.y), 16, 1, config).phase == pinch::Phase::Swipe,
                    "direction not classified as swipe");
        }
    });
    test("swipe remains a swipe until fingers change", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        r.update(fingers(1, 5), 16, 1, config);
        require(!r.update(fingers(1.3, 5), 32, 1, config).zoom, "swipe relabelled as pinch");
    });
    test("small sensor noise is not a pinch", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        require(!r.update(fingers(1.02, 0.1, 0.1), 16, 1, config).zoom, "noise activated zoom");
    });
    test("slight centroid drift is tolerated", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        require(r.update(fingers(1.15, 0.2, -0.3), 16, 1, config).zoom.has_value(), "drifting pinch rejected");
    });
    test("two and four finger gestures never zoom", [] {
        pinch::Recognizer r;
        auto contacts = fingers();
        contacts.pop_back();
        require(!r.update(contacts, 0, 1, config).zoom, "two-finger zoom intercepted");
        contacts = fingers();
        contacts.push_back({44, {70, 45}});
        require(!r.update(contacts, 16, 1, config).zoom, "four-finger gesture intercepted");
    });
    test("full contact ids prevent truncated-id collisions", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        auto contacts = fingers(1.2);
        contacts[0].id += 64;
        require(!r.update(contacts, 16, 1, config).zoom, "changed contact id retained old baseline");
    });
    test("reordered slots preserve contact identity", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        auto contacts = fingers(1.2);
        std::reverse(contacts.begin(), contacts.end());
        require(r.update(contacts, 16, 1, config).zoom.has_value(), "slot order changed gesture");
    });
    test("duplicate contact ids are discarded", [] {
        pinch::Recognizer r;
        auto contacts = fingers();
        contacts[1].id = contacts[0].id;
        require(r.update(contacts, 0, 1, config).phase == pinch::Phase::Idle, "duplicate id accepted");
    });
    test("palms and invalid coordinates reset recognition", [] {
        for (int which = 0; which < 2; ++which) {
            pinch::Recognizer r;
            r.update(fingers(), 0, 1, config);
            auto contacts = fingers(1.2);
            if (which == 0) contacts[0].confident = false;
            else contacts[0].point.x = std::numeric_limits<double>::quiet_NaN();
            require(!r.update(contacts, 16, 1, config).zoom, "invalid contact activated zoom");
        }
    });
    test("lifting fingers ends the gesture", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        r.update(fingers(1.2), 16, 1, config);
        require(r.update({}, 32, 1, config).phase == pinch::Phase::Idle, "lift did not reset");
    });
    test("a stale gesture never resumes with its previous baseline", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        require(!r.update(fingers(1.2), 200, 1, config).zoom, "stale baseline reused");
    });
    test("a sensor jump does not cause a large magnification jump", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        require(r.update(fingers(3), 16, 1, config).phase == pinch::Phase::Rejected, "jump accepted");
    });
    test("degenerate finger geometry does not divide by zero", [] {
        pinch::Recognizer r;
        require(r.update(fingers(0), 0, 1, config).phase == pinch::Phase::Rejected, "zero radius accepted");
    });
    test("pinch can reverse direction without lifting", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 2, config);
        r.update(fingers(1.2), 16, 2, config);
        near(*r.update(fingers(0.9), 32, 2, config).zoom, 2 * 0.9 / 1.2);
    });
    test("parallel HID frame ignores padding slots", [] {
        pinch::FrameAssembler a;
        auto contacts = fingers();
        contacts.push_back({99, {0, 0}, false});
        const auto frame = a.feed({3, 10, contacts}, 0);
        require(frame && frame->size() == 3, "parallel frame padding not removed");
    });
    test("hybrid HID reports assemble exactly one complete frame", [] {
        pinch::FrameAssembler a;
        const auto contacts = fingers();
        require(!a.feed({3, 10, {contacts[0]}}, 0), "partial frame emitted");
        require(!a.feed({0, 10, {contacts[1]}}, 1), "partial frame emitted");
        const auto frame = a.feed({0, 10, {contacts[2]}}, 2);
        require(frame && frame->size() == 3, "hybrid frame not assembled");
    });
    test("different devices cannot share a hybrid frame", [] {
        pinch::FrameAssembler a, b;
        const auto contacts = fingers();
        a.feed({3, 10, {contacts[0]}}, 0);
        require(!b.feed({0, 10, {contacts[1], contacts[2]}}, 1), "devices combined");
    });
    test("scan-time changes discard incomplete frames", [] {
        pinch::FrameAssembler a;
        const auto contacts = fingers();
        a.feed({3, 10, {contacts[0]}}, 0);
        require(!a.feed({0, 11, {contacts[1], contacts[2]}}, 1), "scan times combined");
    });
    test("hybrid frame timeout discards missing contacts", [] {
        pinch::FrameAssembler a;
        const auto contacts = fingers();
        a.feed({3, 10, {contacts[0]}}, 0);
        require(!a.feed({0, 10, {contacts[1], contacts[2]}}, 60), "timed out contacts combined");
    });
    test("a new frame replaces an incomplete hybrid frame", [] {
        pinch::FrameAssembler a;
        const auto contacts = fingers();
        a.feed({3, 10, {contacts[0]}}, 0);
        const auto frame = a.feed({3, 11, contacts}, 1);
        require(frame && frame->size() == 3, "new frame not assembled");
    });
    test("invalid contact counts are bounded", [] {
        pinch::FrameAssembler a;
        require(!a.feed({100000, 10, fingers()}, 0), "unbounded contact count accepted");
    });
    test("an empty HID report releases contacts", [] {
        pinch::FrameAssembler a;
        const auto frame = a.feed({0, 10, {{0, {}, false}}}, 0);
        require(frame && frame->empty(), "release report ignored");
    });
    test("smoothing is monotonic and independent of frame subdivision", [] {
        const double once = pinch::smooth(1, 2, 32, 40);
        const double twice = pinch::smooth(pinch::smooth(1, 2, 16, 40), 2, 16, 40);
        require(once > 1 && once < 2, "smoothing overshot");
        near(once, twice);
        near(pinch::smooth(1, 2, 16, 0), 2);
    });
    test("100 percent always has zero fullscreen offsets", [] {
        const auto view = pinch::viewAt(1, {500, 500}, {-1920, 0, 3840, 1080});
        require(view.x == 0 && view.y == 0 && view.zoom == 1, "100% retained an offset");
    });
    test("zoom remains anchored at pointer on the primary display", [] {
        const auto view = pinch::viewAt(2, {960, 540}, {0, 0, 1920, 1080});
        require(view.x == 480 && view.y == 270, "wrong fullscreen offset");
    });
    test("negative monitor origins produce negative magnifier offsets", [] {
        const auto view = pinch::viewAt(2, {-960, -540}, {-1920, -1080, 3840, 2160});
        require(view.x == -480 && view.y == -270, "negative virtual origin was lost");
    });
    test("a fixed content point stays under the same display point throughout a pinch", [] {
        const pinch::Desktop desktop{0, 0, 1920, 1080};
        const auto anchor = pinch::anchorAt({1, 0, 0}, {720, 420});
        for (const double scale : {1.1, 1.5, 2.0, 3.0, 1.2}) {
            const auto view = pinch::anchoredView(scale, anchor, desktop);
            near((anchor.content.x - view.x) * scale, anchor.display.x, 1.6);
            near((anchor.content.y - view.y) * scale, anchor.display.y, 1.6);
        }
    });
    test("starting another pinch from an existing zoom does not recenter the viewport", [] {
        const pinch::View original{2, 400, 200};
        const auto anchor = pinch::anchorAt(original, {700, 400});
        const auto view = pinch::anchoredView(2, anchor, {0, 0, 1920, 1080});
        require(view.x == original.x && view.y == original.y, "viewport jumped when a pinch started");
    });
    test("native swipe gate holds an undecided gesture until classification", [] {
        pinch::GestureGate gate;
        require(gate.update(pinch::Phase::Pending) == pinch::GateMode::Pending, "native swipe started too early");
        require(gate.update(pinch::Phase::Pinch) == pinch::GateMode::Suppress, "pinch did not suppress swipe");
    });
    test("a pinch cannot later activate native multitasking", [] {
        pinch::GestureGate gate(pinch::Phase::Pinch);
        require(gate.update(pinch::Phase::Swipe) == pinch::GateMode::Suppress, "pinch changed into native swipe");
        require(gate.finish(pinch::Phase::Idle) == pinch::GateMode::Suppress, "finger lift activated native swipe");
    });
    test("an authoritative late pinch cancels an early native swipe classification", [] {
        pinch::GestureGate gate;
        require(gate.update(pinch::Phase::Swipe) == pinch::GateMode::Forward, "native swipe blocked");
        require(gate.update(pinch::Phase::Pinch) == pinch::GateMode::Suppress, "late pinch leaked into native multitasking");
    });
    test("a short undecided native swipe is forwarded on release", [] {
        pinch::GestureGate gate;
        require(gate.finish(pinch::Phase::Idle) == pinch::GateMode::Forward, "short native swipe lost");
    });
    test("reversing a pinch at 100 percent responds on the next frame", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        r.update(fingers(0.9), 16, 1, config);
        near(*r.update(fingers(0.7), 32, 1, config).zoom, 1);
        require(*r.update(fingers(0.72), 48, 1, config).zoom > 1.02, "lower-bound dead zone");
    });
    test("reversing at the maximum does not require retracing excess finger travel", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 3.9, config);
        r.update(fingers(1.1), 16, 3.9, config);
        near(*r.update(fingers(1.5), 32, 3.9, config).zoom, 4);
        require(*r.update(fingers(1.45), 48, 4, config).zoom < 3.9, "upper-bound dead zone");
    });
    test("a keyboard change rebases an ongoing pinch without losing finger tracking", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        r.update(fingers(1.1), 16, 1, config);
        r.adoptZoom(3);
        near(*r.update(fingers(1.21), 32, 3, config).zoom, 3.3);
    });
    test("fast pinches gain more zoom for the same travel, while slow pinches stay precise", [] {
        auto accelerated = config;
        accelerated.acceleration = 0.35;
        const auto run = [&](int interval) {
            pinch::Recognizer r;
            r.update(fingers(), 0, 1, accelerated);
            r.update(fingers(1.05), interval, 1, accelerated);
            double level = 1;
            for (int i = 1; i <= 8; ++i) {
                const double scale = 1.05 * std::pow(1.02, i);
                level = *r.update(fingers(scale), (i + 1) * interval, level, accelerated, interval).zoom;
            }
            return level;
        };
        const double slow = run(100), fast = run(8);
        near(slow, std::pow(1.02, 8));
        require(fast > slow && fast < std::pow(slow, 1.351), "unbounded or missing acceleration");
    });
    test("acceleration can be disabled", [] {
        auto linear = config;
        linear.acceleration = 0;
        pinch::Recognizer a, b;
        a.update(fingers(), 0, 1, linear); b.update(fingers(), 0, 1, linear);
        a.update(fingers(1.1), 10, 1, linear); b.update(fingers(1.1), 100, 1, linear);
        near(*a.update(fingers(1.3), 20, 1, linear, 10).zoom,
             *b.update(fingers(1.3), 200, 1, linear, 100).zoom);
    });
    test("no timestamp means no spurious acceleration from queued reports", [] {
        auto accelerated = config;
        accelerated.acceleration = 1.5;
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, accelerated);
        r.update(fingers(1.1), 10, 1, accelerated);
        near(*r.update(fingers(1.2), 10, 1, accelerated, 0).zoom, 1.2 / 1.1);
    });
    test("HID scan timestamps preserve velocity when Windows batches input", [] {
        pinch::FrameTiming clock;
        clock.advance(100, 1000);
        near(clock.advance(180, 1000.1), 8);
        near(clock.advance(260, 1000.2), 8);
    });
    test("16-bit scan time wrap is handled", [] {
        pinch::FrameTiming clock;
        clock.advance(65520, 1000);
        near(clock.advance(64, 1001), 8);
    });
    test("reversed pinch is optional and does not change native swipe classification", [] {
        auto reversed = config;
        reversed.invertPinch = true;
        pinch::Recognizer r;
        r.update(fingers(), 0, 2, reversed);
        r.update(fingers(1.1), 16, 2, reversed);
        require(*r.update(fingers(1.21), 32, 2, reversed).zoom < 2, "reverse direction ignored");
    });
    test("there is no artificial 400 percent cap unless configured", [] {
        near(pinch::maximumZoom(0), 4096);
        near(pinch::maximumZoom(400), 4);
        near(pinch::maximumZoom(1200), 12);
        auto unlimited = config;
        unlimited.maxZoom = pinch::maximumZoom(0);
        pinch::Recognizer r;
        r.update(fingers(), 0, 5, unlimited);
        r.update(fingers(1.1), 16, 5, unlimited);
        require(*r.update(fingers(1.2), 32, 5, unlimited).zoom > 5, "hidden legacy cap");
    });
    test("waiting for native startup discards unrendered pinch travel", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        r.update(fingers(1.05), 16, 1, config);
        for (int i = 1; i <= 8; ++i) {
            r.update(fingers(1.05 + i * 0.04), 16 + i * 16, 1, config);
            r.adoptZoom(1); // Host discards movement while the native app starts.
        }
        const double previous = 1.05 + 8 * 0.04;
        near(*r.update(fingers(previous * 1.01), 160, 1, config).zoom, 1.01);
    });
    test("direct window swipes keep a stable cycle despite activation changing z-order", [] {
        pinch::WindowCycle cycle;
        require(cycle.next({1, 2, 3}, 1, 1) == 2, "first next window");
        require(cycle.next({2, 1, 3}, 2, 1) == 3, "cycle collapsed to last two windows");
        require(cycle.next({3, 2, 1}, 3, -1) == 2, "reverse window direction");
    });
    test("external window activation resets the direct swipe order", [] {
        pinch::WindowCycle cycle;
        cycle.next({1, 2, 3}, 1, 1);
        require(cycle.next({3, 1, 2}, 3, 1) == 1, "external activation ignored");
    });
    test("closed windows are removed from the direct swipe cycle", [] {
        pinch::WindowCycle cycle;
        cycle.next({1, 2, 3}, 1, 1);
        require(cycle.next({2, 1}, 2, 1) == 1, "closed window remained selectable");
        require(cycle.next({1}, 1, 1) == 0, "single window should not be switched");
    });
    test("swipe direction separates vertical multitasking from direct lateral switching", [] {
        require(pinch::swipeDirection({-4, 0.5}) == -1, "left direction");
        require(pinch::swipeDirection({4, 0.5}) == 1, "right direction");
        require(pinch::swipeDirection({0.5, -4}) == -2, "up was interpreted as lateral");
        require(pinch::swipeDirection({0.5, 4}) == 2, "down was interpreted as lateral");
    });
    test("wheel retains high resolution deltas proportionally", [] {
        pinch::WheelMotion a, b;
        a.add(120, 2, 0, 15, 100, 16);
        for (int i = 0; i < 4; ++i) b.add(30, 2, 0, 15, 100, 16);
        near(a.goal(), b.goal()); near(a.goal(), 2.3);
    });
    test("wheel publishes decimal intermediate frames and finishes", [] {
        pinch::WheelMotion w; w.add(120, 1, 10, 15, 100, 16);
        const double middle = *w.tick(60);
        require(middle > 1 && middle < 1.15, "wheel still jumps to its final level");
        near(*w.tick(110), 1.15);
        require(!w.active() && !w.tick(126), "wheel timer would continue indefinitely");
    });
    test("successive wheel notches accumulate from their goal", [] {
        pinch::WheelMotion w; w.add(120, 2, 0, 15, 100, 16);
        w.tick(16); w.add(240, 2.1, 16, 15, 100, 16);
        near(w.goal(), 2 * std::pow(1.15, 3));
    });
    test("wheel reversal drops the old direction immediately", [] {
        pinch::WheelMotion w; w.add(360, 3, 0, 15, 100, 16);
        const double before = *w.tick(25);
        w.add(-120, before, 25, 15, 100, 16);
        require(*w.tick(40) < before && w.goal() < before, "old momentum continued after reversing wheel");
    });
    test("wheel limit does not accumulate invisible momentum", [] {
        pinch::WheelMotion w; w.add(32767, 15, 0, 100, 40, 16);
        near(*w.tick(40), 16);
        w.add(-30, 16, 40, 15, 100, 16);
        require(w.goal() < 16 && *w.tick(56) < 16, "cannot leave upper limit immediately");
        w.add(-32768, 1, 200, 100, 100, 16); near(*w.tick(300), 1);
        w.add(30, 1, 300, 15, 100, 16); require(w.goal() > 1, "cannot leave lower limit");
    });
    test("wheel cancellation cannot replay the old goal", [] {
        pinch::WheelMotion w; w.add(120, 2, 0, 15, 100, 16); w.cancel();
        require(!w.tick(20), "cancelled trajectory still publishes frames");
        w.add(-120, 5, 30, 15, 100, 16); near(w.goal(), 5 / 1.15);
    });
    test("wheel negative and fractional movement share the same scale", [] {
        pinch::WheelMotion w; w.add(-60, 2, 0, 15, 100, 16);
        near(w.goal(), 2 / std::sqrt(1.15));
        w.add(0, 2, 0, 15, 100, 16); require(std::isfinite(w.goal()), "invalid wheel goal");
    });
    test("interactive session stays inactive across lock suspend resume", [] {
        pinch::InteractiveSession session; require(!session.active(), "unknown session must not accept zoom input");
        session.snapshot(true, true); require(session.active(), "unlocked session stays inactive");
        session.lock(); session.suspend(); session.resume();
        require(!session.active(), "resume accidentally unlocked a locked session");
        session.snapshot(true, false); require(!session.active(), "locked session snapshot accepted input");
        session.unlock(); require(session.active(), "unlock cannot recover input");
    });
    test("a connection is not an unlock", [] {
        pinch::InteractiveSession session; session.snapshot(true, true); session.lock();
        session.disconnect(); session.connect(); require(!session.active(), "reconnect bypasses lock");
        session.unlock(); require(session.active(), "unlocked reconnect stays inactive");
        session.disconnect(); require(!session.active(), "disconnected session keeps zoom active");
    });
    test("native launch has bounded retries with a cooldown", [] {
        pinch::NativeLaunchPolicy launch;
        require(launch.mayLaunch(0), "fresh login does not prewarm Magnifier");
        launch.attempted(0); require(!launch.mayLaunch(1), "zero uptime bypasses launch cooldown");
        require(launch.mayLaunch(1500), "cooldown never recovers"); launch.attempted(1500);
        launch.attempted(3000); require(!launch.mayLaunch(9000), "failed launch retries forever");
        launch.request(); require(launch.mayLaunch(9000), "fresh physical input cannot retry after temporary startup failure");
    });
    test("Win Esc prevents automatic reopen until fresh physical input", [] {
        pinch::NativeLaunchPolicy launch; launch.observe(true, true);
        require(launch.observe(false, false), "native exit not detected");
        require(!launch.mayLaunch(9000), "closed Magnifier reopens from the timer");
        launch.resetAttempts(); require(!launch.mayLaunch(10000), "resume or settings reopened closed Magnifier");
        launch.request(); require(launch.mayLaunch(10000), "fresh pinch cannot reopen Magnifier");
    });
    test("renderer rebuild is not a native process exit", [] {
        pinch::NativeLaunchPolicy launch; launch.observe(true, true);
        require(!launch.observe(false, true), "rebuilding renderer treated as an exit");
        launch.observe(true, true);
        require(launch.observe(false, false), "exit lost after renderer recovery");
    });
    test("clean close before native READY does not reopen from startup retries", [] {
        pinch::NativeLaunchPolicy launch; launch.attempted(100);
        launch.observe(false, true);
        require(launch.observe(false, false, true), "early clean close not detected");
        require(!launch.mayLaunch(9000), "Win Esc before READY still causes startup retries");
        launch.request(); require(launch.mayLaunch(9000), "fresh physical input cannot recover after early close");
    });
    std::cout << passed << " tests passed\n";
}
