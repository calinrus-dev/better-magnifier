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
pinch::Config config;
}

int main() {
    test("spread produces continuous proportional zoom", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        auto result = r.update(fingers(1.1), 16, 1, config);
        require(result.began && result.zoom.has_value(), "pinch not activated");
        near(*result.zoom, std::pow(1.1, 1.5));
        result = r.update(fingers(1.25), 32, *result.zoom, config);
        require(!result.began, "pinch began twice");
        near(*result.zoom, std::pow(1.25, 1.5));
    });
    test("contracting zooms out from current magnification", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 2, config);
        near(*r.update(fingers(0.85), 16, 2, config).zoom, 2 * std::pow(0.85, 1.5));
    });
    test("zoom is bounded at 100 percent", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 1, config);
        near(*r.update(fingers(0.8), 16, 1, config).zoom, 1);
    });
    test("zoom respects configured maximum", [] {
        pinch::Recognizer r;
        r.update(fingers(), 0, 3.9, config);
        near(*r.update(fingers(1.2), 16, 3.9, config).zoom, 4);
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
        near(*r.update(fingers(0.9), 32, 2, config).zoom, 2 * std::pow(0.9, 1.5));
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
    std::cout << passed << " tests passed\n";
}
