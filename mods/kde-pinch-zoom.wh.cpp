// ==WindhawkMod==
// @id              kde-pinch-zoom
// @name            Better Magnifier
// @name:es-ES      Better Magnifier
// @description     KWin-inspired accessibility: smooth pinch and mouse-wheel zoom for native Windows Magnifier
// @description:es-ES Accesibilidad inspirada en KWin: zoom fluido con pellizco y rueda para la Lupa nativa de Windows
// @version         0.7.0
// @author          calinrus-dev
// @github          https://github.com/calinrus-dev
// @homepage        https://github.com/calinrus-dev/better-magnifier
// @include         explorer.exe
// @include         magnify.exe
// @architecture    amd64
// @compilerOptions -DNOMINMAX -static -lhid -lwtsapi32 -lshell32 -ladvapi32 -ldwmapi -lole32
// @license         GPL-3.0-or-later
// ==/WindhawkMod==

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 calinrus-dev
// Raw HID handling was informed by Three Finger Drag by
// Rodrigo Thomazi (GPL-3.0). See THIRD_PARTY.md.

// ==WindhawkModReadme==
/*
# Better Magnifier — experimental

Spread three fingers to magnify the desktop; pinch them together to zoom out.
Win+mouse wheel provides proportional smooth zoom; Win+middle click resets.
Ctrl+Alt+wheel switches windows. The mouse profile is configurable.
Two-finger app zoom and Windows three/four-finger swipe settings are untouched.
A Precision Touchpad and Windows build 26200 are required. A companion in
Explorer holds native app-switch/task-view swipes until the gesture is classified.
Pinches suppress those swipes; vertical swipes use the original Windows handler.
Horizontal swipes switch windows directly by default (configurable).
Zoom remains off if that native companion is not loaded. Experimental: validate
the shell hooks on your machine before relying on this mod.

Ctrl+Alt+Shift+F10 resets the zoom. Ctrl+Alt+Shift+F9 pauses/resumes the mod.
Disabling/unloading restores the previous view if the mod still owns it.
Windows Magnifier provides rendering, keyboard shortcuts and pointer tracking.
Its window and touch controls are hidden by default (configurable).

Use Observe only first to check gesture recognition without changing the view.
The standalone diagnostic included in the repository lists compatible HID
devices and counts gesture events without storing contact coordinates.

Installation, checks and current limitations:
https://github.com/calinrus-dev/better-magnifier
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- enabled: true
  $name: Enabled
- observeOnly: false
  $name: Observe only (recognition without zoom)
- sensitivity: 100
  $name: Zoom sensitivity (%)
  $description: 100 follows finger spread directly; higher values zoom faster.
- acceleration: 35
  $name: Pinch acceleration (%)
  $description: Extra gain for fast pinches; slow movement stays precise. 0 disables acceleration.
- invertPinch: false
  $name: Reverse pinch direction
- threshold: 3
  $name: Pinch activation threshold (%)
  $description: Minimum change in finger spread before zoom starts.
- maxZoom: 0
  $name: Maximum zoom (%)
  $description: 0 uses the native Windows Magnifier limit (1600%) without a separate mod cap.
- keyboardInterop: true
  $name: Native Windows Magnifier integration
  $description: Enables pinch and smooth wheel zoom. Turn off to retain standard Windows behavior.
- smoothWheel: true
  $name: Smooth mouse wheel zoom
  $description: Uses continuous native zoom instead of jumps. Ordinary scrolling is unchanged.
- winWheel: true
  $name: Use Win + mouse wheel for zoom
  $description: Win + wheel zooms; Win + middle click resets to 100%. Disable to use Ctrl+Alt instead. Uses a neutral menu-mask key to prevent Start opening on release; elevated apps may reject it.
- wheelWindowSwitch: true
  $name: Ctrl+Alt + mouse wheel switches windows
  $description: Available with the Win-wheel profile. One window per notch, with a bounded rate for free-spinning wheels.
- windowButtons: false
  $name: Extra Ctrl+Alt mouse window actions
  $description: In the Win-wheel profile, middle click maximizes/restores and right click minimizes the current app window. Disabled by default.
- disableReading: true
  $name: Disable Magnifier click-to-read and its reading cursor
  $description: Blocks Magnifier Ctrl+Alt + left-click reading and its special cursor on verified builds. Does not disable Narrator or intercept ordinary left clicks.
- wheelStep: 20
  $name: Wheel zoom per notch (%)
  $description: Proportional change per 120 wheel units. High-resolution wheel deltas retain their precision.
- wheelDuration: 100
  $name: Wheel transition time (milliseconds)
  $description: 40 to 250 ms. More wheel movement increases the zoom distance.
- wheelMaxRate: 400
  $name: Maximum wheel zoom growth per second (%)
  $description: 400 permits up to five times the zoom per second. Bounds free-spin speed without losing small wheel deltas. 50 to 2000.
- keyboardHoldMultiplier: 2
  $name: Progressive keyboard hold speed
  $description: 1 keeps standard repeats. 2 to 4 controls continuous acceleration after holding Win plus/minus for 200 ms, including numpad. Release stops at the visible level; a tap stays native.
- keyboardStep: 0
  $name: Keyboard zoom increment (%)
  $description: 0 keeps the increment configured in Windows Magnifier.
- instantHorizontalSwitch: true
  $name: Switch windows directly with a horizontal three-finger swipe
  $description: Changes one window per swipe without opening the native app selector. Left selects next; right selects previous.
- normalZoomInOverview: true
  $name: Show Task View at 100 percent
  $description: Task View temporarily suspends zoom, then restores the level and position. Fresh zoom input overrides the suspension. The desktop remains zoomable; no per-window profiles.
- keepPointerStable: true
  $name: Keep the pointer stationary during a pinch
  $description: Keeps the pointer stable while pinching, including slow movements. Real mouse movement and native keyboard zoom remain available.
- suppressStartupFeedback: true
  $name: Suppress Magnifier startup feedback cursor
  $description: Keeps Windows from showing its blue startup indicator when this mod opens Magnifier. Other apps are unaffected.
- hideMagnifier: true
  $name: Hide Windows Magnifier window and touch controls
- smoothing: 0
  $name: Smoothing time for standalone API mode (milliseconds)
  $description: Native mode uses Windows Magnifier animations.
- fallbackHeight: 60
  $name: Fallback touchpad height (% of width)
  $description: Used only when the HID descriptor omits physical dimensions.
- debugLogging: false
  $name: Log gesture decisions
*/
// ==/WindhawkModSettings==

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace pinch {

constexpr std::size_t kMaxContacts = 32;
struct Point {
    double x = 0;
    double y = 0;
};
struct Contact {
    std::uint32_t id;
    Point point;
    bool down = true;
    bool confident = true;
};
struct Report {
    std::uint32_t count = 0;
    std::optional<std::uint32_t> scan;
    std::vector<Contact> contacts;
};

// Each HID device owns an assembler. Count > 0 starts a hybrid frame; zero
// continues it. Never publish incomplete frames or join different scan times.
class FrameAssembler {
    std::uint32_t expected_ = 0;
    std::optional<std::uint32_t> scan_;
    std::vector<Contact> contacts_;
    std::uint64_t started_ = 0;

public:
    void reset() {
        expected_ = 0;
        scan_.reset();
        contacts_.clear();
    }
    std::optional<std::vector<Contact>> feed(const Report& report,
                                             std::uint64_t now) {
        if (report.count > kMaxContacts) {
            reset();
            return std::nullopt;
        }
        if (expected_ && (now - started_ > 50 ||
                          (scan_ && report.scan && scan_ != report.scan))) {
            reset();
        }
        if (report.count) {
            reset();
            expected_ = report.count;
            scan_ = report.scan;
            started_ = now;
        } else if (!expected_) {
            // Ignore orphan hybrid continuations; a fully released report is
            // still useful because it ends a gesture immediately.
            if (std::none_of(report.contacts.begin(), report.contacts.end(),
                             [](const Contact& c) { return c.down; })) {
                return std::vector<Contact>{};
            }
            return std::nullopt;
        }
        for (const auto& contact : report.contacts) {
            if (contacts_.size() == expected_) {
                break;  // Remaining descriptor slots are padding.
            }
            contacts_.push_back(contact);
        }
        if (contacts_.size() < expected_) {
            return std::nullopt;
        }
        auto result = std::move(contacts_);
        reset();
        return result;
    }
};

// Lifecycle policy is pure: no Windows calls and no forced restart after Win+Esc.
class InteractiveSession {
    bool connected_ = false;
    bool unlocked_ = false;
    bool awake_ = true;
public:
    bool active() const { return connected_ && unlocked_ && awake_; }
    void snapshot(bool connected, bool unlocked) { connected_ = connected; unlocked_ = unlocked; }
    void lock() { unlocked_ = false; }
    void unlock() { connected_ = true; unlocked_ = true; }
    void disconnect() { connected_ = false; }
    void connect() { connected_ = true; } // A connection is not an unlock.
    void suspend() { awake_ = false; }
    void resume() { awake_ = true; unlocked_ = false; } // Refresh the real lock state.
};
class NativeLaunchPolicy {
    bool requested_ = true;
    bool sawReady_ = false;
    unsigned attempts_ = 0;
    std::uint64_t lastAttempt_ = 0;
    bool attempted_ = false;
public:
    // Return true only on an exit after a working native instance. Closing a
    // ready Magnifier intentionally suppresses timer-driven reopening.
    bool observe(bool ready, bool alive, bool cleanExit = false) {
        if (ready) { sawReady_ = true; attempts_ = 0; return false; }
        if (alive) return false; // A rebuilding renderer is not a process exit.
        if (!sawReady_ && !cleanExit) return false;
        sawReady_ = false;
        requested_ = false;
        return true;
    }
    void request() { requested_ = true; attempts_ = 0; lastAttempt_ = 0; }
    void resetAttempts() { attempts_ = 0; } // Preserve an intentional close.
    bool mayLaunch(std::uint64_t now) const {
        return requested_ && attempts_ < 3 && (!attempted_ || now - lastAttempt_ >= 1500);
    }
    void attempted(std::uint64_t now) { lastAttempt_ = now; attempted_ = true; ++attempts_; }
};

struct Config {
    double sensitivity = 1.0;
    double acceleration = 0.35;
    double threshold = 0.03;
    double maxZoom = 4096.0;
    double smoothingMs = 0;
    double fallbackAspect = 0.60;
    bool enabled = true;
    bool observeOnly = false;
    bool debug = false;
    bool invertPinch = false;
    bool keyboardInterop = true;
    bool hideMagnifier = true;
    bool suppressStartupFeedback = true;
    bool smoothWheel = true;
    bool winWheel = true;
    bool wheelWindowSwitch = true;
    bool windowButtons = false;
    bool disableReading = true;
    int wheelStep = 20;
    int wheelDuration = 100;
    int wheelMaxRate = 400;
    bool instantHorizontalSwitch = true;
    bool normalZoomInOverview = true;
    int keyboardStep = 0;
    int keyboardHoldMultiplier = 2;
    int mouseFollow = 0;  // edges, proportional, fixed
    double edgeMargin = 0.12;
};
inline double maximumZoom(int percent) {
    return percent <= 0 ? 4096.0 : std::clamp(percent / 100.0, 1.0, 4096.0);
}

class FrameTiming {
    std::optional<std::uint32_t> scan_;
    double wall_ = 0;
public:
    double advance(std::optional<std::uint32_t> scan, double wall) {
        double elapsed = wall_ > 0 ? wall - wall_ : 0;
        if (scan && scan_) {
            const auto delta = (*scan - *scan_) & 0xFFFFu;
            if (delta > 0 && delta <= 1500) elapsed = delta * 0.1;
        }
        scan_ = scan;
        wall_ = wall;
        return elapsed > 0 && elapsed <= 150 ? elapsed : 0;
    }
};
enum class Phase { Idle, Pending, Swipe, Pinch, Rejected };
struct Decision {
    Phase phase = Phase::Idle;
    std::optional<double> zoom;
    bool began = false;
};

class Recognizer {
    Phase phase_ = Phase::Idle;
    std::array<std::uint32_t, 3> ids_{};
    Point start_{};
    double radius_ = 0;
    double zoom_ = 1;
    double previousScale_ = 1;
    double velocity_ = 0;
    bool hasVelocity_ = false;
    std::uint64_t lastTime_ = 0;
    Point movement_{};

public:
    void reset() { phase_ = Phase::Idle; }
    Phase phase() const { return phase_; }
    Point movement() const { return movement_; }
    void adoptZoom(double zoom) {
        if (std::isfinite(zoom) && zoom >= 1) zoom_ = zoom;
    }
    Decision update(const std::vector<Contact>& frame, std::uint64_t now,
                    double currentZoom, const Config& config, double elapsedMs = -1) {
        std::vector<Contact> active;
        for (const auto& contact : frame) {
            if (contact.down) {
                if (!contact.confident || !std::isfinite(contact.point.x) ||
                    !std::isfinite(contact.point.y)) {
                    reset();
                    return {};
                }
                active.push_back(contact);
            }
        }
        if (active.size() != 3) {
            reset();
            return {};
        }
        std::sort(active.begin(), active.end(),
                  [](const Contact& a, const Contact& b) { return a.id < b.id; });
        std::array<std::uint32_t, 3> ids{active[0].id, active[1].id, active[2].id};
        if (ids[0] == ids[1] || ids[1] == ids[2]) {
            reset();
            return {};
        }
        Point center{};
        for (const auto& c : active) {
            center.x += c.point.x / 3;
            center.y += c.point.y / 3;
        }
        double radius = 0;
        for (const auto& c : active) {
            radius += std::pow(c.point.x - center.x, 2) +
                      std::pow(c.point.y - center.y, 2);
        }
        radius = std::sqrt(radius / 3);
        if (phase_ == Phase::Idle || ids != ids_ || now - lastTime_ > 150) {
            ids_ = ids;
            start_ = center;
            movement_ = {};
            radius_ = radius;
            zoom_ = std::isfinite(currentZoom) ? std::max(1.0, currentZoom) : 1.0;
            previousScale_ = 1;
            velocity_ = 0;
            hasVelocity_ = false;
            phase_ = radius >= 3 ? Phase::Pending : Phase::Rejected;
            lastTime_ = now;
            return {phase_, std::nullopt, false};
        }
        if (elapsedMs < 0) elapsedMs = static_cast<double>(now - lastTime_);
        lastTime_ = now;
        if (phase_ == Phase::Swipe || phase_ == Phase::Rejected) {
            return {phase_, std::nullopt, false};
        }
        const double scale = radius / radius_;
        if (scale <= 0 || !std::isfinite(scale) ||
            std::abs(std::log(scale / previousScale_)) > 0.5) {
            phase_ = Phase::Rejected;
            return {phase_, std::nullopt, false};
        }
        const double scaleDelta = std::log(scale / previousScale_);
        previousScale_ = scale;
        movement_ = {center.x - start_.x, center.y - start_.y};
        const double travel = std::hypot(movement_.x, movement_.y);
        const double spread = std::abs(radius - radius_);
        if (phase_ == Phase::Pending && travel > std::max(2.5, spread * 1.5)) {
            phase_ = Phase::Swipe;
            return {phase_, std::nullopt, false};
        }
        bool began = false;
        if (phase_ == Phase::Pending &&
            std::abs(std::log(scale)) >= std::log1p(config.threshold) &&
            spread >= 0.25 && travel <= std::max(0.8, spread * 0.8)) {
            phase_ = Phase::Pinch;
            zoom_ = std::clamp(currentZoom, 1.0, config.maxZoom);
            began = true;
        }
        if (phase_ != Phase::Pinch) {
            return {phase_, std::nullopt, false};
        }
        double boost = 1;
        if (!began && elapsedMs > 0 && elapsedMs <= 150) {
            const double speed = std::abs(scaleDelta) * 1000 / elapsedMs;
            velocity_ = hasVelocity_ ? velocity_ + (speed - velocity_) * (1 - std::exp(-elapsedMs / 35.0)) : speed;
            hasVelocity_ = true;
            boost += config.acceleration * std::clamp((velocity_ - 0.4) / 1.6, 0.0, 1.0);
        }
        const double delta = began ? 0 : scaleDelta * (config.invertPinch ? -1 : 1);
        // Integrate from the clamped value so reversing at 100% or the maximum
        // responds on the very next frame instead of traversing a dead zone.
        zoom_ = std::clamp(zoom_ * std::exp(delta * config.sensitivity * boost), 1.0, config.maxZoom);
        return {phase_, zoom_, began};
    }
};

struct View {
    double zoom = 1;
    int x = 0;
    int y = 0;
};
struct Desktop {
    double left;
    double top;
    double width;
    double height;
};
struct Anchor {
    Point content;
    Point display;
};
inline Anchor anchorAt(View view, Point content) {
    return {content, {(content.x - view.x) * view.zoom, (content.y - view.y) * view.zoom}};
}
inline View anchoredView(double zoom, Anchor anchor, Desktop desktop) {
    if (zoom <= 1) return {};
    const double factor = 1 - 1 / zoom;
    return {zoom,
        static_cast<int>(std::lround(std::clamp(anchor.content.x - anchor.display.x / zoom,
                                               desktop.left * factor, (desktop.left + desktop.width) * factor))),
        static_cast<int>(std::lround(std::clamp(anchor.content.y - anchor.display.y / zoom,
                                               desktop.top * factor, (desktop.top + desktop.height) * factor)))};
}
enum class GateMode { Pending, Forward, Suppress };
class GestureGate {
    GateMode mode_ = GateMode::Pending;
public:
    explicit GestureGate(Phase phase = Phase::Pending) { update(phase); }
    GateMode update(Phase phase) {
        if (phase == Phase::Pinch) mode_ = GateMode::Suppress;
        else if (mode_ == GateMode::Pending && phase == Phase::Swipe) mode_ = GateMode::Forward;
        return mode_;
    }
    GateMode finish(Phase phase) {
        update(phase);
        if (mode_ == GateMode::Pending) mode_ = GateMode::Forward;
        return mode_;
    }
    GateMode mode() const { return mode_; }
};
// Finite wheel trajectory in logarithmic zoom space. No OS calls or timer ownership.
class WheelMotion {
    double value_ = 1, from_ = 1, goal_ = 1;
    double rate_ = std::numeric_limits<double>::infinity();
    std::uint64_t started_ = 0, lastTick_ = 0;
    unsigned duration_ = 100;
    int direction_ = 0;
    bool active_ = false;
public:
    bool active() const { return active_; }
    double goal() const { return goal_; }
    void cancel() { active_ = false; direction_ = 0; }
    std::optional<double> tick(std::uint64_t now) {
        if (!active_) return std::nullopt;
        const auto elapsed = now >= lastTick_ ? std::min<std::uint64_t>(now - lastTick_, 64) : 0;
        const double t = now >= started_ ? std::clamp(static_cast<double>(now - started_) / duration_, 0.0, 1.0) : 0;
        const double ease = 1 - std::pow(1 - t, 3);
        const double desired = std::log(from_) + (std::log(goal_) - std::log(from_)) * ease;
        const double previous = std::log(value_);
        const double limit = std::isfinite(rate_) ? rate_ * elapsed / 1000.0 : std::numeric_limits<double>::infinity();
        value_ = std::exp(previous + std::clamp(desired - previous, -limit, limit));
        lastTick_ = now;
        if (t >= 1 && std::abs(std::log(goal_ / value_)) <= 0.000000001) { value_ = goal_; active_ = false; }
        return value_;
    }
    void add(int delta, double current, std::uint64_t now, int step, int duration, double maximum, int maximumRate = 0) {
        if (!delta || !std::isfinite(current) || current < 1) return;
        if (active_) tick(now);
        else value_ = goal_ = current;
        const int direction = delta < 0 ? -1 : 1;
        if (direction_ && direction != direction_) value_ = goal_ = current; // Drop both old goal and not-yet-delivered forward frames.
        direction_ = direction;
        from_ = value_;
        const double cap = std::clamp(maximum, 1.0, 16.0);
        const double notch = std::log1p(std::clamp(step, 1, 100) / 100.0);
        const double distance = std::clamp(delta / 120.0 * notch, -32.0, 32.0);
        rate_ = maximumRate <= 0 ? std::numeric_limits<double>::infinity() : std::log1p(std::clamp(maximumRate, 50, 2000) / 100.0);
        double goal = std::log(goal_) + distance;
        if (std::isfinite(rate_)) {
            // Keep at most 200 ms of motion ahead, while preserving a complete
            // ordinary notch. Excess free-spin input is discarded, never replayed.
            const double lead = std::max(notch, rate_ * 0.20);
            goal = std::clamp(goal, std::log(value_) - lead, std::log(value_) + lead);
        }
        goal_ = std::exp(std::clamp(goal, 0.0, std::log(cap)));
        duration_ = static_cast<unsigned>(std::clamp(duration, 40, 250));
        started_ = lastTick_ = now;
        active_ = std::abs(goal_ - value_) > 0.000000001;
    }
};
// A bounded mailbox keeps the newest direction and preserves fractional wheel
// units. Revision and delta share one CAS packet, so a cancellation cannot mix
// an old generation's amount with a new generation's tag.
constexpr int kWheelPacketLimit = 1200;
inline std::uint64_t wheelPacket(int delta, std::uint32_t revision) {
    return (static_cast<std::uint64_t>(revision) << 32) | static_cast<std::uint32_t>(delta);
}
inline int wheelDelta(std::uint64_t packet) { return static_cast<std::int32_t>(static_cast<std::uint32_t>(packet)); }
inline std::uint32_t wheelRevision(std::uint64_t packet) { return static_cast<std::uint32_t>(packet >> 32); }
inline std::uint64_t mergeWheel(std::uint64_t previous, int delta, std::uint32_t revision) {
    const int old = wheelRevision(previous) == revision ? wheelDelta(previous) : 0;
    const bool sameDirection = (old < 0) == (delta < 0);
    const auto total = static_cast<std::int64_t>(delta) + (sameDirection ? old : 0);
    return wheelPacket(static_cast<int>(std::clamp<std::int64_t>(total, -kWheelPacketLimit, kWheelPacketLimit)), revision);
}
// Stable order across repeated direct swipes, even though activation changes z-order.
class WindowCycle {
    std::vector<std::uintptr_t> order_;
    std::uintptr_t last_ = 0;
public:
    std::uintptr_t next(const std::vector<std::uintptr_t>& available, std::uintptr_t current, int step) {
        if (current != last_) order_ = available;
        else {
            std::erase_if(order_, [&](auto window) { return std::find(available.begin(), available.end(), window) == available.end(); });
            for (auto window : available) if (std::find(order_.begin(), order_.end(), window) == order_.end()) order_.push_back(window);
        }
        if (order_.size() < 2) return 0;
        const auto found = std::find(order_.begin(), order_.end(), current);
        const int size = static_cast<int>(order_.size());
        const int index = found == order_.end() ? (step > 0 ? -1 : 0) : static_cast<int>(found - order_.begin());
        last_ = order_[static_cast<std::size_t>((index + step + size) % size)];
        return last_;
    }
};
inline int swipeDirection(Point movement) {
    if (std::abs(movement.x) > std::abs(movement.y) * 1.25) return movement.x < 0 ? -1 : 1;
    return movement.y < 0 ? -2 : 2;
}
inline View viewAt(double zoom, Point cursor, Desktop desktop) {
    if (!std::isfinite(zoom) || zoom <= 1 || desktop.width <= 0 || desktop.height <= 0) {
        return {};
    }
    const double x = std::clamp(cursor.x, desktop.left, desktop.left + desktop.width);
    const double y = std::clamp(cursor.y, desktop.top, desktop.top + desktop.height);
    return {zoom,
            static_cast<int>(std::lround(x * (1 - 1 / zoom))),
            static_cast<int>(std::lround(y * (1 - 1 / zoom)))};
}
inline double smooth(double current, double target, double elapsed, double duration) {
    if (duration <= 0) {
        return target;
    }
    const double result = current + (target - current) * (1 - std::exp(-elapsed / duration));
    return std::abs(result - target) < 0.0005 ? target : result;
}

// Native key-hold policy. Only real WM_HOTKEY 0/1 sessions use this;
// buttons, other hotkeys and single short taps retain the original behavior.
class KeyboardHold {
    int direction_ = 0, closedDirection_ = 0;
    bool continuous_ = false, closed_ = false;
    std::uint32_t closedAt_ = 0;
    std::uint64_t closedClock_ = 0, began_ = 0, lastTick_ = 0;
    double target_ = 1;
public:
    static constexpr std::uint64_t kDelay = 200;
    static bool atOrBefore(std::uint32_t value, std::uint32_t cutoff) {
        return static_cast<std::int32_t>(value - cutoff) <= 0;
    }
    bool stale(int direction, std::uint32_t timestamp, std::uint64_t now) const {
        return closed_ && direction == closedDirection_ && now >= closedClock_ &&
            now - closedClock_ <= 60000 && atOrBefore(timestamp, closedAt_);
    }
    bool active() const { return direction_ != 0; }
    bool continuous() const { return continuous_; }
    int direction() const { return direction_; }
    void begin(int direction, std::uint64_t now) {
        direction_ = direction; continuous_ = false; began_ = lastTick_ = now;
    }
    bool eligible(std::uint64_t now) const {
        return active() && !continuous_ && now >= began_ && now - began_ >= kDelay;
    }
    void activate(double visible, std::uint64_t now) {
        target_ = std::clamp(visible, 1.0, 16.0); continuous_ = true; lastTick_ = now;
    }
    std::optional<double> tick(std::uint64_t now, int speed, double maximum) {
        if (!continuous_ || now < lastTick_) return std::nullopt;
        const auto elapsed = std::min<std::uint64_t>(now - lastTick_, 64);
        lastTick_ = now;
        if (!elapsed) return std::nullopt;
        const double age = now >= began_ + kDelay ? static_cast<double>(now - began_ - kDelay) : 0;
        const double t = std::clamp(age / 1200.0, 0.0, 1.0);
        const double ramp = t * t * (3 - 2 * t);
        const double maximumRate = std::log1p(2.0 * std::clamp(speed, 2, 4));
        const double rate = 0.35 + (maximumRate - 0.35) * ramp;
        const double cap = std::clamp(maximum, 1.0, 16.0);
        const double next = std::clamp(target_ * std::exp(direction_ * rate * elapsed / 1000.0), 1.0, cap);
        if (std::abs(next - target_) < 0.00000001) return std::nullopt;
        target_ = next;
        return target_;
    }
    bool close(std::uint64_t now, bool seal = true) {
        const bool freeze = continuous_;
        if (seal && direction_) {
            closed_ = true; closedDirection_ = direction_;
            closedAt_ = static_cast<std::uint32_t>(now); closedClock_ = now;
        }
        direction_ = 0; continuous_ = false;
        return freeze;
    }
};

// Observe physical key state without suppressing or synthesizing modifiers.
// A consumed Win+mouse action needs one neutral menu mask unless Windows has
// already seen another physical keyboard combination in this Win session.
class WinMenuMask {
    std::array<std::uint64_t, 4> down_{};
    bool keyboardUsed_ = false;
    bool maskIssued_ = false;
    static constexpr unsigned leftWin = 0x5b, rightWin = 0x5c;
    bool held(unsigned key) const { return key < 256 && (down_[key / 64] & (std::uint64_t{1} << (key % 64))); }
    bool otherHeld() const {
        for (unsigned i = 0; i < down_.size(); ++i) {
            auto keys = down_[i];
            if (i == leftWin / 64) keys &= ~((std::uint64_t{1} << (leftWin % 64)) | (std::uint64_t{1} << (rightWin % 64)));
            if (keys) return true;
        }
        return false;
    }
public:
    void key(unsigned vk, bool down, bool injected = false) {
        if (injected || vk >= 256) return;
        const bool wasWin = winDown();
        const bool isWin = vk == leftWin || vk == rightWin;
        if (down) down_[vk / 64] |= std::uint64_t{1} << (vk % 64);
        else down_[vk / 64] &= ~(std::uint64_t{1} << (vk % 64));
        if (isWin) {
            if (!wasWin && winDown()) { keyboardUsed_ = otherHeld(); maskIssued_ = false; }
            else if (!winDown()) { keyboardUsed_ = false; maskIssued_ = false; }
        } else if (wasWin) keyboardUsed_ = true;
    }
    bool winDown() const { return held(leftWin) || held(rightWin); }
    bool winOnly() const { return winDown() && !otherHeld(); }
    bool consume() {
        if (!winOnly() || keyboardUsed_ || maskIssued_) return false;
        maskIssued_ = true;
        return true;
    }
    void maskFailed() { maskIssued_ = false; }
    void reset() { down_.fill(0); keyboardUsed_ = false; maskIssued_ = false; }
};

// Window switching is discrete even for free-spinning/high-resolution wheels.
// No timer flushes this accumulator: pending movement needs a fresh wheel event.
class WindowWheel {
    int residual_ = 0;
    std::uint64_t lastInput_ = 0, lastAction_ = 0;
    bool haveInput_ = false, haveAction_ = false;
public:
    void reset() { residual_ = 0; lastInput_ = lastAction_ = 0; haveInput_ = haveAction_ = false; }
    std::optional<int> feed(int delta, std::uint64_t now) {
        if (!delta) return std::nullopt;
        if ((haveInput_ && now < lastInput_) || (haveAction_ && now < lastAction_)) reset();
        if (haveInput_ && now - lastInput_ > 200) residual_ = 0;
        if (residual_ && ((residual_ < 0) != (delta < 0))) residual_ = 0;
        lastInput_ = now; haveInput_ = true;
        const auto total = static_cast<std::int64_t>(residual_) + delta;
        const bool cooling = haveAction_ && now - lastAction_ < 125;
        residual_ = static_cast<int>(std::clamp<std::int64_t>(total, cooling ? -120 : -240, cooling ? 120 : 240));
        if (cooling || (residual_ > -120 && residual_ < 120)) return std::nullopt;
        const int sign = residual_ < 0 ? -1 : 1;
        residual_ -= sign * 120;
        lastAction_ = now; haveAction_ = true;
        return -sign; // Positive wheel = previous window; negative wheel = next.
    }
};

class MouseButtonOwnership {
    unsigned pressed_ = 0, owned_ = 0;
public:
    bool pressed(unsigned button) const { return button < 3 && (pressed_ & (1u << button)) != 0; }
    bool owned(unsigned button) const { return button < 3 && (owned_ & (1u << button)) != 0; }
    bool press(unsigned button, bool consume) {
        if (button >= 3) return false; // 0 left, 1 middle, 2 right.
        const unsigned bit = 1u << button;
        if (!(pressed_ & bit)) {
            pressed_ |= bit;
            if (consume) owned_ |= bit;
            else owned_ &= ~bit;
        }
        return (owned_ & bit) != 0;
    }
    bool release(unsigned button) {
        if (button >= 3) return false;
        const unsigned bit = 1u << button;
        const bool owned = (owned_ & bit) != 0;
        pressed_ &= ~bit; owned_ &= ~bit;
        return owned;
    }
    void reset() { pressed_ = owned_ = 0; }
};

}  // namespace pinch

// Capability planning is pure. Resolving optional symbols never weakens the
// required native ABI or consumes an input when its complete backend is absent.
namespace compatibility {
enum NativeCapability : std::uint32_t {
    NativeCore = 1, NativeKeyboardObserver = 2, NativeWheel = 4,
    NativeViewportRestore = 8, NativeBootstrapRun = 16,
    NativeBootstrapWindows = 32, NativeHideUi = 64
};
enum GateCapability : std::uint32_t {
    GateSwipe = 1, GateVisibility = 2, GateDesktop = 4
};
constexpr std::uint32_t symbol(unsigned index) { return std::uint32_t{1} << index; }
struct Plan {
    std::uint32_t capabilities = 0;
    bool supported = false;
};
constexpr Plan nativePlan(std::uint32_t found) {
    constexpr auto required = symbol(0) | symbol(1) | symbol(2) |
                              symbol(3) | symbol(5) | symbol(7);
    constexpr auto windows = symbol(10) | symbol(11);
    if ((found & required) != required ||
        (!(found & symbol(4)) && (found & windows) != windows)) return {};
    std::uint32_t capabilities = NativeCore | NativeHideUi;
    if (found & symbol(4)) capabilities |= NativeBootstrapRun;
    if ((found & windows) == windows) capabilities |= NativeBootstrapWindows;
    if (found & symbol(6)) capabilities |= NativeKeyboardObserver;
    if (found & symbol(8)) capabilities |= NativeViewportRestore;
    if (found & symbol(9)) capabilities |= NativeWheel;
    return {capabilities, true};
}
constexpr Plan gatePlan(std::uint32_t found, bool desktop) {
    constexpr auto required = symbol(0) | symbol(1) | symbol(2) | symbol(3);
    constexpr auto visibility = symbol(4) | symbol(5);
    if ((found & required) != required) return {};
    std::uint32_t capabilities = GateSwipe;
    if ((found & visibility) == visibility) capabilities |= GateVisibility;
    if (desktop) capabilities |= GateDesktop;
    return {capabilities, true};
}
constexpr bool nativeDeclined(std::uint32_t capabilities, long reason) {
    return reason != 0 && !(capabilities & NativeCore);
}
constexpr bool mouseZoomAllowed(std::uint32_t capabilities, long reason, long error) {
    return !nativeDeclined(capabilities, reason) && error != 50; // ERROR_NOT_SUPPORTED, including lens/docked.
}
constexpr bool overviewAllowed(std::uint32_t native, std::uint32_t gate) {
    return (native & NativeViewportRestore) && (native & NativeKeyboardObserver) && (gate & GateVisibility);
}
}  // namespace compatibility

#ifndef PINCH_ZOOM_CORE_TEST

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <hidsdi.h>
#include <dwmapi.h>
#include <hidpi.h>
#include <shellapi.h>
#include <objbase.h>
#include <servprov.h>
#include <new>
#include <wtsapi32.h>
#include <atomic>
#include <cstdio>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#ifndef PINCH_ZOOM_STANDALONE
#include <windhawk_api.h>
#endif

// Offline profiles are tied to the loaded image's exact CodeView GUID/age,
// PE timestamp and image size. Never reuse offsets after an identity change.
namespace binary_profiles {
enum class Kind { Magnifier, Shell, Desktop };
struct Identity {
    std::array<unsigned char, 16> guid{};
    DWORD age = 0, imageSize = 0, timestamp = 0;
    bool operator==(const Identity&) const = default;
};
struct Profile {
    Kind kind;
    Identity identity;
    std::array<DWORD, 12> rvas{};
    std::size_t count;
};
constexpr Profile profiles[] = {
    {Kind::Magnifier, {{0x99,0x46,0x73,0xd3,0xf7,0xad,0x69,0xbc,0x0b,0x62,0xa7,0xf4,0x56,0x60,0x23,0x4f},1,0x130000,0x5a1425a3}, {0x258ac,0x78108,0x77a7c,0x77b3c,0x547b0,0x52d3c,0x53330,0x1c2e8,0x2852c,0x52aa4,0x55a20,0x2a520}, 12},
    {Kind::Shell, {{0xda,0xf3,0x07,0x95,0x7f,0xd4,0x02,0x5f,0x65,0x69,0xfc,0x50,0x27,0x3c,0x40,0xd7},1,0x9a5000,0xbc86bb4d}, {0x16bec0,0x16c2c0,0x16bde0,0x4d5ee0,0x2198b0,0x216860}, 6},
    {Kind::Desktop, {{0x27,0x6f,0x66,0x1d,0x2f,0xba,0x88,0x85,0x58,0x88,0xd3,0xe2,0xb9,0x4b,0x0f,0xd8},1,0x344000,0xbee7e13f}, {0x2839dc}, 1},
};
inline bool range(DWORD rva, std::size_t length, DWORD imageSize) {
    return rva <= imageSize && length <= imageSize - rva;
}
const IMAGE_NT_HEADERS64* headers(HMODULE module) {
    if (!module) return nullptr;
    const auto* base = reinterpret_cast<const BYTE*>(module);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < static_cast<LONG>(sizeof(*dos)) || dos->e_lfanew > 0x800) return nullptr;
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC || nt->FileHeader.NumberOfSections > 96 ||
        nt->OptionalHeader.SizeOfImage < 4096 || nt->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_DEBUG) return nullptr;
    return nt;
}
std::optional<Identity> identify(HMODULE module) {
    const auto* nt = headers(module);
    if (!nt) return std::nullopt;
    const auto* base = reinterpret_cast<const BYTE*>(module);
    const DWORD size = nt->OptionalHeader.SizeOfImage;
    const auto directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    if (!directory.VirtualAddress || !range(directory.VirtualAddress, directory.Size, size) ||
        directory.Size / sizeof(IMAGE_DEBUG_DIRECTORY) > 64) return std::nullopt;
    const auto* entries = reinterpret_cast<const IMAGE_DEBUG_DIRECTORY*>(base + directory.VirtualAddress);
    for (std::size_t i = 0; i < directory.Size / sizeof(*entries); ++i) {
        const auto& entry = entries[i];
        if (entry.Type != IMAGE_DEBUG_TYPE_CODEVIEW || entry.SizeOfData < 24 || !range(entry.AddressOfRawData, entry.SizeOfData, size)) continue;
        const auto* data = base + entry.AddressOfRawData;
        if (data[0] != 'R' || data[1] != 'S' || data[2] != 'D' || data[3] != 'S') continue;
        Identity result;
        std::copy_n(data + 4, 16, result.guid.begin());
        // CodeView age is little endian and may not be aligned.
        result.age = static_cast<DWORD>(data[20]) | (static_cast<DWORD>(data[21]) << 8) |
                     (static_cast<DWORD>(data[22]) << 16) | (static_cast<DWORD>(data[23]) << 24);
        result.imageSize = size; result.timestamp = nt->FileHeader.TimeDateStamp;
        return result;
    }
    return std::nullopt;
}
bool permitted(HMODULE module, DWORD rva, bool data) {
    const auto* nt = headers(module);
    if (!nt || !rva || !range(rva, data ? sizeof(LONG) : 1, nt->OptionalHeader.SizeOfImage)) return false;
    const auto* sections = IMAGE_FIRST_SECTION(nt);
    const auto offset = reinterpret_cast<const BYTE*>(sections) - reinterpret_cast<const BYTE*>(module);
    if (offset < 0 || static_cast<std::size_t>(offset) + nt->FileHeader.NumberOfSections * sizeof(*sections) > nt->OptionalHeader.SizeOfHeaders) return false;
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        const auto& section = sections[i];
        const DWORD length = std::max(section.Misc.VirtualSize, section.SizeOfRawData);
        if (rva >= section.VirtualAddress && rva - section.VirtualAddress < length &&
            (!data || (rva % alignof(LONG) == 0 && length - (rva - section.VirtualAddress) >= sizeof(LONG))))
            return data ? (section.Characteristics & IMAGE_SCN_MEM_WRITE) != 0 : (section.Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
    }
    return false;
}
bool resolve(HMODULE module, Kind kind, void** targets, std::size_t count) {
    std::fill_n(targets, count, nullptr);
    const auto identity = identify(module);
    if (!identity) return false;
    for (const auto& profile : profiles) {
        if (profile.kind != kind || profile.count != count || profile.identity != *identity) continue;
        for (std::size_t i = 0; i < count; ++i) if (!permitted(module, profile.rvas[i], kind == Kind::Desktop)) return false;
        for (std::size_t i = 0; i < count; ++i) targets[i] = reinterpret_cast<BYTE*>(module) + profile.rvas[i];
        return true;
    }
    return false;
}
} // namespace binary_profiles

namespace bridge {
enum class InputSource : LONG { None = 0, Pinch = 1, Wheel = 2, Keyboard = 3, Reset = 4 };
inline volatile LONG* desktopRaised = nullptr;
constexpr wchar_t kMapping[] = L"Local\\KdePinchZoom.NativeGate.v13";
constexpr wchar_t kWake[] = L"Local\\KdePinchZoom.NativeWake.v13";
struct alignas(8) Record {
    volatile LONG phase;
    volatile LONG down;
    volatile LONG enabled;
    volatile LONG gateReady;
    volatile LONG shellPid;
    LONG reserved;
    volatile LONG64 heartbeat;
    volatile LONG magnifierPid;
    volatile LONG magnifierReady;
    volatile LONG controlEnabled;
    volatile LONG hideUi;
    volatile LONG keyboardStep;
    volatile LONG keyboardHoldMultiplier;
    volatile LONG keyboardHoldRepeats;
    volatile LONG keyboardHoldFreezes;
    volatile LONG maxZoomPercent;
    volatile LONG syncRequest;
    volatile LONG nativeCommands;
    volatile LONG pendingCommand;
    volatile LONG nativeError;
    volatile LONG nativeCaps;
    volatile LONG gateCaps;
    volatile LONG resolverReasonNative;
    volatile LONG resolverReasonGate;
    volatile LONG resolverModeNative; // 0 unavailable, 1 exact offline profile, 2 Microsoft symbols.
    volatile LONG resolverModeGate;
    volatile LONG resolverModeDesktop;
    volatile LONG nativeLevel; // Native target in 1/10000 units, not an animation frame.
    volatile LONG requestedLevel;
    volatile LONG requestedSequence;
    volatile LONG appliedSequence;
    volatile LONG startAtNormal;
    volatile LONG zoomRevision;
    volatile LONG requestRevision;
    volatile LONG blockedCursorMoves;
    volatile LONG lastCursorOrigin;
    volatile LONG instantSwitch;
    volatile LONG normalInOverview;
    volatile LONG overviewBypassed;
    volatile LONG swipeDirection;
    volatile LONG shellMask;
    volatile LONG shellRevision;
    volatile LONG shellApplied;
    volatile LONG pendingGestures;
    volatile LONG blockedCursorClips;
    volatile LONG wheelEnabled;
    volatile LONG wheelActive;
    volatile LONG wheelEvents;
    volatile LONG inputRevision;
    volatile LONG64 hostWindow;
    volatile LONG64 wheelPacket;
    volatile LONG64 wheelTime;
    volatile LONG requestSource;
    volatile LONG requestInputRevision;
    volatile LONG currentSource;
    volatile LONG overviewSavedLevel;
    volatile LONG winWheel;
    volatile LONG wheelWindowSwitch;
    volatile LONG disableReading;
    volatile LONG nativeViewMode; // 0 unknown; 1 docked; 2 fullscreen; 3 lens, published by the native UI owner.
    volatile LONG64 windowWheelPacket;
    volatile LONG64 windowWheelTime;
    volatile LONG mouseHooksReady;
    volatile LONG readingReady;
};
// Only the native UI produces wheel input. The host consumes one bounded
// packet per frame; no per-delta posted messages can starve WM_TIMER.
inline void enqueueWheel(Record* record, int delta, LONG revision, std::uint64_t now) {
    if (!record || !delta || InterlockedCompareExchange(&record->inputRevision, 0, 0) != revision) return;
    const auto previousTime = static_cast<std::uint64_t>(InterlockedCompareExchange64(&record->wheelTime, 0, 0));
    InterlockedExchange64(&record->wheelTime, static_cast<LONG64>(now));
    LONG64 observed = InterlockedCompareExchange64(&record->wheelPacket, 0, 0);
    for (;;) {
        const bool stale = now < previousTime || now - previousTime > 200;
        const auto merged = pinch::mergeWheel(stale ? 0 : static_cast<std::uint64_t>(observed), delta, static_cast<std::uint32_t>(revision));
        const auto desired = static_cast<LONG64>(merged);
        const LONG64 actual = InterlockedCompareExchange64(&record->wheelPacket, desired, observed);
        if (actual == observed) return;
        observed = actual;
        if (InterlockedCompareExchange(&record->inputRevision, 0, 0) != revision) return;
    }
}
inline void enqueueWindowWheel(Record* record, int delta, std::uint64_t now) {
    if (!record || !delta) return;
    const auto before = static_cast<std::uint64_t>(InterlockedExchange64(&record->windowWheelTime, static_cast<LONG64>(now)));
    LONG64 old = InterlockedCompareExchange64(&record->windowWheelPacket, 0, 0);
    for (;;) {
        const auto packet = pinch::mergeWheel(now < before || now - before > 200 ? 0 : static_cast<std::uint64_t>(old), delta, 0);
        const LONG64 actual = InterlockedCompareExchange64(&record->windowWheelPacket, static_cast<LONG64>(packet), old);
        if (actual == old) return;
        old = actual;
    }
}
class Mapping {
    HANDLE mapping_ = nullptr;
    HANDLE wake_ = nullptr;
#if defined(PINCH_ZOOM_NATIVE_TEST) || defined(PINCH_ZOOM_RUNTIME_TEST)
    bool testRecord_ = false;
#endif
    Record* record_ = nullptr;
    mutable HANDLE shellProcess_ = nullptr;
    mutable DWORD shellPid_ = 0;
    mutable HANDLE magnifierProcess_ = nullptr;
    mutable DWORD magnifierPid_ = 0;
public:
    bool open() {
        if (record_) return true;
        mapping_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(Record), kMapping);
        if (mapping_) record_ = static_cast<Record*>(MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Record)));
        if (record_) wake_ = CreateEventW(nullptr, FALSE, FALSE, kWake);
        return record_ != nullptr;
    }
    HANDLE wakeEvent() const { return wake_; }
    void wake() { if (wake_) SetEvent(wake_); }
    Record* get() const { return record_; }
#if defined(PINCH_ZOOM_NATIVE_TEST) || defined(PINCH_ZOOM_RUNTIME_TEST)
    void useTestRecord(Record* record) { record_ = record; testRecord_ = true; }
#endif
    bool magnifierProcessAlive() const {
        if (!record_) return false;
        const auto pid = static_cast<DWORD>(InterlockedCompareExchange(&record_->magnifierPid, 0, 0));
        if (pid != magnifierPid_) {
            if (magnifierProcess_) CloseHandle(magnifierProcess_);
            magnifierProcess_ = OpenProcess(SYNCHRONIZE, FALSE, pid);
            magnifierPid_ = pid;
        }
        return magnifierProcess_ && WaitForSingleObject(magnifierProcess_, 0) == WAIT_TIMEOUT;
    }
    bool magnifierAvailable() const {
        return record_ && InterlockedCompareExchange(&record_->magnifierReady, 0, 0) && magnifierProcessAlive();
    }
    void shellReason(LONG bit, bool visible) {
        if (!record_) return;
        const LONG previous = visible ? InterlockedOr(&record_->shellMask, bit) : InterlockedAnd(&record_->shellMask, ~bit);
        if (((previous & bit) != 0) != visible) { InterlockedIncrement(&record_->shellRevision); wake(); }
    }
    void configure(const pinch::Config& config) {
        if (!record_) return;
        InterlockedExchange(&record_->controlEnabled, config.enabled && !config.observeOnly && config.keyboardInterop ? 1 : 0);
        InterlockedExchange(&record_->wheelEnabled, config.smoothWheel && config.enabled && !config.observeOnly && config.keyboardInterop ? 1 : 0);
        InterlockedExchange(&record_->hideUi, config.hideMagnifier ? 1 : 0);
        InterlockedExchange(&record_->winWheel, config.winWheel ? 1 : 0);
        InterlockedExchange(&record_->wheelWindowSwitch, config.wheelWindowSwitch ? 1 : 0);
        InterlockedExchange(&record_->disableReading, config.disableReading ? 1 : 0);
        InterlockedExchange(&record_->instantSwitch, config.instantHorizontalSwitch && config.enabled && !config.observeOnly ? 1 : 0);
        InterlockedExchange(&record_->normalInOverview, config.normalZoomInOverview ? 1 : 0);
        InterlockedExchange(&record_->keyboardStep, config.keyboardStep);
        InterlockedExchange(&record_->keyboardHoldMultiplier, std::clamp(config.keyboardHoldMultiplier, 1, 4));
        InterlockedExchange(&record_->maxZoomPercent, static_cast<LONG>(config.maxZoom * 100));
        if (!config.enabled || config.observeOnly) {
            InterlockedExchange(&record_->appliedSequence, record_->requestedSequence);
            InterlockedExchange(&record_->pendingCommand, 0);
        }
        wake();
    }
    double nativeLevel() const {
        const LONG value = record_ ? InterlockedCompareExchange(&record_->nativeLevel, 0, 0) : 0;
        return std::max(1.0, value / 10000.0);
    }
    bool requestLevel(double level, LONG expectedRevision = -1, InputSource source = InputSource::Pinch) {
        if (!magnifierAvailable()) return false;
        const LONG revision = InterlockedCompareExchange(&record_->zoomRevision, 0, 0);
        if (expectedRevision >= 0 && revision != expectedRevision) return true; // Rebase on the next input frame.
        InterlockedExchange(&record_->requestRevision, revision);
        InterlockedExchange(&record_->requestedLevel, static_cast<LONG>(std::lround(std::clamp(level, 1.0, 16.0) * 10000)));
        InterlockedExchange(&record_->requestSource, static_cast<LONG>(source));
        InterlockedExchange(&record_->requestInputRevision, InterlockedCompareExchange(&record_->inputRevision, 0, 0));
        InterlockedIncrement(&record_->requestedSequence);
        wake();
        return true;
    }
    void syncNative() { if (record_) { InterlockedIncrement(&record_->syncRequest); wake(); } }
    bool threeContacts() const { return record_ && InterlockedCompareExchange(&record_->down, 0, 0) != 0; }
    bool hostAvailable() const {
        if (!record_ || GetTickCount64() - static_cast<ULONGLONG>(InterlockedCompareExchange64(&record_->heartbeat, 0, 0)) > 250) return false;
        DWORD owner = 0;
        const auto window = reinterpret_cast<HWND>(InterlockedCompareExchange64(&record_->hostWindow, 0, 0));
        return window && GetWindowThreadProcessId(window, &owner) && owner == static_cast<DWORD>(record_->shellPid);
    }
    bool gateAvailable() const {
        if (!record_ || !InterlockedCompareExchange(&record_->gateReady, 0, 0)) return false;
        const auto pid = static_cast<DWORD>(InterlockedCompareExchange(&record_->shellPid, 0, 0));
        if (pid != shellPid_) {
            if (shellProcess_) CloseHandle(shellProcess_);
            shellProcess_ = OpenProcess(SYNCHRONIZE, FALSE, pid);
            shellPid_ = pid;
        }
        return shellProcess_ && WaitForSingleObject(shellProcess_, 0) == WAIT_TIMEOUT;
    }
    void publish(pinch::Phase phase, bool down, bool enabled) {
        if (!record_) return;
        // Keep the last classification after lift, so FinishSwipe can suppress
        // the same pinch even when its final HID frame has already arrived.
        if (phase != pinch::Phase::Idle) InterlockedExchange(&record_->phase, static_cast<LONG>(phase));
        InterlockedExchange(&record_->down, down ? 1 : 0);
        InterlockedExchange(&record_->enabled, enabled ? 1 : 0);
        InterlockedExchange64(&record_->heartbeat, static_cast<LONG64>(GetTickCount64()));
    }
    void heartbeat() {
        if (record_) InterlockedExchange64(&record_->heartbeat, static_cast<LONG64>(GetTickCount64()));
    }
    pinch::Phase phase(bool finishing = false) const {
        if (!record_ || !InterlockedCompareExchange(&record_->enabled, 0, 0) ||
            GetTickCount64() - static_cast<ULONGLONG>(InterlockedCompareExchange64(&record_->heartbeat, 0, 0)) > 250) {
            return pinch::Phase::Swipe; // No live host: preserve native handling.
        }
        if (!finishing && !InterlockedCompareExchange(&record_->down, 0, 0)) return pinch::Phase::Pending;
        const auto value = InterlockedCompareExchange(&record_->phase, 0, 0);
        return static_cast<pinch::Phase>(value);
    }
    ~Mapping() {
        if (magnifierProcess_) CloseHandle(magnifierProcess_);
        if (shellProcess_) CloseHandle(shellProcess_);
#if defined(PINCH_ZOOM_NATIVE_TEST) || defined(PINCH_ZOOM_RUNTIME_TEST)
        if (!testRecord_ && record_) UnmapViewOfFile(record_);
#else
        if (record_) UnmapViewOfFile(record_);
#endif
        if (mapping_) CloseHandle(mapping_);
        if (wake_) CloseHandle(wake_);
    }
};
}  // namespace bridge

// Public Shell launch-site interfaces. Windhawk's MinGW headers omit these
// declarations; the ABI/GUIDs below follow shobjidl_core.h from Windows SDK.
// https://learn.microsoft.com/windows/win32/api/shobjidl_core/nn-shobjidl_core-icreatingprocess
namespace shell_feedback {
inline constexpr GUID kUnknown{0x00000000,0x0000,0x0000,{0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
inline constexpr GUID kServiceProvider{0x6d5140c1,0x7436,0x11ce,{0x80,0x34,0x00,0xaa,0x00,0x60,0x09,0xfa}};
inline constexpr GUID kCreatingProcess{0xc2b937a9,0x3110,0x4398,{0x8a,0x56,0xf3,0x4c,0x63,0x42,0xd2,0x44}};
struct CreateInputs : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetCreateFlags(DWORD*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCreateFlags(DWORD) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddCreateFlags(DWORD) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetHotKey(WORD) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddStartupFlags(DWORD) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTitle(LPCWSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetEnvironmentVariable(LPCWSTR, LPCWSTR) = 0;
};
struct CreatingProcess : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE OnCreating(CreateInputs*) = 0;
};
class Site final : public IServiceProvider, public CreatingProcess {
    std::atomic<ULONG> references_{1};
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** result) override {
        if (!result) return E_POINTER;
        *result = nullptr;
        if (IsEqualIID(iid,kUnknown) || IsEqualIID(iid,kServiceProvider)) *result = static_cast<IServiceProvider*>(this);
        else if (IsEqualIID(iid,kCreatingProcess)) *result = static_cast<CreatingProcess*>(this);
        else return E_NOINTERFACE;
        AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return references_.fetch_add(1,std::memory_order_relaxed)+1; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG remaining=references_.fetch_sub(1,std::memory_order_acq_rel)-1;
        if (!remaining) delete this;
        return remaining;
    }
    HRESULT STDMETHODCALLTYPE QueryService(REFGUID service, REFIID iid, void** result) override {
        if (!result) return E_POINTER;
        *result = nullptr;
        if (!IsEqualGUID(service,kCreatingProcess)) return E_NOINTERFACE;
        return QueryInterface(iid,result);
    }
    HRESULT STDMETHODCALLTYPE OnCreating(CreateInputs* inputs) override {
        if (!inputs) return E_POINTER;
        inputs->AddStartupFlags(STARTF_FORCEOFFFEEDBACK); // Cosmetic hint is best effort; never prevent native launch.
        return S_OK;
    }
};
struct Apartment {
    HRESULT result = CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    ~Apartment() { if (SUCCEEDED(result)) CoUninitialize(); }
    bool usable() const { return SUCCEEDED(result) || result == RPC_E_CHANGED_MODE; }
};
using Execute = decltype(&ShellExecuteExW);
BOOL execute(SHELLEXECUTEINFOW& launch, bool suppress, Execute original = ShellExecuteExW) {
    if (!suppress) return original(&launch);
    struct LastError {
        DWORD value = 0; bool restore = false;
        ~LastError() { if (restore) SetLastError(value); }
    } lastError;
    const auto invoke = [&] { const BOOL result = original(&launch); lastError.value = GetLastError(); lastError.restore = true; return result; };
    Apartment apartment;
    if (!apartment.usable()) return invoke(); // Native launch still works if COM setup is unavailable.
    std::unique_ptr<Site,void(*)(Site*)> site(new(std::nothrow) Site, [](Site* value) { if (value) value->Release(); });
    if (!site) return invoke();
    // NOASYNC prevents retaining a callback into this mod after launch returns.
    // Do not set WAITFORINPUTIDLE: launching must not wait for the GUI to draw.
    launch.fMask |= SEE_MASK_FLAG_HINST_IS_SITE | SEE_MASK_NOASYNC;
    launch.hInstApp = reinterpret_cast<HINSTANCE>(static_cast<IServiceProvider*>(site.get()));
    return invoke();
}
} // namespace shell_feedback

namespace runtime {

constexpr USAGE kDigitizer = 0x0D;
constexpr USAGE kTouchpad = 0x05;
constexpr USAGE kContactId = 0x51;
constexpr USAGE kContactCount = 0x54;
constexpr USAGE kScanTime = 0x56;
constexpr USAGE kTip = 0x42;
constexpr USAGE kConfidence = 0x47;
constexpr UINT kStopMessage = WM_APP + 1;
constexpr UINT kSettingsMessage = WM_APP + 2;
constexpr UINT kWheelMessage = WM_APP + 3;
constexpr UINT kMouseActionMessage = WM_APP + 4;
constexpr UINT_PTR kTimer = 1;
constexpr int kResetHotkey = 1;
constexpr int kPauseHotkey = 2;
constexpr wchar_t kClassName[] = L"WindhawkKdePinchZoom_0_1";

void log(const wchar_t* message) {
#ifdef PINCH_ZOOM_STANDALONE
    std::fwprintf(stdout, L"%ls\n", message);
    std::fflush(stdout);
#else
    Wh_Log(L"%s", message);
#endif
}

struct Slot {
    USHORT collection;
    UCHAR reportId;
    HIDP_VALUE_CAPS x;
    HIDP_VALUE_CAPS y;
    bool hasId = false;
    bool hasTip = false;
    bool hasConfidence = false;
};
struct Device {
    std::vector<BYTE> preparsed;
    std::vector<Slot> slots;
    USHORT countCollection = 0;
    USHORT scanCollection = 0;
    bool hasCount = false;
    bool hasScan = false;
    bool valid = false;
    pinch::FrameAssembler assembler;
    pinch::Recognizer recognizer;
    pinch::FrameTiming timing;
    std::optional<std::uint32_t> frameScan;
    unsigned activeContacts = 0;
    bool waitingForNative = false;
    double pendingPinchRadius = 0, pendingPinchRatio = 1;
    std::optional<double> coldPinchSeed;
    std::uint64_t coldPinchUntil = 0;
    LONG coldPinchCommands = 0;
};

bool hasUsage(const HIDP_VALUE_CAPS& cap, USAGE usage) {
    return cap.IsRange ? cap.Range.UsageMin <= usage && usage <= cap.Range.UsageMax
                       : cap.NotRange.Usage == usage;
}
bool hasUsage(const HIDP_BUTTON_CAPS& cap, USAGE usage) {
    return cap.IsRange ? cap.Range.UsageMin <= usage && usage <= cap.Range.UsageMax
                       : cap.NotRange.Usage == usage;
}

double logicalMinimum(const HIDP_VALUE_CAPS& cap) { return cap.LogicalMin; }
double logicalMaximum(const HIDP_VALUE_CAPS& cap) {
    if (cap.LogicalMin >= 0 && cap.LogicalMax < cap.LogicalMin && cap.BitSize <= 32) {
        return std::pow(2.0, cap.BitSize) - 1;
    }
    return cap.LogicalMax;
}
double physicalMillimeters(const HIDP_VALUE_CAPS& cap) {
    const double range = static_cast<double>(cap.PhysicalMax) - cap.PhysicalMin;
    const unsigned system = cap.Units & 0xF;
    // HID linear centimetres (SI) or inches (English), length exponent 1.
    if (range <= 0 || (cap.Units & 0xFFFFFFF0u) != 0x10 ||
        (system != 1 && system != 3)) {
        return 0;
    }
    int exponent = cap.UnitsExp & 0xF;
    if (exponent >= 8) {
        exponent -= 16;
    }
    const double value = range * std::pow(10.0, exponent) * (system == 1 ? 10 : 25.4);
    return value >= 10 && value <= 500 ? value : 0;
}
double coordinate(ULONG raw, const HIDP_VALUE_CAPS& cap, double length) {
    std::int64_t value = raw;
    if (cap.LogicalMin < 0 && cap.BitSize > 0 && cap.BitSize < 32 &&
        (raw & (1u << (cap.BitSize - 1)))) {
        value -= (std::int64_t{1} << cap.BitSize);
    } else if (cap.LogicalMin < 0 && cap.BitSize == 32) {
        value = static_cast<std::int32_t>(raw);
    }
    return (value - logicalMinimum(cap)) /
           (logicalMaximum(cap) - logicalMinimum(cap)) * length;
}

class Zoom {
    HMODULE module_ = nullptr;
    using Initialize = BOOL(WINAPI*)();
    using GetTransform = BOOL(WINAPI*)(float*, int*, int*);
    using SetTransform = BOOL(WINAPI*)(float, int, int);
    Initialize uninitialize_ = nullptr;
    GetTransform get_ = nullptr;
    SetTransform set_ = nullptr;
    bool initialized_ = false;
    bool owned_ = false;
    pinch::View original_{};
    pinch::View last_{};
    double target_ = 1;
    pinch::Anchor anchor_{};
    POINT lastCursor_{};
    bool pinching_ = false;
    unsigned revision_ = 0;
    decltype(&GetCursorPos) getCursor_ = GetCursorPos;
    bridge::Mapping native_;
    bool nativeRouting_ = true;
    LONG keyboardRevision_ = 0;
    bool nativeActive() const { return nativeRouting_ && native_.magnifierAvailable(); }

    static pinch::Desktop desktop() {
        return {static_cast<double>(GetSystemMetrics(SM_XVIRTUALSCREEN)),
                static_cast<double>(GetSystemMetrics(SM_YVIRTUALSCREEN)),
                static_cast<double>(GetSystemMetrics(SM_CXVIRTUALSCREEN)),
                static_cast<double>(GetSystemMetrics(SM_CYVIRTUALSCREEN))};
    }

    std::optional<pinch::View> read() const {
        float factor;
        int x, y;
        if (!get_ || !get_(&factor, &x, &y)) {
            return std::nullopt;
        }
        return pinch::View{factor, x, y};
    }
    static bool same(const pinch::View& a, const pinch::View& b) {
        return std::abs(a.zoom - b.zoom) < 0.001 && a.x == b.x && a.y == b.y;
    }

public:
#ifdef PINCH_ZOOM_RUNTIME_TEST
    void useTestBackend(GetTransform readFn, SetTransform writeFn, decltype(&GetCursorPos) cursorFn, bool native = false) {
        get_ = readFn;
        set_ = writeFn;
        getCursor_ = cursorFn;
        initialized_ = true;
        nativeRouting_ = native;
    }
    void useTestNativeRecord(bridge::Record* record) { native_.useTestRecord(record); }
#endif
    bool initialize(bool standaloneApi = false) {
        nativeRouting_ = !standaloneApi;
        native_.open();
        // Windhawk's MinGW SDK omits these function declarations and import
        // library. Resolve the documented API from the system DLL explicitly.
        module_ = LoadLibraryExW(L"Magnification.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module_) return false;
        const auto init = reinterpret_cast<Initialize>(GetProcAddress(module_, "MagInitialize"));
        uninitialize_ = reinterpret_cast<Initialize>(GetProcAddress(module_, "MagUninitialize"));
        get_ = reinterpret_cast<GetTransform>(GetProcAddress(module_, "MagGetFullscreenTransform"));
        set_ = reinterpret_cast<SetTransform>(GetProcAddress(module_, "MagSetFullscreenTransform"));
        initialized_ = init && uninitialize_ && get_ && set_ && init() != FALSE;
        return initialized_;
    }
    double current() const {
        if (nativeActive()) {
            auto* state = native_.get();
            const LONG saved = InterlockedCompareExchange(&state->overviewSavedLevel, 0, 0);
            return saved > 0 ? saved / 10000.0 : native_.nativeLevel();
        }
        return read().value_or(pinch::View{}).zoom;
    }
    double rendered() const { return read().value_or(pinch::View{}).zoom; }
    unsigned revision() const { return revision_; }
    bool resetToNormal() {
        if (nativeActive()) {
            InterlockedExchange(&native_.get()->pendingCommand, 3);
            native_.syncNative();
            return true;
        }
        if (nativeRouting_) return false; // Never replace an unavailable native backend with a second magnifier.
        if (!initialized_ || !set_(1.0f, 0, 0)) return false;
        last_ = {};
        original_ = {};
        target_ = 1;
        owned_ = false;
        pinching_ = false;
        ++revision_;
        return true;
    }
    bool target(double value, bool began = false, bridge::InputSource source = bridge::InputSource::Pinch) {
        if (nativeActive()) { (void)began; return native_.requestLevel(value, keyboardRevision_, source); }
        if (nativeRouting_) return false;
        if (!initialized_) {
            return false;
        }
        const auto actual = read();
        if (!actual || !std::isfinite(actual->zoom) || actual->zoom < 1) return false;
        if (!owned_) {
            original_ = {};
            last_ = *actual;
            owned_ = true;
        } else if (!same(*actual, last_)) {
            last_ = *actual;
            target_ = actual->zoom;
            pinching_ = false;
            ++revision_;
            if (!began) return true; // Rebase before applying another finger delta.
        }
        if (began || !pinching_) {
            POINT cursor{};
            if (!getCursor_(&cursor)) return false;
            anchor_ = pinch::anchorAt(last_, {static_cast<double>(cursor.x), static_cast<double>(cursor.y)});
            lastCursor_ = cursor;
            pinching_ = true;
        }
        target_ = value;
        return true;
    }
    void end() {
        pinching_ = false;
        getCursor_(&lastCursor_);
    }
    bool tick(double elapsed, double smoothing, int follow = 0, double margin = 0.12) {
        if (nativeActive()) {
            const LONG sequence = InterlockedCompareExchange(&native_.get()->zoomRevision, 0, 0);
            if (keyboardRevision_ != sequence) { keyboardRevision_ = sequence; ++revision_; }
            return true; // Windows owns rendering, cursor and focus tracking.
        }
        if (nativeRouting_) return true;
        if (!owned_) {
            return true;
        }
        const auto actual = read();
        if (!actual) return false;
        if (!same(*actual, last_)) {
            last_ = *actual;
            target_ = actual->zoom;
            pinching_ = false;
            POINT cursor{};
            if (getCursor_(&cursor)) {
                anchor_ = pinch::anchorAt(last_, {static_cast<double>(cursor.x), static_cast<double>(cursor.y)});
                lastCursor_ = cursor;
            }
            ++revision_;
            return true;
        }
        POINT cursor{};
        if (!getCursor_(&cursor)) {
            return true;
        }
        const double level = pinch::smooth(last_.zoom, target_, elapsed, smoothing);
        if (pinching_ && (cursor.x != lastCursor_.x || cursor.y != lastCursor_.y))
            anchor_ = pinch::anchorAt(last_, {static_cast<double>(cursor.x), static_cast<double>(cursor.y)});
        if (!pinching_ && follow != 2 && (cursor.x != lastCursor_.x || cursor.y != lastCursor_.y)) {
            const auto bounds = desktop();
            const auto source = pinch::Point{static_cast<double>(cursor.x), static_cast<double>(cursor.y)};
            const auto currentAnchor = pinch::anchorAt(last_, source);
            // Push the viewport only when the pointer approaches an edge.
            margin = std::clamp(margin, 0.0, 0.4);
            anchor_ = follow == 1 ? pinch::Anchor{source, source} : pinch::Anchor{source,
                {std::clamp(currentAnchor.display.x, bounds.left + bounds.width * margin,
                            bounds.left + bounds.width * (1 - margin)),
                 std::clamp(currentAnchor.display.y, bounds.top + bounds.height * margin,
                            bounds.top + bounds.height * (1 - margin))}};
        }
        lastCursor_ = cursor;
        const auto next = pinch::anchoredView(level, anchor_, desktop());
        if (same(next, last_)) {
            return true;
        }
        if (!set_(static_cast<float>(next.zoom), next.x, next.y)) {
            log(L"MagSetFullscreenTransform failed; restoring the view.");
            restore();
            return false;
        }
        last_ = next;
        return true;
    }
    void restore() {
        if (owned_) {
            const auto actual = read();
            if (actual && same(*actual, last_) &&
                !set_(static_cast<float>(original_.zoom), original_.x, original_.y)) {
                log(L"Could not restore zoom. Use Windows Magnifier to return to 100%.");
            }
        }
        owned_ = false;
        pinching_ = false;
        target_ = 1;
    }
    ~Zoom() {
        restore();
        if (initialized_ && uninitialize_) {
            uninitialize_();
        }
        if (module_) FreeLibrary(module_);
    }
};

struct Statistics {
    std::uint64_t rawPackets = 0;
    std::uint64_t hidReports = 0;
    std::uint64_t countErrors = 0;
    std::uint64_t reports = 0;
    std::uint64_t frames = 0;
    std::uint64_t pinches = 0;
    std::uint64_t swipes = 0;
    unsigned devices = 0;
};

class WindowSwitcher {
    pinch::WindowCycle cycle_;
    std::mutex mutex_;
    static BOOL CALLBACK collect(HWND window, LPARAM parameter) {
        if (!IsWindowVisible(window) || !IsWindowEnabled(window)) return TRUE;
        const LONG_PTR style = GetWindowLongPtrW(window, GWL_EXSTYLE);
        if (style & (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE)) return TRUE;
        if (window == GetShellWindow() || window == GetDesktopWindow()) return TRUE;
        wchar_t name[96]{};
        GetClassNameW(window, name, ARRAYSIZE(name));
        for (const auto* excluded : {L"Shell_TrayWnd", L"Shell_SecondaryTrayWnd", L"Progman", L"WorkerW", L"XamlExplorerHostIslandWindow"})
            if (wcscmp(name, excluded) == 0) return TRUE;
        DWORD cloaked = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked) return TRUE;
        if (!(style & WS_EX_APPWINDOW)) {
            HWND walk = GetAncestor(window, GA_ROOTOWNER);
            for (unsigned i = 0; walk && i < 32; ++i) {
                HWND popup = GetLastActivePopup(walk);
                if (!popup || popup == walk) break;
                walk = popup;
                if (IsWindowVisible(walk)) break;
            }
            if (walk && walk != window) return TRUE;
        }
        auto& windows = *reinterpret_cast<std::vector<std::uintptr_t>*>(parameter);
        windows.push_back(reinterpret_cast<std::uintptr_t>(window));
        return TRUE;
    }
public:
    static bool eligible(HWND window) {
        if (!window || !IsWindow(window)) return false;
        std::vector<std::uintptr_t> found; collect(window, reinterpret_cast<LPARAM>(&found));
        return !found.empty();
    }
    bool activate(int direction) {
        std::vector<std::uintptr_t> windows;
        EnumWindows(collect, reinterpret_cast<LPARAM>(&windows));
        HWND current = GetAncestor(GetForegroundWindow(), GA_ROOT);
        HWND target = nullptr;
        {
            std::lock_guard lock(mutex_);
            target = reinterpret_cast<HWND>(cycle_.next(windows, reinterpret_cast<std::uintptr_t>(current), direction < 0 ? 1 : -1));
        }
        if (!target || target == current || !IsWindow(target)) return false;
        if (IsIconic(target)) ShowWindowAsync(target, SW_RESTORE);
        if (!SetForegroundWindow(target)) return false;
        return GetAncestor(GetForegroundWindow(), GA_ROOT) == target;
    }
};
inline WindowSwitcher switcher;
inline bool activateWindow(int direction) { return switcher.activate(direction); }

// Hooks run on their own message loop: a cold ShellExecute on the host cannot
// time out a global hook. All keyboard events are observed and passed through.
class MouseInputBroker {
#ifdef PINCH_ZOOM_RUNTIME_TEST
    friend struct InputBrokerTestAccess;
    inline static decltype(&GetAsyncKeyState) apiReadKey = GetAsyncKeyState;
    inline static decltype(&SendInput) apiSendInput = SendInput;
    inline static decltype(&PostMessageW) apiPost = PostMessageW;
    inline static decltype(&CallNextHookEx) apiNext = CallNextHookEx;
#else
    inline static const auto apiReadKey = GetAsyncKeyState;
    inline static const auto apiSendInput = SendInput;
    inline static const auto apiPost = PostMessageW;
    inline static const auto apiNext = CallNextHookEx;
#endif

    HANDLE thread_ = nullptr, ready_ = nullptr;
    DWORD threadId_ = 0;
    HMODULE module_ = nullptr;
    HWND target_ = nullptr;
    bridge::Record* state_ = nullptr;
    std::atomic<unsigned> options_{0};
    std::atomic<unsigned> resync_{0}; // Owner thread applies key/button resets before the next physical input.
    std::atomic<bool> installed_{false};
    inline static thread_local MouseInputBroker* owner_ = nullptr;
    pinch::WinMenuMask menu_;
    pinch::MouseButtonOwnership buttons_;
    static constexpr ULONG_PTR kMaskTag = 0x424d3037;
    void consumeResync() {
        const unsigned requested = resync_.exchange(0, std::memory_order_acq_rel);
        if (!requested) return;
        menu_.reset();
        // Generic modifier aliases would remain down after an L/R-specific UP.
        // The neutral injected mask is never a physical shortcut modifier.
        for (unsigned vk = 8; vk < 256; ++vk) {
            if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU || vk == 0xe8) continue;
            if (apiReadKey(static_cast<int>(vk)) & 0x8000) menu_.key(vk, true);
        }
        if (requested & 2u) buttons_.reset();
    }
    bool maskStart() {
        if (!menu_.consume()) return true;
        INPUT input[2]{};
        for (auto& item : input) {
            item.type = INPUT_KEYBOARD;
            item.ki.wVk = 0xE8; // Unassigned neutral menu mask; no modifier is synthesized.
            item.ki.dwExtraInfo = kMaskTag;
        }
        input[1].ki.dwFlags = KEYEVENTF_KEYUP;
        const UINT sent = apiSendInput(2, input, sizeof(INPUT));
        if (sent == 2) return true;
        if (sent == 1) apiSendInput(1, &input[1], sizeof(INPUT));
        menu_.maskFailed();
        return false; // UIPI can reject input: leave this event to Windows.
    }
    static LRESULT CALLBACK keyboard(int code, WPARAM message, LPARAM data) {
        if (code == HC_ACTION && owner_) {
            const auto* event = reinterpret_cast<const KBDLLHOOKSTRUCT*>(data);
            if (!(event->flags & LLKHF_INJECTED)) owner_->consumeResync();
            owner_->menu_.key(event->vkCode, message == WM_KEYDOWN || message == WM_SYSKEYDOWN,
                             (event->flags & LLKHF_INJECTED) != 0);
        }
        return apiNext(nullptr, code, message, data);
    }
    static LRESULT CALLBACK mouse(int code, WPARAM message, LPARAM data) {
        auto* broker = owner_;
        if (code != HC_ACTION || !broker) return apiNext(nullptr, code, message, data);
        const auto* event = reinterpret_cast<const MSLLHOOKSTRUCT*>(data);
        if (event->flags & LLMHF_INJECTED) return apiNext(nullptr, code, message, data);
        broker->consumeResync();
        const auto forward = [&]() -> LRESULT {
            if (message == WM_MBUTTONDOWN) broker->buttons_.press(1, false);
            if (message == WM_RBUTTONDOWN) broker->buttons_.press(2, false);
            return apiNext(nullptr, code, message, data);
        };
        // A captured DOWN owns its matching UP, even after the modifier is released.
        if ((message == WM_MBUTTONUP && broker->buttons_.release(1)) ||
            (message == WM_RBUTTONUP && broker->buttons_.release(2))) return 1;
        const unsigned button = message == WM_MBUTTONDOWN ? 1 : message == WM_RBUTTONDOWN ? 2 : 3;
        if (button < 3 && broker->buttons_.pressed(button)) return broker->buttons_.owned(button) ? 1 : forward();
        if (message != WM_MOUSEWHEEL && message != WM_MBUTTONDOWN && message != WM_RBUTTONDOWN) return forward();
        const unsigned flags = broker->options_.load();
        if (!(flags & 1)) return forward();
        const bool ctrl = (apiReadKey(VK_CONTROL) & 0x8000) != 0;
        const bool alt = (apiReadKey(VK_MENU) & 0x8000) != 0;
        const bool shift = (apiReadKey(VK_SHIFT) & 0x8000) != 0;
        const bool win = broker->menu_.winDown();
        const bool winZoom = (flags & 2) && win && !ctrl && !alt && !shift && broker->menu_.winOnly();
        const bool ctrlAlt = ctrl && alt && !win && !shift;
        const bool otherMode = broker->state_->nativeViewMode == 1 || broker->state_->nativeViewMode == 3;
        const bool zoomUsable = !otherMode && compatibility::mouseZoomAllowed(
            static_cast<std::uint32_t>(broker->state_->nativeCaps), broker->state_->resolverReasonNative, broker->state_->nativeError);
        const bool zoom = zoomUsable && (flags & 8) && ((flags & 2) ? winZoom : ctrlAlt);
        if (message == WM_MOUSEWHEEL) {
            const int delta = static_cast<SHORT>(HIWORD(event->mouseData));
            if (delta && zoom && (!(flags & 2) || broker->maskStart())) {
                bridge::enqueueWheel(broker->state_, delta,
                    InterlockedCompareExchange(&broker->state_->inputRevision, 0, 0), GetTickCount64());
                InterlockedIncrement(&broker->state_->wheelEvents);
                return 1;
            }
            if (delta && ctrlAlt && (flags & 2) && (flags & 4) && !otherMode) {
                bridge::enqueueWindowWheel(broker->state_, delta, GetTickCount64());
                return 1;
            }
        }
        if (zoomUsable && message == WM_MBUTTONDOWN && ((flags & 2) ? winZoom : ctrlAlt)) {
            if ((!(flags & 2) || broker->maskStart()) && apiPost(broker->target_, kMouseActionMessage, 1, 0)) {
                broker->buttons_.press(1, true); return 1;
            }
        }
        if (ctrlAlt && (flags & 2) && (flags & 16) && (message == WM_MBUTTONDOWN || message == WM_RBUTTONDOWN)) {
            HWND target = GetAncestor(GetForegroundWindow(), GA_ROOT);
            if (WindowSwitcher::eligible(target) && apiPost(broker->target_, kMouseActionMessage,
                    message == WM_MBUTTONDOWN ? 2 : 3, reinterpret_cast<LPARAM>(target))) {
                broker->buttons_.press(message == WM_MBUTTONDOWN ? 1 : 2, true); return 1;
            }
        }
        return forward();
    }
    static DWORD WINAPI run(void* parameter) {
        auto* broker = static_cast<MouseInputBroker*>(parameter);
        owner_ = broker;
        broker->menu_.key(VK_LWIN, (apiReadKey(VK_LWIN) & 0x8000) != 0);
        broker->menu_.key(VK_RWIN, (apiReadKey(VK_RWIN) & 0x8000) != 0);
        MSG message{};
        PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
        HHOOK keys = SetWindowsHookExW(WH_KEYBOARD_LL, keyboard, broker->module_, 0);
        HHOOK pointer = keys ? SetWindowsHookExW(WH_MOUSE_LL, mouse, broker->module_, 0) : nullptr;
        broker->installed_.store(keys && pointer);
        InterlockedExchange(&broker->state_->mouseHooksReady, keys && pointer ? 1 : 0);
        SetEvent(broker->ready_);
        while (keys && pointer && GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message); DispatchMessageW(&message);
        }
        if (pointer) UnhookWindowsHookEx(pointer);
        if (keys) UnhookWindowsHookEx(keys);
        broker->installed_.store(false);
        InterlockedExchange(&broker->state_->mouseHooksReady, 0);
        owner_ = nullptr;
        return 0;
    }
public:
    bool start(HMODULE module, HWND target, bridge::Record* state) {
        module_ = module; target_ = target; state_ = state;
        ready_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!ready_) return false;
        thread_ = CreateThread(nullptr, 0, run, this, 0, &threadId_);
        if (!thread_) { CloseHandle(ready_); ready_ = nullptr; return false; }
        WaitForSingleObject(ready_, INFINITE);
        return installed_.load();
    }
    void configure(const pinch::Config& config, bool interactive) {
        const unsigned next = (interactive && config.enabled && !config.observeOnly && config.keyboardInterop ? 1u : 0u) |
            (config.winWheel ? 2u : 0u) | (config.wheelWindowSwitch ? 4u : 0u) |
            (config.smoothWheel ? 8u : 0u) | (config.windowButtons ? 16u : 0u);
        const unsigned previous = options_.exchange(next);
        if ((next & 1u) && !(previous & 1u)) requestResync();
    }
    void requestResync(bool clearButtons = false) {
        resync_.fetch_or(clearButtons ? 3u : 1u, std::memory_order_release);
    }
    void stop() {
        options_.store(0);
        if (thread_) {
            PostThreadMessageW(threadId_, WM_QUIT, 0, 0);
            WaitForSingleObject(thread_, INFINITE); // No callback may outlive the mod DLL.
            CloseHandle(thread_); thread_ = nullptr;
        }
        if (ready_) { CloseHandle(ready_); ready_ = nullptr; }
    }
    ~MouseInputBroker() { stop(); }
};

class Host {
#ifdef PINCH_ZOOM_RUNTIME_TEST
    friend struct HostTestAccess;
#endif
    pinch::Config config_;
    std::map<HANDLE, Device> devices_;
    Zoom zoom_;
    HWND window_ = nullptr;
    HANDLE activeDevice_ = nullptr;
    bool paused_ = false;
    bool sessionActive_ = false;
    pinch::InteractiveSession activity_;
    std::uint64_t lastSessionRefresh_ = 0;
    bool sessionSnapshot_ = false;
    decltype(&WTSQuerySessionInformationW) querySession_ = WTSQuerySessionInformationW;
    decltype(&WTSFreeMemory) freeSession_ = WTSFreeMemory;
    bool waitForRelease_ = false;
    bool wtsRegistered_ = false;
    bool inputRegistered_ = false;
    std::uint64_t lastTick_ = 0;
    std::uint64_t lastDiagnostic_ = 0;
    Statistics statistics_;
    bridge::Mapping bridge_;
    bool gateNotice_ = false;
    unsigned observedRevision_ = 0;
    HANDLE nativeProcess_ = nullptr;
    pinch::NativeLaunchPolicy nativeLaunch_;
    pinch::WheelMotion wheel_;
    LONG wheelRevision_ = 0;
    MouseInputBroker mouseInput_;
    pinch::WindowWheel windowWheel_;
    int coldWheelDelta_ = 0;
    std::uint64_t coldWheelDeadline_ = 0;
    LONG coldWheelCommands_ = 0;
    void cancelWheel(bool invalidate = true) {
        wheel_.cancel();
        if (invalidate) coldWheelDelta_ = 0;
        if (auto* state = bridge_.get()) {
            InterlockedExchange(&state->wheelActive, 0);
            if (invalidate) {
                InterlockedIncrement(&state->inputRevision);
                InterlockedExchange64(&state->wheelPacket, 0);
                for (auto& [id, dev] : devices_) { (void)id; dev.coldPinchSeed.reset(); }
            } // A new generation already published by native keys keeps its own pending input.
        }
    }
    void wheelInput(int delta, LONG revision, std::uint64_t now) {
        auto* state = bridge_.get();
        if (!delta || !state || paused_ || !sessionActive_ || !config_.enabled || config_.observeOnly ||
            !config_.keyboardInterop || !config_.smoothWheel ||
            (state->down && state->phase == static_cast<LONG>(pinch::Phase::Pinch))) return;
        if (revision == -1 && bridge_.magnifierAvailable()) return; // Legacy wake-only messages never become zoom.
        if (revision != -1 && revision != state->inputRevision) return;
        if (!bridge_.magnifierAvailable()) {
            ensureNativeMagnifier(now, true);
            // Preserve at most one notch while the native UI starts, never a
            // free-spin queue. A close/session change discards this intent.
            if (now > coldWheelDeadline_) coldWheelDelta_ = 0;
            const bool same = (coldWheelDelta_ < 0) == (delta < 0);
            coldWheelDelta_ = std::clamp(delta + (same ? coldWheelDelta_ : 0), -120, 120);
            coldWheelDeadline_ = now + 1500;
            coldWheelCommands_ = state->nativeCommands;
            return;
        }
        if (wheel_.active() && wheelRevision_ != state->inputRevision) cancelWheel(false);
        wheelRevision_ = state->inputRevision;
        wheel_.add(delta, zoom_.current(), now, config_.wheelStep, config_.wheelDuration,
                   std::min(config_.maxZoom, 16.0), config_.wheelMaxRate);
        InterlockedExchange(&state->wheelActive, wheel_.active() ? 1 : 0);
    }

    bool contactsDown() const {
        return std::any_of(devices_.begin(), devices_.end(), [](const auto& entry) { return entry.second.activeContacts != 0; });
    }
    void restartGestureTracking() {
        waitForRelease_ = contactsDown();
        resetGestures();
    }
    void applyColdPinch(std::uint64_t now) {
        auto* state = bridge_.get();
        for (auto& [id, dev] : devices_) {
            if (!dev.coldPinchSeed) continue;
            if (!state || paused_ || !sessionActive_ || !config_.enabled || config_.observeOnly || !config_.keyboardInterop ||
                activeDevice_ != id || dev.activeContacts != 3 || dev.recognizer.phase() != pinch::Phase::Pinch ||
                now > dev.coldPinchUntil || dev.coldPinchCommands != state->nativeCommands ||
                state->nativeViewMode == 1 || state->nativeViewMode == 3 ||
                compatibility::nativeDeclined(static_cast<std::uint32_t>(state->nativeCaps), state->resolverReasonNative)) {
                dev.coldPinchSeed.reset();
                continue;
            }
            if (!bridge_.magnifierAvailable() || !bridge_.gateAvailable()) continue;
            const double seed = *dev.coldPinchSeed;
            dev.coldPinchSeed.reset(); // Deliver only once; no elapsed travel or repeated setter queue.
            if (zoom_.target(seed, true, bridge::InputSource::Pinch)) {
                dev.recognizer.adoptZoom(seed);
                dev.waitingForNative = false;
            }
        }
    }
    void render(std::uint64_t now, bool deliverColdPinch = true) {
        if (!zoom_.tick(static_cast<double>(now - lastTick_), config_.smoothingMs,
                        config_.mouseFollow, config_.edgeMargin)) restartGestureTracking();
        if (observedRevision_ != zoom_.revision()) {
            observedRevision_ = zoom_.revision();
            const double level = zoom_.current();
            for (auto& [id, dev] : devices_) { (void)id; dev.recognizer.adoptZoom(level); }
        }
        if (auto* state = bridge_.get()) {
            const LONG epoch = InterlockedCompareExchange(&state->inputRevision, 0, 0);
            if (wheel_.active() && (epoch != wheelRevision_ || paused_ || !sessionActive_ ||
                !config_.smoothWheel || !bridge_.magnifierAvailable())) cancelWheel(epoch == wheelRevision_);
            const auto packet = static_cast<std::uint64_t>(InterlockedExchange64(&state->wheelPacket, 0));
            const auto captured = static_cast<std::uint64_t>(InterlockedCompareExchange64(&state->wheelTime, 0, 0));
            if (pinch::wheelDelta(packet) && (captured >= now || now - captured <= 200))
                wheelInput(pinch::wheelDelta(packet), static_cast<LONG>(pinch::wheelRevision(packet)), now);
            if (const auto level = wheel_.tick(now)) zoom_.target(*level, true, bridge::InputSource::Wheel);
            InterlockedExchange(&state->wheelActive, wheel_.active() ? 1 : 0);
            if (coldWheelDelta_) {
                if (now > coldWheelDeadline_ || paused_ || !sessionActive_ || coldWheelCommands_ != state->nativeCommands) coldWheelDelta_ = 0;
                else if (bridge_.magnifierAvailable()) {
                    const int delta = std::exchange(coldWheelDelta_, 0);
                    wheelInput(delta, state->inputRevision, now);
                }
            }
            const auto windowPacket = static_cast<std::uint64_t>(InterlockedExchange64(&state->windowWheelPacket, 0));
            const auto windowTime = static_cast<std::uint64_t>(InterlockedCompareExchange64(&state->windowWheelTime, 0, 0));
            if (!paused_ && sessionActive_ && config_.winWheel && config_.wheelWindowSwitch && config_.enabled && !config_.observeOnly &&
                (windowTime >= now || now - windowTime <= 200)) {
                if (const auto direction = windowWheel_.feed(pinch::wheelDelta(windowPacket), now)) activateWindow(-*direction);
            } else windowWheel_.reset();
            mouseInput_.configure(config_, sessionActive_ && !paused_);
            if (deliverColdPinch) applyColdPinch(now);

        }
        lastTick_ = now;
    }
    pinch::Config interactiveConfig() const {
        auto result = config_;
        result.enabled = result.enabled && sessionActive_;
        result.smoothWheel = result.smoothWheel && !paused_;
        result.wheelWindowSwitch = result.wheelWindowSwitch && !paused_;
        result.normalZoomInOverview = result.normalZoomInOverview && !paused_;
        result.disableReading = result.disableReading && !paused_;
        if (paused_) result.keyboardHoldMultiplier = 1;
        return result;
    }
    void applySessionState() {
        const bool active = activity_.active();
        if (active == sessionActive_) return;
        sessionActive_ = active;
        mouseInput_.requestResync(true);
        mouseInput_.configure(config_, sessionActive_ && !paused_);
        if (!active) {
            coldWheelDelta_ = 0; windowWheel_.reset();
            zoom_.restore();
            restartGestureTracking(); // Wait for release only if contacts were down.
            if (auto* state = bridge_.get()) {
                InterlockedExchange(&state->appliedSequence, state->requestedSequence);
                InterlockedExchange(&state->pendingCommand, 0);
                InterlockedIncrement(&state->zoomRevision); // Reject old queued input.
            }
        } else {
            // If a release was observed while inactive, do not discard the first
            // new three-finger gesture after unlocking/resuming.
            waitForRelease_ = waitForRelease_ && contactsDown();
            nativeLaunch_.resetAttempts();
        }
        bridge_.configure(interactiveConfig());
        bridge_.publish(pinch::Phase::Idle, false, false);
    }
    void refreshSession(std::uint64_t now, bool force = false) {
        if (!force && ((wtsRegistered_ && sessionSnapshot_) || now - lastSessionRefresh_ < 1000)) return;
        lastSessionRefresh_ = now;
        sessionSnapshot_ = false;
        if (force) activity_.snapshot(false, false);
        // RDS may not be ready at Explorer startup. Retry without blocking the
        // UI thread; a failed first registration must not last all session.
        if (!wtsRegistered_) wtsRegistered_ = WTSRegisterSessionNotification(window_, NOTIFY_FOR_THIS_SESSION) != FALSE;
        LPWSTR data = nullptr;
        DWORD bytes = 0;
        if (querySession_(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION,
                                        WTSSessionInfoEx, &data, &bytes)) {
            if (data && bytes >= sizeof(WTSINFOEXW)) {
                const auto* information = reinterpret_cast<const WTSINFOEXW*>(data);
                if (information->Level == 1) {
                    const auto& session = information->Data.WTSInfoExLevel1;
                    sessionSnapshot_ = session.SessionFlags == WTS_SESSIONSTATE_LOCK || session.SessionFlags == WTS_SESSIONSTATE_UNLOCK;
                    activity_.snapshot(session.SessionState == WTSActive,
                                       session.SessionFlags == WTS_SESSIONSTATE_UNLOCK);
                }
            }
            if (data) freeSession_(data);
        }
        applySessionState();
    }
    void ensureNativeMagnifier(std::uint64_t now, bool freshPinch = false) {
        if (!config_.enabled || config_.observeOnly || !config_.keyboardInterop || !sessionActive_ || paused_) return;
        const bool ready = bridge_.magnifierAvailable();
        bool ownAlive = false, cleanExit = false;
        if (nativeProcess_) {
            const DWORD wait = WaitForSingleObject(nativeProcess_, 0);
            ownAlive = wait == WAIT_TIMEOUT;
            if (!ownAlive) {
                DWORD code = 0;
                cleanExit = wait == WAIT_OBJECT_0 && GetExitCodeProcess(nativeProcess_, &code) && code == 0;
                CloseHandle(nativeProcess_); nativeProcess_ = nullptr;
            }
        }
        const bool alive = ready || bridge_.magnifierProcessAlive() || ownAlive;
        const bool exited = nativeLaunch_.observe(ready, alive, cleanExit);
        // Observe the exit BEFORE accepting a fresh request. Otherwise a pinch
        // starting just after Win+Esc gets overwritten by close detection.
        if (exited) {
            coldWheelDelta_ = 0;
            zoom_.resetToNormal();
            if (!freshPinch) restartGestureTracking();
        }
        if (freshPinch) nativeLaunch_.request();
        if (ready || alive || !nativeLaunch_.mayLaunch(now)) return;
        if (nativeProcess_) { CloseHandle(nativeProcess_); nativeProcess_ = nullptr; }
        wchar_t system[MAX_PATH];
        const UINT length = GetSystemDirectoryW(system, ARRAYSIZE(system));
        if (!length || length + 13 >= ARRAYSIZE(system)) return;
        std::wstring path(system);
        path += L"\\Magnify.exe";
        SHELLEXECUTEINFOW launch{};
        launch.cbSize = sizeof(launch);
        launch.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
        launch.lpFile = path.c_str();
        launch.nShow = config_.hideMagnifier ? SW_HIDE : SW_SHOWNOACTIVATE;
        nativeLaunch_.attempted(now);
        if (bridge_.get()) InterlockedExchange(&bridge_.get()->startAtNormal, 1);
        if (shell_feedback::execute(launch, config_.suppressStartupFeedback)) nativeProcess_ = launch.hProcess;
        else {
            if (bridge_.get()) InterlockedExchange(&bridge_.get()->startAtNormal, 0);
            log(L"Could not start Windows Magnifier; fresh input can retry after the launch cooldown.");
        }
    }

    void resetGestures() {
        cancelWheel();
        bridge_.publish(pinch::Phase::Idle, false, false);
        activeDevice_ = nullptr;
        for (auto& [id, dev] : devices_) {
            (void)id;
            dev.recognizer.reset();
            dev.assembler.reset();
            dev.waitingForNative = false;
            dev.coldPinchSeed.reset();
            dev.pendingPinchRadius = 0; dev.pendingPinchRatio = 1;
        }
    }
    bool registerTouchpadInput() {
        UINT registrations = 0;
        if (GetRegisteredRawInputDevices(nullptr, &registrations, sizeof(RAWINPUTDEVICE)) == UINT(-1)) return false;
        if (registrations) {
            std::vector<RAWINPUTDEVICE> existing(registrations);
            if (GetRegisteredRawInputDevices(existing.data(), &registrations, sizeof(RAWINPUTDEVICE)) == UINT(-1)) return false;
            for (const auto& item : existing) if (item.usUsagePage == kDigitizer && item.usUsage == kTouchpad) return false;
        }
        RAWINPUTDEVICE registration{kDigitizer, kTouchpad, RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, window_};
        inputRegistered_ = RegisterRawInputDevices(&registration, 1, sizeof(registration)) != FALSE;
        return inputRegistered_;
    }
    Device* device(HANDLE handle) {
        const auto found = devices_.find(handle);
        if (found != devices_.end()) {
            return found->second.valid ? &found->second : nullptr;
        }
        auto& dev = devices_[handle];
        UINT size = 0;
        if (GetRawInputDeviceInfoW(handle, RIDI_PREPARSEDDATA, nullptr, &size) != 0 || !size) {
            return nullptr;
        }
        dev.preparsed.resize(size);
        if (GetRawInputDeviceInfoW(handle, RIDI_PREPARSEDDATA, dev.preparsed.data(), &size) == UINT(-1)) {
            return nullptr;
        }
        auto data = reinterpret_cast<PHIDP_PREPARSED_DATA>(dev.preparsed.data());
        HIDP_CAPS caps{};
        if (HidP_GetCaps(data, &caps) != HIDP_STATUS_SUCCESS ||
            caps.UsagePage != kDigitizer || caps.Usage != kTouchpad ||
            caps.NumberInputValueCaps == 0) {
            return nullptr;
        }
        USHORT count = caps.NumberInputValueCaps;
        std::vector<HIDP_VALUE_CAPS> values(count);
        if (HidP_GetValueCaps(HidP_Input, values.data(), &count, data) != HIDP_STATUS_SUCCESS) {
            return nullptr;
        }
        USHORT buttonCount = caps.NumberInputButtonCaps;
        std::vector<HIDP_BUTTON_CAPS> buttons(buttonCount);
        if (buttonCount && HidP_GetButtonCaps(HidP_Input, buttons.data(), &buttonCount, data) != HIDP_STATUS_SUCCESS) {
            return nullptr;
        }
        for (const auto& cap : values) {
            if (cap.UsagePage == kDigitizer && hasUsage(cap, kContactCount)) {
                dev.hasCount = true;
                dev.countCollection = cap.LinkCollection;
            }
            if (cap.UsagePage == kDigitizer && hasUsage(cap, kScanTime)) {
                dev.hasScan = true;
                dev.scanCollection = cap.LinkCollection;
            }
            if (cap.UsagePage != 1 || !hasUsage(cap, 0x30)) {
                continue;
            }
            for (const auto& y : values) {
                if (y.UsagePage != 1 || !hasUsage(y, 0x31) ||
                    y.LinkCollection != cap.LinkCollection || y.ReportID != cap.ReportID) {
                    continue;
                }
                Slot slot{cap.LinkCollection, cap.ReportID, cap, y};
                for (const auto& field : values) {
                    if (field.UsagePage == kDigitizer && field.LinkCollection == cap.LinkCollection &&
                        field.ReportID == cap.ReportID && hasUsage(field, kContactId)) {
                        slot.hasId = true;
                    }
                }
                for (const auto& field : buttons) {
                    if (field.UsagePage == kDigitizer && field.LinkCollection == cap.LinkCollection &&
                        field.ReportID == cap.ReportID && hasUsage(field, kTip)) {
                        slot.hasTip = true;
                    }
                    if (field.UsagePage == kDigitizer && field.LinkCollection == cap.LinkCollection &&
                        field.ReportID == cap.ReportID && hasUsage(field, kConfidence)) {
                        slot.hasConfidence = true;
                    }
                }
                if (slot.hasId && slot.hasTip && logicalMaximum(cap) > logicalMinimum(cap) &&
                    logicalMaximum(y) > logicalMinimum(y)) {
                    dev.slots.push_back(slot);
                }
                break;
            }
        }
        dev.valid = dev.hasCount && !dev.slots.empty() && dev.slots.size() <= pinch::kMaxContacts;
        if (dev.valid) {
            ++statistics_.devices;
            log(L"Compatible Precision Touchpad HID descriptor found.");
        }
        return dev.valid ? &dev : nullptr;
    }
    void frame(HANDLE handle, Device& dev, const std::vector<pinch::Contact>& contacts, ULONGLONG now, double elapsedMs) {
        ++statistics_.frames;
        dev.activeContacts = static_cast<unsigned>(std::count_if(contacts.begin(), contacts.end(), [](const auto& c) { return c.down; }));
        if (dev.activeContacts != 3) {
            dev.coldPinchSeed.reset(); dev.waitingForNative = false;
            dev.pendingPinchRadius = 0; dev.pendingPinchRatio = 1;
        }
        double radius = 0;
        if (dev.activeContacts == 3) {
            pinch::Point center{};
            for (const auto& contact : contacts) if (contact.down) {
                center.x += contact.point.x / 3; center.y += contact.point.y / 3;
            }
            for (const auto& contact : contacts) if (contact.down)
                radius += std::pow(contact.point.x - center.x, 2) + std::pow(contact.point.y - center.y, 2);
            radius = std::sqrt(radius / 3);
        }
        if (paused_ || !sessionActive_ || !config_.enabled || !config_.keyboardInterop) {
            bridge_.publish(pinch::Phase::Idle, false, false);
            return;
        }
        if (waitForRelease_) {
            if (dev.activeContacts < 3) {
                waitForRelease_ = false;
            }
            return;
        }
        render(now, false); // Validate this HID frame before a pending cold intent can be delivered.
        const auto before = dev.recognizer.phase();
        if (dev.waitingForNative) {
            dev.recognizer.adoptZoom(zoom_.current());
            dev.waitingForNative = !bridge_.magnifierAvailable();
        }
        auto effective = config_;
        if (config_.keyboardInterop) effective.maxZoom = std::min(effective.maxZoom, 16.0);
        const auto decision = dev.recognizer.update(contacts, now, zoom_.current(), effective, elapsedMs);
        if (decision.phase == pinch::Phase::Pending) {
            if (before == pinch::Phase::Pending && dev.pendingPinchRadius > 0 && radius > 0) {
                const double ratio = radius / dev.pendingPinchRadius;
                if (std::isfinite(ratio) && std::abs(std::log(ratio)) > 0.00000001) dev.pendingPinchRatio = ratio;
            } else dev.pendingPinchRatio = 1;
            dev.pendingPinchRadius = radius;
        } else if (decision.phase != pinch::Phase::Pinch) dev.coldPinchSeed.reset();
        auto phase = decision.phase == pinch::Phase::Rejected ||
                           (decision.phase == pinch::Phase::Idle && dev.activeContacts == 3)
                               ? pinch::Phase::Swipe : decision.phase;
        if (const auto* record = bridge_.get(); record && phase == pinch::Phase::Pinch &&
            compatibility::nativeDeclined(static_cast<std::uint32_t>(record->nativeCaps), record->resolverReasonNative)) phase = pinch::Phase::Swipe;
        if (auto* record = bridge_.get()) InterlockedExchange(&record->swipeDirection,
            phase == pinch::Phase::Swipe ? pinch::swipeDirection(dev.recognizer.movement()) : 0);
        bridge_.publish(phase, dev.activeContacts == 3, !config_.observeOnly);
        if (before == pinch::Phase::Pinch && decision.phase != pinch::Phase::Pinch) {
            zoom_.end();
            bridge_.syncNative();
        }
        if (decision.phase == pinch::Phase::Idle && activeDevice_ == handle) {
            activeDevice_ = nullptr;
        }
        if (decision.phase == pinch::Phase::Swipe && before != decision.phase) {
            ++statistics_.swipes;
            if (config_.debug) {
                log(L"Swipe: left to Windows, no zoom.");
            }
        }
        if (decision.began) {
            cancelWheel();
            if (!config_.observeOnly && config_.keyboardInterop && phase == pinch::Phase::Pinch && !bridge_.magnifierAvailable()) {
                double ratio = dev.pendingPinchRadius > 0 && radius > 0 ? radius / dev.pendingPinchRadius : 1;
                if (!std::isfinite(ratio) || std::abs(std::log(ratio)) < 0.00000001) ratio = dev.pendingPinchRatio;
                if (std::isfinite(ratio) && ratio > 0) {
                    const double change = std::clamp(std::log(ratio) * config_.sensitivity * (config_.invertPinch ? -1 : 1),
                                                     -std::log(1.03), std::log(1.03));
                    const double base = zoom_.current();
                    const double seed = std::clamp(base * std::exp(change), 1.0, std::min(config_.maxZoom, 16.0));
                    if (std::isfinite(seed) && std::abs(seed - base) > 0.00000001) {
                        dev.coldPinchSeed = seed;
                        dev.coldPinchUntil = now + 1500;
                        if (auto* state = bridge_.get()) dev.coldPinchCommands = state->nativeCommands;
                    }
                }
            }
            ++statistics_.pinches;
            if (!config_.observeOnly) ensureNativeMagnifier(now, true);
            if (config_.debug) {
                log(L"Three-finger pinch recognized.");
            }
        }
        if (decision.zoom && (!activeDevice_ || activeDevice_ == handle)) {
            activeDevice_ = handle;
            if (dev.coldPinchSeed && bridge_.magnifierAvailable() && bridge_.gateAvailable()) {
                applyColdPinch(now);
                return; // This frame was still received while waiting: discard its unrendered travel.
            }
            if (!config_.observeOnly && (!bridge_.gateAvailable() ||
                (config_.keyboardInterop && !bridge_.magnifierAvailable()))) {
                if (!gateNotice_) log(L"Native zoom integration is not ready; waiting for the companion before applying pinch zoom.");
                gateNotice_ = true;
                dev.recognizer.adoptZoom(zoom_.current());
                dev.waitingForNative = true;
                return;
            }
            if (!config_.observeOnly && !zoom_.target(*decision.zoom, decision.began)) {
                dev.recognizer.reset();
                activeDevice_ = nullptr;
            }
            // Apply on the input frame, without waiting for the 16 ms timer.
            if (!config_.observeOnly) render(now);
        }
    }
    void report(HANDLE handle, Device& dev, char* bytes, ULONG length) {
        auto data = reinterpret_cast<PHIDP_PREPARSED_DATA>(dev.preparsed.data());
        auto value = [&](USAGE page, USHORT collection, USAGE usage) -> std::optional<ULONG> {
            ULONG result = 0;
            if (HidP_GetUsageValue(HidP_Input, page, collection, usage, &result,
                                  data, bytes, length) != HIDP_STATUS_SUCCESS) {
                return std::nullopt;
            }
            return result;
        };
        const auto contactCount = value(kDigitizer, dev.countCollection, kContactCount);
        if (!contactCount) {
            ++statistics_.countErrors;
            if (config_.debug && statistics_.countErrors == 1) {
                log(L"HID packets received, but Contact Count could not be decoded.");
            }
            return;
        }
        ++statistics_.reports;
        pinch::Report decoded;
        decoded.count = *contactCount;
        if (dev.hasScan) {
            decoded.scan = value(kDigitizer, dev.scanCollection, kScanTime);
        }
        if (decoded.count && decoded.scan) dev.frameScan = decoded.scan;
        for (const auto& slot : dev.slots) {
            if (length == 0 || (slot.reportId != 0 && static_cast<UCHAR>(bytes[0]) != slot.reportId)) {
                continue;
            }
            const auto x = value(1, slot.collection, 0x30);
            const auto y = value(1, slot.collection, 0x31);
            const auto id = value(kDigitizer, slot.collection, kContactId);
            if (!x || !y || !id) {
                continue;
            }
            std::array<USAGE, 32> usages{};
            ULONG usageCount = static_cast<ULONG>(usages.size());
            if (HidP_GetUsages(HidP_Input, kDigitizer, slot.collection, usages.data(),
                               &usageCount, data, bytes, length) != HIDP_STATUS_SUCCESS) {
                // A malformed report must not become a spurious release.
                dev.assembler.reset();
                dev.recognizer.reset();
                return;
            }
            const auto end = usages.begin() + usageCount;
            const bool tip = std::find(usages.begin(), end, kTip) != end;
            const bool confident = !slot.hasConfidence ||
                                    std::find(usages.begin(), end, kConfidence) != end;
            double width = physicalMillimeters(slot.x);
            double height = physicalMillimeters(slot.y);
            if (width == 0 || height == 0) {
                width = 100;
                height = width * config_.fallbackAspect;
            }
            decoded.contacts.push_back({*id, {coordinate(*x, slot.x, width),
                                              coordinate(*y, slot.y, height)}, tip, confident});
        }
        const auto now = GetTickCount64();
        if (const auto complete = dev.assembler.feed(decoded, now)) {
            LARGE_INTEGER counter{}, frequency{};
            QueryPerformanceCounter(&counter);
            QueryPerformanceFrequency(&frequency);
            const double preciseMs = 1000.0 * static_cast<double>(counter.QuadPart) / frequency.QuadPart;
            const double elapsed = dev.timing.advance(dev.frameScan, preciseMs);
            frame(handle, dev, *complete, now, elapsed);
        }
    }
    void rawInput(HRAWINPUT handle) {
        UINT size = 0;
        if (GetRawInputData(handle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0 ||
            size < offsetof(RAWINPUT, data.hid.bRawData)) {
            return;
        }
        std::vector<BYTE> buffer(size);
        const UINT received = GetRawInputData(handle, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER));
        if (received == UINT(-1) || received < offsetof(RAWINPUT, data.hid.bRawData)) {
            return;
        }
        const auto* raw = reinterpret_cast<const RAWINPUT*>(buffer.data());
        if (raw->header.dwType != RIM_TYPEHID) {
            return;
        }
        ++statistics_.rawPackets;
        const auto& hid = raw->data.hid;
        const auto payload = static_cast<std::size_t>(received) - offsetof(RAWINPUT, data.hid.bRawData);
        if (!hid.dwSizeHid || hid.dwCount > payload / hid.dwSizeHid) {
            return;
        }
        if (auto* dev = device(raw->header.hDevice)) {
            for (DWORD i = 0; i < hid.dwCount; ++i) {
                ++statistics_.hidReports;
                report(raw->header.hDevice, *dev,
                       reinterpret_cast<char*>(const_cast<BYTE*>(hid.bRawData)) + i * hid.dwSizeHid,
                       hid.dwSizeHid);
            }
        }
    }
    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM w, LPARAM l) {
        auto* host = reinterpret_cast<Host*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            host = static_cast<Host*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(host));
        }
        if (!host) {
            return DefWindowProcW(window, message, w, l);
        }
        switch (message) {
            case kMouseActionMessage:
                if (!host->sessionActive_ || host->paused_ || !host->config_.enabled || host->config_.observeOnly) return 0;
                if (w == 1) {
                    host->coldWheelDelta_ = 0;
                    host->cancelWheel(); host->zoom_.resetToNormal(); host->restartGestureTracking();
                } else if (host->config_.windowButtons && host->config_.winWheel && (w == 2 || w == 3)) {
                    HWND target = reinterpret_cast<HWND>(l);
                    if (target == GetAncestor(GetForegroundWindow(), GA_ROOT) && WindowSwitcher::eligible(target))
                        ShowWindowAsync(target, w == 3 ? SW_MINIMIZE : IsZoomed(target) ? SW_RESTORE : SW_MAXIMIZE);
                }
                return 0;
            case kWheelMessage:
                host->wheelInput(static_cast<int>(w), static_cast<LONG>(l), GetTickCount64());
                return 0;
            case WM_INPUT:
                host->rawInput(reinterpret_cast<HRAWINPUT>(l));
                // DefWindowProc is required to clean up foreground WM_INPUT.
                return DefWindowProcW(window, message, w, l);
            case WM_INPUT_DEVICE_CHANGE:
                host->devices_.erase(reinterpret_cast<HANDLE>(l));
                host->resetGestures();
                if (w == GIDC_REMOVAL) host->zoom_.restore();
                return 0;
            case WM_DISPLAYCHANGE:
                host->zoom_.restore();
                host->resetGestures();
                host->waitForRelease_ = true;
                return 0;
            case WM_TIMER: {
                const auto now = GetTickCount64();
                host->refreshSession(now);
                host->bridge_.heartbeat();
                if (bridge::desktopRaised) host->bridge_.shellReason(2, InterlockedCompareExchange(bridge::desktopRaised, 0, 0) != 0);
                host->ensureNativeMagnifier(now);
                host->render(now);
                if (host->config_.debug && now - host->lastDiagnostic_ >= 5000) {
                    host->lastDiagnostic_ = now;
                    wchar_t summary[256];
                    swprintf_s(summary, L"Input: raw=%llu HID=%llu decoded=%llu frames=%llu pinches=%llu swipes=%llu",
                               static_cast<unsigned long long>(host->statistics_.rawPackets),
                               static_cast<unsigned long long>(host->statistics_.hidReports),
                               static_cast<unsigned long long>(host->statistics_.reports),
                               static_cast<unsigned long long>(host->statistics_.frames),
                               static_cast<unsigned long long>(host->statistics_.pinches),
                               static_cast<unsigned long long>(host->statistics_.swipes));
                    log(summary);
                }
                return 0;
            }
            case WM_HOTKEY:
                if (!host->sessionActive_) return 0;
                host->zoom_.resetToNormal();
                host->bridge_.syncNative();
                host->restartGestureTracking();
                host->bridge_.publish(pinch::Phase::Idle, false, false);
                if (w == kPauseHotkey) {
                    host->paused_ = !host->paused_;
                    host->bridge_.configure(host->interactiveConfig());
                    host->mouseInput_.configure(host->config_, host->sessionActive_ && !host->paused_);
                    log(host->paused_ ? L"Paused." : L"Resumed.");
                } else {
                    log(L"Zoom reset.");
                }
                return 0;
            case WM_WTSSESSION_CHANGE:
                if (w == WTS_SESSION_LOCK) host->activity_.lock();
                else if (w == WTS_SESSION_UNLOCK) host->activity_.unlock();
                else if (w == WTS_CONSOLE_DISCONNECT || w == WTS_REMOTE_DISCONNECT || w == WTS_SESSION_LOGOFF)
                    host->activity_.disconnect();
                else if (w == WTS_CONSOLE_CONNECT || w == WTS_REMOTE_CONNECT)
                    host->activity_.connect();
                host->applySessionState();
                if (w == WTS_CONSOLE_CONNECT || w == WTS_REMOTE_CONNECT) host->refreshSession(GetTickCount64(), true);
                return 0;
            case WM_POWERBROADCAST:
                if (w == PBT_APMSUSPEND) {
                    host->activity_.suspend();
                    host->applySessionState();
                } else if (w == PBT_APMRESUMEAUTOMATIC || w == PBT_APMRESUMESUSPEND) {
                    host->activity_.resume();
                    host->applySessionState();
                    host->refreshSession(GetTickCount64(), true);
                }
                return TRUE;
            case kSettingsMessage:
                host->zoom_.end();
                host->restartGestureTracking();
                host->config_ = *reinterpret_cast<const pinch::Config*>(l);
                host->bridge_.configure(host->interactiveConfig());
                host->mouseInput_.configure(host->config_, host->sessionActive_ && !host->paused_);
                host->nativeLaunch_.resetAttempts();
                if (!host->config_.enabled || host->config_.observeOnly) host->zoom_.resetToNormal();
                host->bridge_.syncNative();
                return 0;
            case kStopMessage:
            case WM_CLOSE:
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProcW(window, message, w, l);
    }

public:
    explicit Host(pinch::Config config) : config_(config) {}
    bool initialize(HINSTANCE module) {
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        if (!zoom_.initialize()) {
            log(L"MagInitialize failed.");
            return false;
        }
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = windowProc;
        windowClass.hInstance = module;
        windowClass.lpszClassName = kClassName;
        if (!RegisterClassW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
        // Hidden top-level window also receives session and power broadcasts.
        window_ = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, kClassName, L"Better Magnifier",
                                  WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, module, this);
        if (!window_) {
            return false;
        }
        if (!bridge_.open()) return false;
#ifdef PINCH_ZOOM_STANDALONE
        const bool wantTouchpad = true; // Observation remains usable without the native shell gate.
#else
        const bool wantTouchpad = config_.observeOnly || bridge_.gateAvailable();
#endif
        if (wantTouchpad && !registerTouchpadInput()) log(L"Touchpad input unavailable; mouse wheel and session controller remain active.");
        if (!wantTouchpad) nativeLaunch_.observe(false, false, true); // Do not auto-open a native fallback after an unsupported shell update.
        // Keep an emergency reset available before allowing any zoom changes.
        if (!RegisterHotKey(window_, kResetHotkey, MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, VK_F10)) {
            log(L"Emergency reset hotkey unavailable; startup cancelled.");
            return false;
        }
        if (!RegisterHotKey(window_, kPauseHotkey, MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, VK_F9)) {
            log(L"Pause hotkey unavailable; reset hotkey remains available.");
        }
        refreshSession(GetTickCount64(), true);
        if (!bridge_.open()) return false;
        bridge_.configure(interactiveConfig());
        InterlockedExchange64(&bridge_.get()->hostWindow, reinterpret_cast<LONG64>(window_));
        mouseInput_.configure(config_, sessionActive_ && !paused_);
        if (!mouseInput_.start(module, window_, bridge_.get())) log(L"Mouse shortcut hooks unavailable; native keyboard controls remain available.");
        bridge_.publish(pinch::Phase::Pending, false, sessionActive_ && config_.enabled && !config_.observeOnly);
        lastTick_ = GetTickCount64();
        lastDiagnostic_ = lastTick_;
        if (!SetTimer(window_, kTimer, 16, nullptr)) {
            return false;
        }
        UINT count = 0;
        if (inputRegistered_ && GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) == 0 && count) {
            std::vector<RAWINPUTDEVICELIST> list(count);
            const UINT actual = GetRawInputDeviceList(list.data(), &count, sizeof(RAWINPUTDEVICELIST));
            if (actual != UINT(-1)) {
                for (UINT i = 0; i < actual; ++i) {
                    if (list[i].dwType == RIM_TYPEHID) {
                        device(list[i].hDevice);
                    }
                }
            }
        }
        log(config_.observeOnly ? L"Ready: observation only; no desktop zoom changes." : L"Ready: three-finger pinch zoom.");
        return true;
    }
    HWND window() const { return window_; }
    const Statistics& statistics() const { return statistics_; }
    void loop(HANDLE stop, unsigned seconds = 0) {
        const auto end = seconds ? GetTickCount64() + seconds * 1000ULL : 0;
        bool running = true;
        while (running) {
            const DWORD timeout = end ? static_cast<DWORD>(std::min<ULONGLONG>(1000, end > GetTickCount64() ? end - GetTickCount64() : 0)) : INFINITE;
            const DWORD result = MsgWaitForMultipleObjects(stop ? 1 : 0, stop ? &stop : nullptr,
                                                           FALSE, timeout, QS_ALLINPUT);
            if ((stop && result == WAIT_OBJECT_0) || result == WAIT_FAILED ||
                (end && GetTickCount64() >= end)) {
                break;
            }
            MSG message{};
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT) {
                    running = false;
                    break;
                }
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
    }
    ~Host() {
        mouseInput_.stop();
        cancelWheel();
        if (auto* state = bridge_.get()) InterlockedExchange64(&state->hostWindow, 0);
        bridge_.publish(pinch::Phase::Idle, false, false);
        zoom_.restore();
        if (auto* record = bridge_.get()) InterlockedExchange(&record->controlEnabled, 0);
        if (nativeProcess_) CloseHandle(nativeProcess_);
        if (window_) {
            KillTimer(window_, kTimer);
            UnregisterHotKey(window_, kResetHotkey);
            UnregisterHotKey(window_, kPauseHotkey);
            if (wtsRegistered_) {
                WTSUnRegisterSessionNotification(window_);
            }
            if (inputRegistered_) {
                RAWINPUTDEVICE removal{kDigitizer, kTouchpad, RIDEV_REMOVE, nullptr};
                RegisterRawInputDevices(&removal, 1, sizeof(removal));
            }
            HINSTANCE module = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(window_, GWLP_HINSTANCE));
            DestroyWindow(window_);
            UnregisterClassW(kClassName, module);
        }
    }
};

}  // namespace runtime

#ifdef PINCH_ZOOM_STANDALONE

int wmain(int argc, wchar_t** argv) {
    pinch::Config config;
    config.observeOnly = true;
    config.debug = true;
    unsigned seconds = 5;
    bool smoke = false;
    bool status = false;
    bool reset = false;
    std::optional<int> nativeStep;
    for (int i = 1; i < argc; ++i) {
        const std::wstring arg = argv[i];
        if (arg == L"--run") {
            config.observeOnly = false;
            seconds = 0;
        } else if (arg == L"--diagnose") {
            config.observeOnly = true;
        } else if (arg == L"--smoke-zoom") {
            smoke = true;
        } else if (arg == L"--status") {
            status = true;
        } else if (arg == L"--reset") {
            reset = true;
        } else if (arg == L"--native-step" && i + 1 < argc) {
            const std::wstring direction = argv[++i];
            if (direction == L"in") nativeStep = 1;
            else if (direction == L"out") nativeStep = 2;
            else if (direction == L"reset") nativeStep = 3;
            else return 2;
        } else if (arg == L"--seconds" && i + 1 < argc) {
            wchar_t* end = nullptr;
            const unsigned long parsed = std::wcstoul(argv[++i], &end, 10);
            if (!end || *end || parsed < 1 || parsed > 3600) {
                std::fputs("--seconds must be between 1 and 3600\n", stderr);
                return 2;
            }
            seconds = static_cast<unsigned>(parsed);
        } else {
            std::fputs("Usage: pinch-zoom-host [--diagnose | --run] [--seconds 1..3600] [--smoke-zoom | --status | --reset]\n", stderr);
            return 2;
        }
    }
    if (nativeStep) {
        bridge::Mapping mapping;
        if (!mapping.open() || !mapping.magnifierAvailable()) return 3;
        InterlockedExchange(&mapping.get()->pendingCommand, *nativeStep);
        mapping.syncNative();
        return 0;
    }
    if (status) {
        bridge::Mapping mapping;
        if (!mapping.open()) return 1;
        const auto* record = mapping.get();
        std::printf("Native gate: %s; shellPid=%ld; hostEnabled=%ld; phase=%ld; lastNativeGesture=%ld; heartbeatAgeMs=%llu\n",
                    mapping.gateAvailable() ? "READY" : "NOT LOADED", record->shellPid,
                    record->enabled, record->phase, record->reserved,
                    static_cast<unsigned long long>(GetTickCount64() - record->heartbeat));
        runtime::Zoom inspector;
        if (inspector.initialize()) std::printf("Windows Magnifier: %s; pid=%ld; hidden=%ld; keyboardCommands=%ld; lastError=%ld; actualZoom=%.2f%%\n",
            mapping.magnifierAvailable() ? "READY" : "NOT LOADED", record->magnifierPid,
            record->hideUi, record->nativeCommands, record->nativeError, inspector.rendered() * 100);
        std::printf("Pointer protection: blocked=%ld; lastOriginRva=0x%lx; threeFingersDown=%ld\n", record->blockedCursorMoves, static_cast<unsigned long>(record->lastCursorOrigin), record->down);
        std::printf("Compatibility: nativeCaps=0x%lx; gateCaps=0x%lx; nativeReason=%ld; gateReason=%ld\n", record->nativeCaps, record->gateCaps, record->resolverReasonNative, record->resolverReasonGate);
        std::printf("Resolvers: native=%ld; shell=%ld; desktop=%ld (1=offline verified, 2=symbols)\n", record->resolverModeNative, record->resolverModeGate, record->resolverModeDesktop);
        std::printf("Keyboard hold: progressiveSpeed=%ld; replacedRepeats=%ld; releaseFreezes=%ld\n", record->keyboardHoldMultiplier, record->keyboardHoldRepeats, record->keyboardHoldFreezes);
        std::printf("Mouse profile: winWheel=%ld; hooksReady=%ld; ctrlAltWindowWheel=%ld; nativeViewMode=%ld\n", record->winWheel, record->mouseHooksReady, record->wheelWindowSwitch, record->nativeViewMode);
        std::printf("Read-from-point: disabled=%ld; verifiedHooks=%ld\n", record->disableReading, record->readingReady);
        std::printf("Wheel: enabled=%ld; active=%ld; nativeEvents=%ld\n", record->wheelEnabled, record->wheelActive, record->wheelEvents);
        std::printf("Cursor clip protection: blocked=%ld\n", record->blockedCursorClips);
        std::printf("Shell view: mask=%ld; applied=%ld/%ld; pendingGestures=%ld; instantSwitch=%ld; direction=%ld\n", record->shellMask, record->shellApplied, record->shellRevision, record->pendingGestures, record->instantSwitch, record->swipeDirection);
        return mapping.hostAvailable() ? 0 : 3;
    }
    if (reset) {
        runtime::Zoom zoom;
        bridge::Mapping native; native.open();
        if (!zoom.initialize(!native.magnifierProcessAlive()) || !zoom.resetToNormal()) return 1;
        runtime::log(L"Desktop zoom reset to 100%.");
        return 0;
    }
    const HANDLE single = CreateMutexW(nullptr, FALSE, L"Local\\KdePinchZoomStandalone_0_1");
    if (!single || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (single) CloseHandle(single);
        std::fputs("Another diagnostic host is already running.\n", stderr);
        return 1;
    }
    int result = 0;
    if (smoke) {
        runtime::Zoom zoom;
        if (!zoom.initialize(true) || !zoom.target(1.1) || !zoom.tick(16, 0) || std::abs(zoom.current() - 1.1) > 0.002) {
            result = 1;
            runtime::log(L"Zoom API smoke test FAILED.");
        } else {
            zoom.restore();
            if (std::abs(zoom.current() - 1.0) > 0.002) {
                result = 1;
                runtime::log(L"Zoom restoration FAILED.");
            } else {
                runtime::log(L"Zoom API: applied 110% and restored 100%.");
            }
        }
    } else {
        runtime::Host host(config);
        if (!host.initialize(GetModuleHandleW(nullptr))) {
            result = 1;
        } else {
            host.loop(nullptr, seconds);
            const auto& stats = host.statistics();
            std::wprintf(L"Summary: devices=%u rawPackets=%llu hidReports=%llu countErrors=%llu reports=%llu frames=%llu pinches=%llu swipes=%llu\n",
                         stats.devices, static_cast<unsigned long long>(stats.rawPackets),
                         static_cast<unsigned long long>(stats.hidReports),
                         static_cast<unsigned long long>(stats.countErrors),
                         static_cast<unsigned long long>(stats.reports),
                         static_cast<unsigned long long>(stats.frames),
                         static_cast<unsigned long long>(stats.pinches),
                         static_cast<unsigned long long>(stats.swipes));
            if (!stats.devices) {
                runtime::log(L"No compatible Precision Touchpad exposed in this session. Physical gesture validation is pending.");
            }
        }
    }
    CloseHandle(single);
    return result;
}

#endif  // Standalone CLI

#if !defined(PINCH_ZOOM_STANDALONE) || defined(PINCH_ZOOM_NATIVE_TEST)

namespace magnifier {
using Change = void(*)(void*, float, POINT*);
using Run = void(*)(void*);
using SetLevel = void(*)(void*, long);
using ZoomCommand = void(*)(void*, int);
using WheelCommand = void(*)(void*, int);
using MoveMouse = void(*)(void*, POINT);
using MoveTo = void(*)(void*, const RECT*, bool);
using ReadTransform = BOOL(WINAPI*)(float*, int*, int*);
static_assert(sizeof(POINT) == 8, "Native CPoint ABI is an 8-byte value");
using SetOption = void(*)(void*, int, long, bool);
using GetOption = long(*)(void*, int);
using GetKeeper = HRESULT(*)(void**);
Change originalChange = nullptr;
Run originalRun = nullptr;
SetLevel setLevel = nullptr;
ZoomCommand originalZoomCommand = nullptr;
WheelCommand originalWheel = nullptr;
WNDPROC originalUiWindowProc = nullptr, originalFullWindowProc = nullptr;
decltype(&GetWindowLongPtrW) windowData = GetWindowLongPtrW;
MoveMouse originalMoveMouse = nullptr;
decltype(&ClipCursor) originalClipCursor = nullptr;
MoveTo moveTo = nullptr;
ReadTransform readTransform = nullptr;
std::atomic<bool> overviewSuspended{false};
std::optional<pinch::View> overviewView;
std::atomic<bool> keepPointerStable{true};
std::atomic<void*> nativeUi{nullptr};
std::atomic<bool> trackingInitialized{false};
std::atomic<DWORD> rendererThread{0};
std::atomic<unsigned> rendererGeneration{0};
SetOption originalOption = nullptr;
GetOption getOption = nullptr;
GetKeeper getKeeper = nullptr;
decltype(&ShowWindow) originalShow = nullptr;
decltype(&SetWindowPos) originalPosition = nullptr;
decltype(&CreateWindowExW) originalCreate = nullptr;
decltype(&SetWindowLongPtrW) originalStyle = nullptr;
bridge::Mapping mapping;
bool role = false;
bool available = false;
// Optional Magnifier-only read-from-point entrypoints. No system mouse hook,
// click filtering or cursor replacement is installed by this mod.
using RegisterReadPoint = void(*)(void*);
using SetReadPointCursor = bool(*)(const void*);
RegisterReadPoint originalRegisterReadPoint = nullptr;
SetReadPointCursor originalReadPointRegistry = nullptr, originalReadPointDefault = nullptr;
std::atomic<bool> readingHooksReady{false};
extern std::atomic<bool> stopping;
bool suppressReadPoint() {
    const auto* state = mapping.get();
    return readingHooksReady.load() && !stopping.load() && state && state->disableReading;
}
void registerReadPoint(void* self) {
    if (!suppressReadPoint()) originalRegisterReadPoint(self);
}
bool readPointRegistry(const void* self) {
    return suppressReadPoint() ? false : originalReadPointRegistry(self);
}
bool readPointDefault(const void* self) {
    return suppressReadPoint() ? false : originalReadPointDefault(self);
}
bool resolveReadingTargets(HMODULE module, std::array<void*, 3>& targets) {
    targets.fill(nullptr);
    // PDB-verified ABI for this exact image only:
    // FocusTracker::RegisterReadFromPointInput() void(this), and both private
    // MousePointerStateManager::SetReadFromPointMouseCursors...() bool(const this).
    // Keep this identity independent of the core profiles: updating a core
    // profile must not silently approve unreviewed reading ABI/offsets.
    constexpr binary_profiles::Identity verified{{0x99,0x46,0x73,0xd3,0xf7,0xad,0x69,0xbc,
        0x0b,0x62,0xa7,0xf4,0x56,0x60,0x23,0x4f}, 1, 0x130000, 0x5a1425a3};
    constexpr std::array<DWORD, 3> rvas{0x72840, 0x898f8, 0x899bc};
    const auto identity = binary_profiles::identify(module);
    if (!identity || *identity != verified) return false;
    for (const auto rva : rvas) if (!binary_profiles::permitted(module, rva, false)) return false;
    for (std::size_t i = 0; i < targets.size(); ++i) targets[i] = reinterpret_cast<BYTE*>(module) + rvas[i];
    return true;
}
#ifndef PINCH_ZOOM_NATIVE_TEST
void installReadingHooks(HMODULE module) {
    std::array<void*, 3> targets{};
    if (!resolveReadingTargets(module, targets)) {
        Wh_Log(L"Read-from-point interception unavailable for this image; retaining native reading behavior.");
        return;
    }
    const bool installed =
        Wh_SetFunctionHook(targets[0], reinterpret_cast<void*>(registerReadPoint), reinterpret_cast<void**>(&originalRegisterReadPoint)) &&
        Wh_SetFunctionHook(targets[1], reinterpret_cast<void*>(readPointRegistry), reinterpret_cast<void**>(&originalReadPointRegistry)) &&
        Wh_SetFunctionHook(targets[2], reinterpret_cast<void*>(readPointDefault), reinterpret_cast<void**>(&originalReadPointDefault));
    // An optional partial installation always forwards originals. It cannot
    // leave point capture active while suppressing only its cursor or vice versa.
    readingHooksReady.store(installed);
    if (auto* state = mapping.get()) InterlockedExchange(&state->readingReady, installed ? 1 : 0);
    Wh_Log(L"Optional read-from-point interception ready=%d", installed);
}
#endif
bool symbolUnsupported = false;
bool hooksAttempted = false;
LONG capabilities = 0;
std::atomic<bool> hiding{true};
std::mutex stylesMutex;
std::map<HWND, LONG_PTR> styles;
HANDLE stopEvent = nullptr;
HANDLE thread = nullptr;
std::atomic<HWND> messageWindow{nullptr};
std::atomic<bool> stopping{false};
std::atomic<unsigned> activeUpdates{0};
constexpr wchar_t kNativeClass[] = L"KdePinchZoomNativeTarget.v13";
std::atomic<DWORD> uiThread{0};
std::atomic<void*> nativeSelf{nullptr}; // Dereferenced only on the native rendering thread.
std::atomic<bool> uiControlReady{false};
std::atomic<bool> uiPrimeBlocked{false}; // A failed UI prime waits for a real native callback/command.
thread_local bool applyingPinch = false;
thread_local unsigned nativeCommandDepth = 0;
constexpr UINT_PTR kKeyboardHoldTimer = 0x4b48;
constexpr UINT kCancelKeyboardHold = WM_APP + 0x61;
pinch::KeyboardHold keyboardHold;
LONG keyboardHoldRevision = 0;
std::uint64_t keyboardHoldGeneration = 0; // Native UI thread only.
int keyboardHeldVk = 0;
thread_local int keyboardHotkeyDirection = 0, keyboardHotkeyVk = 0;
thread_local DWORD keyboardHotkeyTimestamp = 0;
thread_local bool keyboardWheelMessage = false;
thread_local DWORD keyboardWheelTimestamp = 0;
// UI-thread ordering for native asynchronous WM_MOUSEWHEEL relay (0x606).
// WM_HOTKEY is promoted to the head of the queue and can overtake that relay.
bool keyboardNativeTimestampSet = false;
DWORD keyboardNativeTimestamp = 0;
std::uint64_t keyboardNativeClock = 0;
decltype(&GetAsyncKeyState) keyboardKeyState = GetAsyncKeyState;
decltype(&GetTickCount64) keyboardClock64 = GetTickCount64;
decltype(&GetMessageTime) keyboardMessageTime = GetMessageTime;
decltype(&SetTimer) keyboardSetTimer = SetTimer;
decltype(&KillTimer) keyboardKillTimer = KillTimer;
decltype(&PostMessageW) keyboardPostMessage = PostMessageW;
void cancelKeyboardHold(bool seal = true);
bool freezeKeyboardHold();
void pollKeyboardHold();
const UINT updateMessage = RegisterWindowMessageW(L"KdePinchZoom.NativeTarget.v13");
LRESULT CALLBACK messages(HWND window, UINT message, WPARAM w, LPARAM l);
void publish(float level) {
    if (mapping.get()) InterlockedExchange(&mapping.get()->nativeLevel, static_cast<LONG>(std::lround(level * 10000)));
}
// Option4 mode names in this verified image:1 Docked,2 Fullscreen,3 Lens
// (native dispatcher RVA7E858). Only the owner thread inspects native options.
// Control readiness does not require a fullscreen renderer callback at 100%.
bool refreshControlReady() {
    auto* state = mapping.get();
    const auto window = messageWindow.load();
    const auto ui = nativeUi.load();
    if (!state || stopping.load() || !ui || !window || uiThread.load() != GetCurrentThreadId() || !getKeeper || !getOption) return false;
    void* keeper = nullptr;
    if (FAILED(getKeeper(&keeper)) || !keeper) {
        uiControlReady.store(false);
        InterlockedExchange(&state->magnifierReady, 0);
        InterlockedExchange(&state->nativeError, ERROR_RETRY);
        return false;
    }
    const long nativeMode = getOption(keeper, 4);
    InterlockedExchange(&state->nativeViewMode, nativeMode);
    const bool fullscreen = nativeMode == 2;
    if (stopping.load() || messageWindow.load() != window || nativeUi.load() != ui) return false;
    if (!fullscreen) {
        nativeSelf.store(nullptr);
        rendererThread.store(0);
        trackingInitialized.store(false);
        overviewSuspended.store(false);
        overviewView.reset();
        InterlockedExchange(&state->overviewSavedLevel, 0);
    }
    const bool ready = fullscreen && !uiPrimeBlocked.load();
    const bool wasReady = uiControlReady.exchange(ready);
    if (ready && !wasReady && !nativeSelf && readTransform) {
        float factor = 1; int x = 0, y = 0;
        if (readTransform(&factor, &x, &y) && std::isfinite(factor) && factor >= 1 && factor <= 16 && !nativeSelf)
            publish(factor); // Adopt the visible baseline without creating zoom or moving the pointer.
    }
    if (stopping.load() || messageWindow.load() != window || nativeUi.load() != ui) return false;
    InterlockedExchange(&state->magnifierReady, ready ? 1 : 0);
    if (ready) InterlockedExchange(&state->nativeError, 0);
    else if (!fullscreen) InterlockedExchange(&state->nativeError, ERROR_NOT_SUPPORTED);
    return ready;
}
void ensureUi(void* self) {
    if (!self) return;
    nativeUi.store(self);
    if (messageWindow) { refreshControlReady(); return; }
#ifdef PINCH_ZOOM_NATIVE_TEST
    return;
#else
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                      reinterpret_cast<LPCWSTR>(&messages), &module);
    WNDCLASSW cls{};
    cls.hInstance = module;
    cls.lpfnWndProc = messages;
    cls.lpszClassName = kNativeClass;
    if (RegisterClassW(&cls) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
        messageWindow.store(CreateWindowExW(0, kNativeClass, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, module, nullptr));
        if (messageWindow) {
            uiThread.store(GetCurrentThreadId());
            refreshControlReady();
        }
    }
#endif
}
void run(void* self) {
    ensureUi(self);
    // Run owns the message loop. Tail-call it so no mod return address remains
    // on the UI stack when Windhawk later unloads this DLL.
    [[clang::musttail]] return originalRun(self);
}
// A reload can attach after Run/ChangeMagnificationFactor already happened.
// Bootstrap only from the owner's normal message loop; do not change zoom,
// move the pointer, or read live objects from a worker thread.
void recoverRenderer(void* self) {
    if (!self || nativeSelf || stopping.load()) return;
    void* keeper = nullptr;
    if (!getKeeper || !getOption || FAILED(getKeeper(&keeper)) || !keeper || getOption(keeper, 4) != 2) return;
    float level = 1; int x = 0, y = 0;
    if (!readTransform || !readTransform(&level, &x, &y) || !std::isfinite(level) || level < 1) return;
    nativeSelf = self;
    rendererThread.store(GetCurrentThreadId());
    publish(level);
    trackingInitialized.store(level > 1);
    if (auto* state = mapping.get()) {
        InterlockedIncrement(&state->zoomRevision);
        InterlockedIncrement(&state->inputRevision);
        uiPrimeBlocked.store(false);
        refreshControlReady();
    }
}
LRESULT CALLBACK uiWindowProc(HWND window, UINT message, WPARAM w, LPARAM l) {
    struct HotkeyContext {
        int previousDirection = keyboardHotkeyDirection;
        int previousVk = keyboardHotkeyVk;
        DWORD previousTimestamp = keyboardHotkeyTimestamp;
        bool previousWheel = keyboardWheelMessage;
        DWORD previousWheelTimestamp = keyboardWheelTimestamp;
        HotkeyContext(UINT message, WPARAM identifier, LPARAM parameters) {
            keyboardWheelMessage = message == 0x606;
            keyboardWheelTimestamp = keyboardWheelMessage ? static_cast<DWORD>(keyboardMessageTime()) : 0;
            keyboardHotkeyDirection = message == WM_HOTKEY && identifier <= 1 ? (identifier == 0 ? 1 : -1) : 0;
            keyboardHotkeyVk = 0;
            if (keyboardHotkeyDirection && parameters) {
                const unsigned modifiers = LOWORD(parameters), vk = HIWORD(parameters);
                if ((modifiers & MOD_WIN) && vk && vk <= 255) keyboardHotkeyVk = static_cast<int>(vk);
                else keyboardHotkeyDirection = 0; // Unknown/non-Win signatures stay entirely native.
            }
            keyboardHotkeyTimestamp = keyboardHotkeyDirection ? static_cast<DWORD>(keyboardMessageTime()) : 0;
        }
        ~HotkeyContext() {
            keyboardHotkeyDirection = previousDirection; keyboardHotkeyTimestamp = previousTimestamp; keyboardHotkeyVk = previousVk;
            keyboardWheelMessage = previousWheel; keyboardWheelTimestamp = previousWheelTimestamp;
        }
    } context(message, w, l);
    const LRESULT result = originalUiWindowProc(window, message, w, l);
    if (message == WM_NULL && !stopping.load()) ensureUi(reinterpret_cast<void*>(windowData(window, GWLP_USERDATA)));
    return result;
}
LRESULT CALLBACK fullWindowProc(HWND window, UINT message, WPARAM w, LPARAM l) {
    const LRESULT result = originalFullWindowProc(window, message, w, l);
    if (message == WM_NULL) recoverRenderer(reinterpret_cast<void*>(windowData(window, GWLP_USERDATA)));
    return result;
}
BOOL CALLBACK wakeExistingWindow(HWND window, LPARAM) {
    DWORD owner = 0; GetWindowThreadProcessId(window, &owner);
    if (owner == GetCurrentProcessId()) PostMessageW(window, WM_NULL, 0, 0);
    return TRUE;
}
BOOL CALLBACK wakeExistingTree(HWND window, LPARAM) {
    DWORD owner = 0; GetWindowThreadProcessId(window, &owner);
    if (owner == GetCurrentProcessId()) { wakeExistingWindow(window, 0); EnumChildWindows(window, wakeExistingWindow, 0); }
    return TRUE;
}
void change(void* self, float level, POINT* point) {
    if (!applyingPinch && !nativeCommandDepth) cancelKeyboardHold();
    // A native renderer call is itself sufficient evidence of a native command
    // while pinch is inactive, even when HandleZoomCommand is unavailable.
    struct ExternalCommand {
        bool active;
        explicit ExternalCommand(bool value) : active(value) { if (active) ++nativeCommandDepth; }
        ~ExternalCommand() { if (active) --nativeCommandDepth; }
    } externalCommand(!applyingPinch);
    if (overviewSuspended.load() && !applyingPinch) { level = 1; point = nullptr; }
    rendererThread.store(GetCurrentThreadId());
    nativeSelf = self;
    if (InterlockedExchange(&mapping.get()->startAtNormal, 0)) {
        // A pinch-started process must not replay a previously saved keyboard zoom.
        void* keeper = nullptr;
        if (SUCCEEDED(getKeeper(&keeper)) && keeper) {
            applyingPinch = true;
            originalOption(keeper, 14, 100, false);
            applyingPinch = false;
        }
        level = 1;
        point = nullptr;
    }
    const LONG before = mapping.get()->nativeLevel;
    originalChange(self, level, point); // Preserve all native animations and focus tracking.
    publish(level);
    rendererGeneration.fetch_add(1);
    if (!applyingPinch && std::abs(level * 10000.0 - before) >= 1) {
        InterlockedExchange(&mapping.get()->appliedSequence, mapping.get()->requestedSequence);
        InterlockedIncrement(&mapping.get()->zoomRevision);
        InterlockedIncrement(&mapping.get()->inputRevision);
        trackingInitialized.store(level > 1);
    }
    uiPrimeBlocked.store(false);
    refreshControlReady();
}
long readOption(void* self, int name) {
    if (name == 20 && mapping.get() && mapping.get()->keyboardStep > 0) return mapping.get()->keyboardStep;
    return getOption(self, name); // Holding never changes the configured native increment.
}

bool holdPointer() {
    if (!keepPointerStable.load() || stopping.load() || nativeCommandDepth) return false;
    if (applyingPinch) return true;
    auto* state = mapping.get();
    return state && state->controlEnabled && (state->wheelActive || (state->enabled && state->down &&
        state->phase == static_cast<LONG>(pinch::Phase::Pinch))) &&
        GetTickCount64() - static_cast<ULONGLONG>(InterlockedCompareExchange64(&state->heartbeat, 0, 0)) < 250;
}
void moveMouse(void* self, POINT point) {
    if (holdPointer()) {
        if (auto* state = mapping.get()) {
            InterlockedIncrement(&state->blockedCursorMoves);
            const auto address = reinterpret_cast<std::uintptr_t>(__builtin_return_address(0));
            const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
            InterlockedExchange(&state->lastCursorOrigin, address >= base && address - base <= MAXLONG ? static_cast<LONG>(address - base) : 0);
        }
        return; // Hardware mouse input is untouched; only Magnifier's own warp is omitted.
    }
    originalMoveMouse(self, point);
}
BOOL WINAPI clipCursor(const RECT* rectangle) {
    if (rectangle && holdPointer()) {
        if (auto* state = mapping.get()) InterlockedIncrement(&state->blockedCursorClips);
        // ClipCursor can itself relocate the pointer when its shrinking bounds
        // exclude the current pixel. Releasing it does not inject mouse movement.
        return originalClipCursor(nullptr);
    }
    return originalClipCursor(rectangle);
}
struct NativeCommandScope {
    bool previous = applyingPinch;
    unsigned rendererBefore = rendererGeneration.load();
    NativeCommandScope() { ++nativeCommandDepth; applyingPinch = false; }
    ~NativeCommandScope() { applyingPinch = previous; --nativeCommandDepth; }
};
void nativeCommandComplete(unsigned rendererBefore) {
    if (applyingPinch) return;
    void* keeper = nullptr;
    if (FAILED(getKeeper(&keeper)) || !keeper) return;
    if (rendererGeneration.load() == rendererBefore) publish(getOption(keeper, 14) / 100.0f);
    InterlockedExchange(&mapping.get()->appliedSequence, mapping.get()->requestedSequence);
    InterlockedIncrement(&mapping.get()->inputRevision);
    InterlockedExchange(&mapping.get()->wheelActive, 0);
    InterlockedIncrement(&mapping.get()->nativeCommands);
    InterlockedIncrement(&mapping.get()->zoomRevision);
    trackingInitialized.store(mapping.get()->nativeLevel > 10000);
}
bool updateOverview(bool preserveInput = false);
void resumeForNativeCommand(bool preserveInput = false) {
    auto* state = mapping.get();
    if (state && (state->shellMask || overviewSuspended.load())) InterlockedExchange(&state->overviewBypassed, 1);
    if (!overviewSuspended.load()) return;
    // Explicit native controls always win over temporary overview suspension.
    if (!updateOverview(preserveInput)) {
        overviewSuspended.store(false); overviewView.reset();
        if (state) InterlockedExchange(&state->overviewSavedLevel, 0);
    }
}
void uiLevel(void* self, long level) {
    if (!nativeCommandDepth) cancelKeyboardHold();
    ensureUi(self);
    resumeForNativeCommand();
    NativeCommandScope scope;
    setLevel(self, level);
    nativeCommandComplete(scope.rendererBefore);
}
bool keyboardHoldAvailable() {
    const auto* state = mapping.get();
    constexpr auto required = compatibility::NativeCore | compatibility::NativeKeyboardObserver |
                              compatibility::NativeViewportRestore | compatibility::NativeBootstrapWindows;
    return state && !stopping.load() && state->controlEnabled && state->magnifierReady &&
        state->keyboardHoldMultiplier > 1 && (static_cast<unsigned>(state->nativeCaps) & required) == required &&
        messageWindow && nativeSelf && nativeUi && readTransform && moveTo && originalChange && originalOption && setLevel &&
        rendererThread.load() == GetCurrentThreadId() && uiThread.load() == GetCurrentThreadId();
}
int physicalZoomDirection(int nativeDirection = 0, int nativeVk = 0) {
    const auto down = [](int key) { return (keyboardKeyState(key) & 0x8000) != 0; };
    if ((!down(VK_LWIN) && !down(VK_RWIN)) || down(VK_ESCAPE)) return 0;
    // The native WM_HOTKEY carries the real layout-dependent key. The fixed
    // keys below are only a fallback for manually posted messages with lParam0.
    if (nativeDirection && nativeVk) return down(nativeVk) ? nativeDirection : 0;
    const bool plus = down(VK_OEM_PLUS) || down(VK_ADD);
    const bool minus = down(VK_OEM_MINUS) || down(VK_SUBTRACT);
    if (plus == minus) return 0;
    return plus ? 1 : -1;
}
void cancelKeyboardHold(bool seal) {
    // A future renderer/settings thread must never mutate UI-owned hold state.
    if (uiThread.load() != GetCurrentThreadId()) {
        if (const HWND window = messageWindow.load()) keyboardPostMessage(window, kCancelKeyboardHold, seal ? 0 : 1, 0);
        return;
    }
    if (const HWND window = messageWindow.load()) keyboardKillTimer(window, kKeyboardHoldTimer);
    ++keyboardHoldGeneration;
    keyboardHold.close(keyboardClock64(), seal);
    keyboardHeldVk = 0;
}
bool freezeKeyboardHold() {
    auto* state = mapping.get();
    if (!keyboardHoldAvailable() || !state || state->inputRevision != keyboardHoldRevision || overviewSuspended.load()) return false;
    const auto expectedSelf = nativeSelf.load();
    const auto expectedUi = nativeUi.load();
    const auto expectedWindow = messageWindow.load();
    const LONG expectedRevision = state->inputRevision;
    const auto expectedGeneration = keyboardHoldGeneration;
    struct ActiveFreeze {
        bool previous = applyingPinch;
        ActiveFreeze() { activeUpdates.fetch_add(1); applyingPinch = true; }
        ~ActiveFreeze() { applyingPinch = previous; activeUpdates.fetch_sub(1); }
    } active;
    float level = 1; int x = 0, y = 0;
    if (!readTransform(&level, &x, &y) || !std::isfinite(level) || level < 1 || level > 16) return false;
    void* keeper = nullptr;
    if (FAILED(getKeeper(&keeper)) || !keeper) return false;
    // Reading the native view/keeper can reenter other Windows callbacks. A
    // close, mode switch or newer input must win before any target is written.
    if (!keyboardHoldAvailable() || nativeSelf.load() != expectedSelf || nativeUi.load() != expectedUi ||
        messageWindow.load() != expectedWindow || state->inputRevision != expectedRevision ||
        keyboardHoldGeneration != expectedGeneration || overviewSuspended.load()) return false;
    // Retarget Windows' engine once to its rendered factor, not to the last
    // queued integer goal. Preserve the currently visible source rectangle.
    const long percent = std::clamp<long>(static_cast<long>(std::lround(level * 100)), level > 1 ? 101 : 100, 1600);
    const bool primeTracking = level > 1 && (!trackingInitialized.load() || state->nativeLevel <= 10000);
    originalOption(keeper, 14, percent, false);
    if (primeTracking) {
        // Win-minus may already have targeted 100% while the displayed frame
        // is still magnified. Re-enable the native tracking lifecycle once.
        setLevel(expectedUi, percent);
        trackingInitialized.store(true);
    }
    originalChange(nativeSelf, level, nullptr);
    if (level > 1) {
        alignas(16) RECT rectangle{x, y,
            x + static_cast<LONG>(std::lround(std::max(1, GetSystemMetrics(SM_CXVIRTUALSCREEN)) / level)),
            y + static_cast<LONG>(std::lround(std::max(1, GetSystemMetrics(SM_CYVIRTUALSCREEN)) / level))};
        moveTo(nativeSelf, &rectangle, true);
    }
    InterlockedExchange(&state->currentSource, static_cast<LONG>(bridge::InputSource::Keyboard));
    publish(level);
    trackingInitialized.store(level > 1);
    InterlockedExchange(&state->appliedSequence, state->requestedSequence);
    InterlockedExchange(&state->pendingCommand, 0);
    InterlockedExchange(&state->wheelActive, 0);
    InterlockedExchange64(&state->wheelPacket, 0);
    InterlockedIncrement(&state->zoomRevision);
    InterlockedIncrement(&state->inputRevision);
    InterlockedIncrement(&state->keyboardHoldFreezes);
    return true;
}
bool beginKeyboardContinuous(std::uint64_t now) {
    auto* state = mapping.get();
    if (!keyboardHoldAvailable() || !state || state->inputRevision != keyboardHoldRevision) return false;
    const auto expectedSelf = nativeSelf.load(), expectedUi = nativeUi.load();
    const auto expectedWindow = messageWindow.load();
    const LONG expectedRevision = state->inputRevision;
    const auto expectedGeneration = keyboardHoldGeneration;
    struct ActiveRead {
        ActiveRead() { activeUpdates.fetch_add(1); }
        ~ActiveRead() { activeUpdates.fetch_sub(1); }
    } active;
    float level = 1; int x = 0, y = 0;
    if (!readTransform(&level, &x, &y) || !std::isfinite(level) || level < 1 || level > 16) return false;
    if (!keyboardHoldAvailable() || nativeSelf.load() != expectedSelf || nativeUi.load() != expectedUi ||
        messageWindow.load() != expectedWindow || state->inputRevision != expectedRevision ||
        keyboardHoldGeneration != expectedGeneration || overviewSuspended.load()) return false;
    keyboardHold.activate(level, now); // Read the visible factor only once, never as frame feedback.
    InterlockedExchange(&state->appliedSequence, state->requestedSequence);
    InterlockedExchange(&state->pendingCommand, 0);
    InterlockedExchange(&state->wheelActive, 0);
    InterlockedExchange64(&state->wheelPacket, 0);
    InterlockedExchange(&state->currentSource, static_cast<LONG>(bridge::InputSource::Keyboard));
    InterlockedIncrement(&state->zoomRevision);
    keyboardHoldRevision = InterlockedIncrement(&state->inputRevision);
    return true;
}
bool writeKeyboardContinuous(double level) {
    auto* state = mapping.get();
    if (!keyboardHoldAvailable() || !state || state->inputRevision != keyboardHoldRevision) return false;
    const auto expectedSelf = nativeSelf.load(), expectedUi = nativeUi.load();
    const auto expectedWindow = messageWindow.load();
    const LONG expectedRevision = state->inputRevision;
    const auto expectedGeneration = keyboardHoldGeneration;
    struct ActiveFrame {
        bool previous = applyingPinch;
        ActiveFrame() { activeUpdates.fetch_add(1); applyingPinch = true; }
        ~ActiveFrame() { applyingPinch = previous; activeUpdates.fetch_sub(1); }
    } active;
    void* keeper = nullptr;
    if (FAILED(getKeeper(&keeper)) || !keeper) return false;
    if (!keyboardHoldAvailable() || nativeSelf.load() != expectedSelf || nativeUi.load() != expectedUi ||
        messageWindow.load() != expectedWindow || state->inputRevision != expectedRevision ||
        keyboardHoldGeneration != expectedGeneration || overviewSuspended.load()) return false;
    const bool prime = level > 1 && (!trackingInitialized.load() || state->nativeLevel <= 10000);
    const bool reset = level <= 1 && state->nativeLevel > 10000;
    const long percent = static_cast<long>(std::lround(level * 100));
    originalOption(keeper, 14, prime ? std::max(101L, percent) : percent, false);
    if (prime || reset) {
        setLevel(expectedUi, prime ? std::max(101L, percent) : 100L);
        if (!keyboardHoldAvailable() || nativeSelf.load() != expectedSelf || nativeUi.load() != expectedUi ||
            messageWindow.load() != expectedWindow || state->inputRevision != expectedRevision ||
            keyboardHoldGeneration != expectedGeneration) return false;
    }
    if (!reset) originalChange(expectedSelf, static_cast<float>(level), nullptr);
    if (prime) originalOption(keeper, 14, percent, false);
    trackingInitialized.store(level > 1);
    InterlockedExchange(&state->currentSource, static_cast<LONG>(bridge::InputSource::Keyboard));
    publish(static_cast<float>(level));
    return true;
}
void pollKeyboardHold() {
    if (nativeCommandDepth || !keyboardHold.active()) return;
    auto* state = mapping.get();
    if (!keyboardHoldAvailable() || !state || state->inputRevision != keyboardHoldRevision ||
        state->wheelActive || (state->down && state->phase == static_cast<LONG>(pinch::Phase::Pinch))) {
        cancelKeyboardHold();
        return; // A newer input wins; never freeze its target.
    }
    if (physicalZoomDirection(keyboardHold.direction(), keyboardHeldVk) != keyboardHold.direction()) {
        const bool continuous = keyboardHold.continuous();
        cancelKeyboardHold();
        if (continuous) freezeKeyboardHold();
        return;
    }
    const auto now = keyboardClock64();
    if (keyboardHold.eligible(now) && !beginKeyboardContinuous(now)) { cancelKeyboardHold(); return; }
    if (const auto level = keyboardHold.tick(now, state->keyboardHoldMultiplier, state->maxZoomPercent / 100.0))
        if (!writeKeyboardContinuous(*level)) cancelKeyboardHold();
}
void zoomCommand(void* self, int command) {
    const int direction = command == 0x800a ? 1 : command == 0x800b ? -1 : 0;
    if (direction && mapping.get()) {
        InterlockedExchange(&mapping.get()->startAtNormal, 0); // A real native key owns its initial activation.
        uiPrimeBlocked.store(false);
    }
    ensureUi(self);
    const bool hotkey = direction && keyboardHotkeyDirection == direction;
    if (hotkey && keyboardHoldAvailable()) {
        if (keyboardHold.active() && keyboardHold.direction() != direction) {
            // Another keyboard direction starts from the visible view, not from
            // the old hold's ahead-of-animation goal. Other input actors cancel
            // without freezing and keep their existing priority paths.
            const bool continuous = keyboardHold.continuous();
            cancelKeyboardHold();
            if (continuous) freezeKeyboardHold();
        } else pollKeyboardHold();
        if (keyboardHold.stale(direction, keyboardHotkeyTimestamp, keyboardClock64())) return;
        if (keyboardHold.active() && keyboardHold.direction() == direction) {
            InterlockedIncrement(&mapping.get()->keyboardHoldRepeats);
            return; // Only this held native hotkey's repeats are replaced by continuous frames.
        }
    } else if (!nativeCommandDepth) cancelKeyboardHold();
    resumeForNativeCommand();
    {
        NativeCommandScope scope;
        if (hotkey) {
            // Record only a native hotkey actually forwarded below. Buttons and
            // consumed autorepeats do not create a new physical ordering fence.
            keyboardNativeTimestampSet = true;
            keyboardNativeTimestamp = keyboardHotkeyTimestamp;
            keyboardNativeClock = keyboardClock64();
        }
        originalZoomCommand(self, command); // A first tap and every unrelated command remain native.
        nativeCommandComplete(scope.rendererBefore);
    }
    // A first Win-plus can itself create the renderer. Recheck after forwarding it.
    if (hotkey && keyboardHoldAvailable() && physicalZoomDirection(direction, keyboardHotkeyVk) == direction) {
        keyboardHeldVk = keyboardHotkeyVk;
        keyboardHold.begin(direction, keyboardClock64());
        keyboardHoldRevision = mapping.get()->inputRevision;
        if (!keyboardSetTimer(messageWindow, kKeyboardHoldTimer, 16, nullptr)) cancelKeyboardHold(false);
    }
}
// This is the native Ctrl+Alt-wheel handler. Whichever low-level hook runs
// first consumes the event, so Source and Magnifier never both enqueue it.
void wheel(void* self, int delta) {
    const auto now = keyboardClock64();
    if (keyboardWheelMessage && keyboardNativeTimestampSet && now >= keyboardNativeClock &&
        now - keyboardNativeClock <= 60000 &&
        static_cast<std::int32_t>(keyboardWheelTimestamp - keyboardNativeTimestamp) < 0) {
        return; // An older queued wheel relay cannot cancel or overwrite a newer accepted key.
    }
    if (!nativeCommandDepth) cancelKeyboardHold();
    ensureUi(self);
    auto* state = mapping.get();
    const bool winProfile = state && state->winWheel && state->mouseHooksReady;
    if (state && state->nativeViewMode != 1 && state->nativeViewMode != 3 && state->controlEnabled &&
        GetTickCount64() - static_cast<ULONGLONG>(InterlockedCompareExchange64(&state->heartbeat, 0, 0)) < 250 &&
        InterlockedCompareExchange64(&state->hostWindow, 0, 0)) {
        if (winProfile && state->wheelWindowSwitch &&
            !(GetAsyncKeyState(VK_SHIFT) & 0x8000) && !(GetAsyncKeyState(VK_LWIN) & 0x8000) && !(GetAsyncKeyState(VK_RWIN) & 0x8000)) {
            bridge::enqueueWindowWheel(state, delta, GetTickCount64());
            return;
        }
        if (!winProfile && state->wheelEnabled && state->magnifierReady) {
            resumeForNativeCommand();
            const LONG revision = InterlockedCompareExchange(&state->inputRevision, 0, 0);
            bridge::enqueueWheel(state, delta, revision, GetTickCount64());
            InterlockedIncrement(&state->wheelEvents);
            return;
        }
    }
    originalWheel(self, delta); // Lens, opt-out and an absent host retain native behavior.
}

void option(void* self, int name, long value, bool save) {
    if (applyingPinch) save = false; // No registry writes for every HID frame.
    const long previousMode = name == 4 ? getOption(self, 4) : 0;
    originalOption(self, name, value, save);
    if (name == 4 && mapping.get() && previousMode != getOption(self, 4)) {
        cancelKeyboardHold();
        // Mode changes can destroy the fullscreen renderer. Do not keep using it.
        InterlockedExchange(&mapping.get()->magnifierReady, 0);
        nativeSelf = nullptr;
        rendererThread.store(0);
        uiControlReady.store(false);
        uiPrimeBlocked.store(false);
        if (uiThread.load() == GetCurrentThreadId()) refreshControlReady();
    }
}
bool overviewAvailable() {
    const auto* state = mapping.get();
    return state && moveTo && compatibility::overviewAllowed(
        static_cast<std::uint32_t>(state->nativeCaps), static_cast<std::uint32_t>(state->gateCaps));
}
bool updateOverview(bool preserveInput) {
    auto* state = mapping.get();
    if (!(state->shellMask & 1)) InterlockedExchange(&state->overviewBypassed, 0);
    const bool suspend = !state->overviewBypassed && overviewAvailable() && state->controlEnabled && state->normalInOverview && (state->shellMask & 1) != 0 && (overviewSuspended.load() || !state->pendingGestures);
    if (suspend == overviewSuspended.load()) { state->shellApplied = state->shellRevision; return true; }
    cancelKeyboardHold();
    if (!nativeSelf || !readTransform || !moveTo || rendererThread.load() != GetCurrentThreadId()) return false;
    void* keeper = nullptr;
    if (FAILED(getKeeper(&keeper)) || !keeper) return false;
    float zoom = 1; int x = 0, y = 0;
    if (suspend && !readTransform(&zoom, &x, &y)) return false;
    const bool previous = applyingPinch;
    applyingPinch = true;
    if (keepPointerStable.load() && originalClipCursor) originalClipCursor(nullptr);
    if (suspend) {
        overviewView = pinch::View{zoom, x, y};
        InterlockedExchange(&state->overviewSavedLevel, static_cast<LONG>(std::lround(zoom * 10000)));
        overviewSuspended.store(true);
        originalOption(keeper, 14, 100, false);
        originalChange(nativeSelf, 1, nullptr);
        publish(1);
    } else {
        const auto saved = overviewView.value_or(pinch::View{});
        overviewSuspended.store(false);
        originalOption(keeper, 14, static_cast<long>(std::lround(saved.zoom * 100)), false);
        originalChange(nativeSelf, static_cast<float>(saved.zoom), nullptr);
        if (saved.zoom > 1) {
            const int width = std::max(1, GetSystemMetrics(SM_CXVIRTUALSCREEN));
            const int height = std::max(1, GetSystemMetrics(SM_CYVIRTUALSCREEN));
            alignas(16) RECT rectangle{saved.x, saved.y,
                saved.x + static_cast<LONG>(std::lround(width / saved.zoom)),
                saved.y + static_cast<LONG>(std::lround(height / saved.zoom))};
            moveTo(nativeSelf, &rectangle, true);
        }
        publish(static_cast<float>(saved.zoom));
        trackingInitialized.store(saved.zoom > 1);
        overviewView.reset();
        InterlockedExchange(&state->overviewSavedLevel, 0);
    }
    applyingPinch = previous;
    InterlockedExchange(&state->appliedSequence, state->requestedSequence);
    InterlockedIncrement(&state->zoomRevision);
    if (!preserveInput) {
        InterlockedIncrement(&state->inputRevision);
        InterlockedExchange(&state->wheelActive, 0);
    }
    InterlockedExchange(&state->shellApplied, state->shellRevision);
    return true;
}
LRESULT CALLBACK messages(HWND window, UINT message, WPARAM w, LPARAM l) {
    if (message == kCancelKeyboardHold) { cancelKeyboardHold(w == 0); return 0; }
    if (message == WM_TIMER && w == kKeyboardHoldTimer) {
        // This WndProc callback is not a Windhawk hook. Count it before any
        // external call so a reentrant WM_CLOSE pins outstanding callback code.
        struct ActiveTimer {
            ActiveTimer() { activeUpdates.fetch_add(1); }
            ~ActiveTimer() { activeUpdates.fetch_sub(1); }
        } active;
        pollKeyboardHold(); return 0;
    }
    if (message == WM_CLOSE) {
        cancelKeyboardHold();
        const bool reentrant = activeUpdates.load() != 0;
        DestroyWindow(window);
        return reentrant ? 1 : 0;
    }
    if (message == WM_NCDESTROY) { cancelKeyboardHold(); messageWindow.store(nullptr); return DefWindowProcW(window, message, w, l); }
    if (message == updateMessage) {
        if (stopping.load()) return 0;
        struct ActiveUpdate {
            ActiveUpdate() { activeUpdates.fetch_add(1); }
            ~ActiveUpdate() { activeUpdates.fetch_sub(1); }
        } active;
        auto* state = mapping.get();
        if (w == 1) { updateOverview(); return 0; }
        const LONG sequence = InterlockedCompareExchange(&state->requestedSequence, 0, 0);
        const LONG action = InterlockedExchange(&state->pendingCommand, 0);
        const auto source = action == 3 ? bridge::InputSource::Reset : action ? bridge::InputSource::Keyboard :
            static_cast<bridge::InputSource>(InterlockedCompareExchange(&state->requestSource, 0, 0));
        void* keeper = nullptr;
        if ((action || sequence != state->appliedSequence) && (state->controlEnabled || action == 3)) {
            const bool stale = !action && (state->requestRevision != state->zoomRevision ||
                                           state->requestInputRevision != state->inputRevision);
            const bool explicitSource = source == bridge::InputSource::Pinch || source == bridge::InputSource::Wheel ||
                                        source == bridge::InputSource::Keyboard || source == bridge::InputSource::Reset;
            if (stale || !explicitSource) {
                InterlockedExchange(&state->appliedSequence, sequence);
                return 0; // Old input cannot dismiss overview, cancel another actor or acquire ownership.
            }
            if (!nativeUi || !messageWindow || uiThread.load() != GetCurrentThreadId()) {
                InterlockedExchange(&state->nativeError, ERROR_INVALID_THREAD_ID);
                return 0;
            }
            if (FAILED(getKeeper(&keeper)) || !keeper) {
                InterlockedExchange(&state->nativeError, ERROR_RETRY);
                return 0;
            }
            if (getOption(keeper, 4) != 2) {
                uiControlReady.store(false);
                InterlockedExchange(&state->magnifierReady, 0);
                InterlockedExchange(&state->nativeError, ERROR_NOT_SUPPORTED);
                return 0;
            }
            if (uiPrimeBlocked.load() && !nativeSelf && action != 3) {
                InterlockedExchange(&state->nativeError, ERROR_RETRY);
                return 0; // No repeated setters for an unacknowledged cold request.
            }
            // Only an objective validated before restoration can override the
            // temporary overview. Our restoration itself may advance epochs.
            cancelKeyboardHold();
            if (source == bridge::InputSource::Reset && overviewSuspended.load()) {
                InterlockedExchange(&state->overviewBypassed, 1);
                overviewSuspended.store(false);
                overviewView.reset();
                InterlockedExchange(&state->overviewSavedLevel, 0);
            } else {
                resumeForNativeCommand(true);
            }
            double level = InterlockedCompareExchange(&state->requestedLevel, 0, 0) / 10000.0;
            if (action) {
                const double current = getOption(keeper, 14) / 100.0;
                const double step = getOption(keeper, 20) / 100.0;
                level = action == 3 ? 1 : current + (action == 1 ? step : -step);
            }
            const double cap = std::clamp(state->maxZoomPercent / 100.0, 1.0, 16.0);
            level = std::clamp(level, 1.0, cap);
            const bool hasRenderer = nativeSelf != nullptr;
            if (hasRenderer && rendererThread.load() != GetCurrentThreadId()) {
                InterlockedExchange(&state->nativeError, ERROR_INVALID_THREAD_ID);
                return 0;
            }
            const double previous = state->nativeLevel / 10000.0;
            const bool prime = level > 1 && (!hasRenderer || !trackingInitialized.load() || previous <= 1);
            const bool reset = level <= 1 && (previous > 1 || action == 3);
            double delivered = level;
            if (prime || reset || std::abs(level - previous) >= 0.00005) {
                struct Applying {
                    bool previous = applyingPinch;
                    Applying() { applyingPinch = true; }
                    ~Applying() { applyingPinch = previous; }
                } applying;
                if (keepPointerStable.load() && originalClipCursor) originalClipCursor(nullptr);
                long percent = static_cast<long>(std::lround(level * 100));
                if (prime) percent = std::max(101L, percent);
                if (prime || reset) {
                    // UI owns the first transition even when the fullscreen
                    // window/renderer has not yet been observed at cold 100%.
                    InterlockedExchange(&state->startAtNormal, 0);
                    if (reset) uiPrimeBlocked.store(false);
                    originalOption(keeper, 14, percent, false);
                    setLevel(nativeUi, percent);
                    delivered = percent / 100.0;
                    if (prime && (!nativeSelf || rendererThread.load() != GetCurrentThreadId())) {
                        uiPrimeBlocked.store(true);
                        uiControlReady.store(false);
                        trackingInitialized.store(false);
                        InterlockedExchange(&state->magnifierReady, 0);
                        InterlockedExchange(&state->nativeError, ERROR_RETRY);
                        return 0; // Keep sequence unacknowledged; no NULL call or automatic retry loop.
                    }
                    trackingInitialized.store(prime && nativeSelf);
                    if (prime && !action && source != bridge::InputSource::Reset && std::abs(delivered - level) >= 0.000001) {
                        originalChange(nativeSelf, static_cast<float>(level), nullptr);
                        originalOption(keeper, 14, static_cast<long>(std::lround(level * 100)), false);
                        delivered = level;
                    }
                } else {
                    if (!nativeSelf || rendererThread.load() != GetCurrentThreadId()) {
                        InterlockedExchange(&state->nativeError, ERROR_RETRY);
                        return 0;
                    }
                    originalOption(keeper, 14, percent, false);
                    originalChange(nativeSelf, static_cast<float>(level), nullptr);
                }
            }
            publish(static_cast<float>(delivered));
            InterlockedExchange(&state->currentSource, static_cast<LONG>(source));
            if (prime) InterlockedIncrement(&state->zoomRevision);
            InterlockedExchange(&state->nativeError, 0);
            InterlockedExchange(&state->appliedSequence, sequence);
            if (action) { InterlockedIncrement(&state->nativeCommands); InterlockedIncrement(&state->zoomRevision); InterlockedIncrement(&state->inputRevision); }
            refreshControlReady();
        }
        return 0;
    }
    return DefWindowProcW(window, message, w, l);
}
bool isUi(HWND window) {
    DWORD process = 0;
    GetWindowThreadProcessId(window, &process);
    if (process != GetCurrentProcessId()) return false;
    wchar_t name[128]{}, title[128]{};
    GetClassNameW(window, name, ARRAYSIZE(name));
    GetWindowTextW(window, title, ARRAYSIZE(title));
    return wcscmp(name, L"MagUIClass") == 0 || wcscmp(name, L"ScreenMagnifierUIWnd") == 0 ||
           wcsstr(title, L"Magnifier Touch") != nullptr;
}
void hide(HWND window) {
    const LONG_PTR style = GetWindowLongPtrW(window, GWL_EXSTYLE);
    {
        std::lock_guard lock(stylesMutex);
        styles.try_emplace(window, style);
    }
    originalStyle(window, GWL_EXSTYLE, (style & ~WS_EX_APPWINDOW) | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);
    originalShow(window, SW_HIDE);
}
BOOL CALLBACK visibility(HWND window, LPARAM) {
    if (!isUi(window)) return TRUE;
    if (hiding.load()) hide(window);
    else {
        std::optional<LONG_PTR> saved;
        {
            std::lock_guard lock(stylesMutex);
            if (auto found = styles.find(window); found != styles.end()) { saved = found->second; styles.erase(found); }
        }
        if (saved) {
            originalStyle(window, GWL_EXSTYLE, *saved);
            wchar_t name[128]{};
            GetClassNameW(window, name, ARRAYSIZE(name));
            if (wcscmp(name, L"MagUIClass") == 0 || wcscmp(name, L"ScreenMagnifierUIWnd") == 0)
                originalShow(window, SW_SHOWNOACTIVATE);
        }
    }
    return TRUE;
}
BOOL WINAPI show(HWND window, int command) {
    return originalShow(window, hiding.load() && isUi(window) ? SW_HIDE : command);
}
BOOL WINAPI position(HWND window, HWND after, int x, int y, int width, int height, UINT flags) {
    if (hiding.load() && isUi(window)) flags = (flags & ~SWP_SHOWWINDOW) | SWP_HIDEWINDOW;
    return originalPosition(window, after, x, y, width, height, flags);
}
LONG_PTR WINAPI style(HWND window, int index, LONG_PTR value) {
    if (hiding.load() && index == GWL_EXSTYLE && isUi(window)) {
        { std::lock_guard lock(stylesMutex); styles.try_emplace(window, GetWindowLongPtrW(window, GWL_EXSTYLE)); }
        value = (value & ~WS_EX_APPWINDOW) | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    }
    return originalStyle(window, index, value);
}
HWND WINAPI create(DWORD ex, LPCWSTR name, LPCWSTR title, DWORD flags, int x, int y, int width, int height,
                   HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter) {
    const bool named = reinterpret_cast<ULONG_PTR>(name) > 0xFFFF;
    if (hiding.load() && named &&
        (wcscmp(name, L"MagUIClass") == 0 || wcscmp(name, L"ScreenMagnifierUIWnd") == 0 ||
         (title && wcsstr(title, L"Magnifier Touch")))) flags &= ~WS_VISIBLE;
    const HWND window = originalCreate(ex, name, title, flags, x, y, width, height, parent, menu, instance, parameter);
    if (window && hiding.load() && isUi(window)) hide(window);
    return window;
}

DWORD WINAPI worker(void*) {
    LONG last = mapping.get()->appliedSequence, lastCommand = -1;
    const HANDLE events[] = {stopEvent, mapping.wakeEvent()};
    for (;;) {
        const DWORD result = events[1] ? WaitForMultipleObjects(2, events, FALSE, 250) : WaitForSingleObject(stopEvent, 4);
        if (result == WAIT_OBJECT_0 || result == WAIT_FAILED) break;
        auto* state = mapping.get();
        if (!state) continue;
        const bool hideNow = InterlockedCompareExchange(&state->hideUi, 0, 0) != 0 &&
            (state->magnifierReady || (capabilities & compatibility::NativeBootstrapWindows));
        if (hiding.exchange(hideNow) != hideNow) EnumWindows(visibility, 0);
        const LONG sequence = InterlockedCompareExchange(&state->requestedSequence, 0, 0);
        const LONG command = InterlockedCompareExchange(&state->syncRequest, 0, 0);
        const HWND target = messageWindow.load();
        const bool shouldSuspend = !state->overviewBypassed && overviewAvailable() && state->controlEnabled && state->normalInOverview && (state->shellMask & 1) != 0 && (overviewSuspended.load() || !state->pendingGestures);
        if (target && (overviewAvailable() || overviewSuspended.load()) &&
            (state->shellApplied != state->shellRevision || shouldSuspend != overviewSuspended.load()))
            PostMessageW(target, updateMessage, 1, 0);
        // Coalesce pending HID updates; process them on Windows' own UI thread.
        if (target && (sequence != last || command != lastCommand)) {
            if (((sequence != last && sequence) || state->pendingCommand) && PostMessageW(target, updateMessage, 0, 0)) {
                last = sequence; lastCommand = command;
            } else if (sequence == last && !state->pendingCommand) { lastCommand = command; }
        }
    }
    return 0;
}
#ifndef PINCH_ZOOM_NATIVE_TEST
bool initialize(bool supportedBuild = true) {
    if (!mapping.open()) return false;
    role = true;
    InterlockedExchange(&mapping.get()->magnifierReady, 0);
    InterlockedExchange(&mapping.get()->nativeViewMode, 0);
    InterlockedExchange(&mapping.get()->readingReady, 0);
    available = false;
    capabilities = 0;
    symbolUnsupported = false;
    hooksAttempted = false;
    const auto unsupported = [] {
        symbolUnsupported = true;
        InterlockedExchange(&mapping.get()->nativeCaps, 0);
        InterlockedExchange(&mapping.get()->startAtNormal, 0);
        InterlockedExchange(&mapping.get()->resolverReasonNative, ERROR_NOT_SUPPORTED);
        InterlockedExchange(&mapping.get()->nativeError, ERROR_NOT_SUPPORTED);
        return false; // No hook was attempted: native Windows remains untouched.
    };
    if (!supportedBuild) return unsupported();
    void* targets[12]{};
    const HMODULE module = GetModuleHandleW(nullptr);
    const bool profiled = binary_profiles::resolve(module, binary_profiles::Kind::Magnifier, targets, std::size(targets));
    if (!profiled) {
        WH_FIND_SYMBOL symbol{};
        const HANDLE search = Wh_FindFirstSymbol(GetModuleHandleW(nullptr), nullptr, &symbol);
        if (!search) return unsupported();
        do {
            if (wcsstr(symbol.symbol, L"FullscreenMagnifier::ChangeMagnificationFactor(float,class CPoint")) targets[0] = symbol.address;
            if (wcsstr(symbol.symbol, L"OptionsKeeper::SetOption(") && wcsstr(symbol.symbol, L"long,bool")) targets[1] = symbol.address;
            if (wcsstr(symbol.symbol, L"OptionsKeeper::GetOption(")) targets[2] = symbol.address;
            if (wcsstr(symbol.symbol, L"OptionsKeeper::GetOptionsKeeper(")) targets[3] = symbol.address;
            if (wcsstr(symbol.symbol, L"MagnifierUI::Run(")) targets[4] = symbol.address;
            if (wcsstr(symbol.symbol, L"MagnifierUI::HandleSetZoomLevel(")) targets[5] = symbol.address;
            if (wcsstr(symbol.symbol, L"MagnifierUI::HandleZoomCommand(")) targets[6] = symbol.address;
            if (wcsstr(symbol.symbol, L" Magnifier::MoveMouse(")) targets[7] = symbol.address;
            if (wcsstr(symbol.symbol, L"FullscreenMagnifier::MoveTo(")) targets[8] = symbol.address;
            if (wcsstr(symbol.symbol, L"MagnifierUI::HandleMouseWheelZoom(")) targets[9] = symbol.address;
            if (wcsstr(symbol.symbol, L"MagnifierUI::s_WindowProc(")) targets[10] = symbol.address;
            if (wcsstr(symbol.symbol, L"FullscreenMagnifier::StaticWndProc(")) targets[11] = symbol.address;
            if (std::all_of(std::begin(targets), std::end(targets), [](void* target) { return target != nullptr; })) break;
        } while (Wh_FindNextSymbol(search, &symbol));
        Wh_FindCloseSymbol(search);
    }
    // USERDATA-to-object recovery is verified only for exact image profiles.
    // A changed binary may still expose the same static WndProc names with a
    // different storage layout. Capture self from native Run/commands instead.
    if (!profiled) { targets[10] = nullptr; targets[11] = nullptr; }
    InterlockedExchange(&mapping.get()->resolverModeNative, profiled ? 1 : 2);
    for (auto& target : targets) if (target) {
        const auto offset = reinterpret_cast<ULONG_PTR>(target) - reinterpret_cast<ULONG_PTR>(module);
        if (offset > MAXDWORD || !binary_profiles::permitted(module, static_cast<DWORD>(offset), false)) target = nullptr;
    }
    std::uint32_t found = 0;
    for (unsigned i = 0; i < std::size(targets); ++i) if (targets[i]) found |= compatibility::symbol(i);
    const auto plan = compatibility::nativePlan(found);
    if (!plan.supported) {
        Wh_Log(L"Native core/bootstrap unavailable; retaining native Windows shortcuts and wheel.");
        return unsupported();
    }
    const auto api = LoadLibraryExW(L"Magnification.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!api) return unsupported();
    readTransform = reinterpret_cast<ReadTransform>(GetProcAddress(api, "MagGetFullscreenTransform"));
    if (!readTransform) return unsupported();
    moveTo = reinterpret_cast<MoveTo>(targets[8]);
    getOption = reinterpret_cast<GetOption>(targets[2]);
    getKeeper = reinterpret_cast<GetKeeper>(targets[3]);
    hiding.store(Wh_GetIntSetting(L"hideMagnifier") != 0 && (plan.capabilities & compatibility::NativeBootstrapWindows));
    keepPointerStable.store(Wh_GetIntSetting(L"keepPointerStable") != 0);
    hooksAttempted = true;
    // Missing optional groups are skipped before any member is installed. An
    // actual hook failure is fatal, so Windhawk rolls back this initialization.
    const bool installed =
        Wh_SetFunctionHook(targets[5], reinterpret_cast<void*>(uiLevel), reinterpret_cast<void**>(&setLevel)) &&
        Wh_SetFunctionHook(targets[7], reinterpret_cast<void*>(moveMouse), reinterpret_cast<void**>(&originalMoveMouse)) &&
        Wh_SetFunctionHook(reinterpret_cast<void*>(ClipCursor), reinterpret_cast<void*>(clipCursor), reinterpret_cast<void**>(&originalClipCursor)) &&
        Wh_SetFunctionHook(targets[0], reinterpret_cast<void*>(change), reinterpret_cast<void**>(&originalChange)) &&
        Wh_SetFunctionHook(targets[1], reinterpret_cast<void*>(option), reinterpret_cast<void**>(&originalOption)) &&
        Wh_SetFunctionHook(targets[2], reinterpret_cast<void*>(readOption), reinterpret_cast<void**>(&getOption)) &&
        (!(plan.capabilities & compatibility::NativeBootstrapRun) ||
            Wh_SetFunctionHook(targets[4], reinterpret_cast<void*>(run), reinterpret_cast<void**>(&originalRun))) &&
        (!(plan.capabilities & compatibility::NativeBootstrapWindows) ||
            (Wh_SetFunctionHook(targets[10], reinterpret_cast<void*>(uiWindowProc), reinterpret_cast<void**>(&originalUiWindowProc)) &&
             Wh_SetFunctionHook(targets[11], reinterpret_cast<void*>(fullWindowProc), reinterpret_cast<void**>(&originalFullWindowProc)))) &&
        (!(plan.capabilities & compatibility::NativeKeyboardObserver) ||
            Wh_SetFunctionHook(targets[6], reinterpret_cast<void*>(zoomCommand), reinterpret_cast<void**>(&originalZoomCommand))) &&
        (!(plan.capabilities & compatibility::NativeWheel) ||
            Wh_SetFunctionHook(targets[9], reinterpret_cast<void*>(wheel), reinterpret_cast<void**>(&originalWheel))) &&
        Wh_SetFunctionHook(reinterpret_cast<void*>(ShowWindow), reinterpret_cast<void*>(show), reinterpret_cast<void**>(&originalShow)) &&
        Wh_SetFunctionHook(reinterpret_cast<void*>(SetWindowPos), reinterpret_cast<void*>(position), reinterpret_cast<void**>(&originalPosition)) &&
        Wh_SetFunctionHook(reinterpret_cast<void*>(CreateWindowExW), reinterpret_cast<void*>(create), reinterpret_cast<void**>(&originalCreate)) &&
        Wh_SetFunctionHook(reinterpret_cast<void*>(SetWindowLongPtrW), reinterpret_cast<void*>(style), reinterpret_cast<void**>(&originalStyle));
    if (!installed) {
        InterlockedExchange(&mapping.get()->nativeCaps, 0);
        InterlockedExchange(&mapping.get()->resolverReasonNative, ERROR_GEN_FAILURE);
        return false;
    }
    installReadingHooks(module);
    available = true;
    capabilities = static_cast<LONG>(plan.capabilities);
    InterlockedExchange(&mapping.get()->nativeCaps, capabilities);
    InterlockedExchange(&mapping.get()->resolverReasonNative, ERROR_SUCCESS);
    InterlockedExchange(&mapping.get()->nativeError, ERROR_SUCCESS);
    Wh_Log(L"Native Magnifier capabilities: 0x%lx", capabilities);
    return true;
}
#endif
void afterInit() {
    if (!mapping.get()) return;
    InterlockedExchange(&mapping.get()->magnifierPid, static_cast<LONG>(GetCurrentProcessId()));
    if (!available) {
        // Publish that the real process exists, but never hide it, start a
        // controller worker or intercept controls without a compatible backend.
        InterlockedExchange(&mapping.get()->magnifierReady, 0);
        InterlockedExchange(&mapping.get()->nativeCaps, 0);
        InterlockedExchange(&mapping.get()->nativeError, ERROR_NOT_SUPPORTED);
        return;
    }
    // Ready is published by validated owner-thread ensureUi/change.
    // Do not overwrite a live UI publication from the injector thread.
    InterlockedIncrement(&mapping.get()->inputRevision);
    InterlockedIncrement(&mapping.get()->zoomRevision);
    InterlockedExchange(&mapping.get()->appliedSequence, mapping.get()->requestedSequence);
    InterlockedExchange(&mapping.get()->pendingCommand, 0);
    EnumWindows(visibility, 0);
    EnumWindows(wakeExistingTree, 0);
    stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (stopEvent) thread = CreateThread(nullptr, 0, worker, nullptr, 0, nullptr);
}
void stop() {
    if (!available) {
        if (mapping.get()) InterlockedExchange(&mapping.get()->magnifierReady, 0);
        return;
    }
    stopping.store(true);
    if (mapping.get()) InterlockedExchange(&mapping.get()->magnifierReady, 0);
    if (stopEvent) SetEvent(stopEvent);
    if (thread) { WaitForSingleObject(thread, INFINITE); CloseHandle(thread); }
    const HWND target = messageWindow.load();
    bool drained = !target;
    if (target && uiThread.load() == GetCurrentThreadId()) {
        const bool active = activeUpdates.load() != 0;
        drained = DestroyWindow(target) != FALSE && !active;
    }
    else if (target) {
        DWORD_PTR result = 0;
        // Destroy synchronously on its own thread. On return, its WndProc has
        // finished unless WM_CLOSE reports a reentrant update still on the stack.
        drained = SendMessageTimeoutW(target, WM_CLOSE, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 500, &result) != 0 && result == 0;
    }
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                      reinterpret_cast<LPCWSTR>(&messages), &module);
    if (drained) UnregisterClassW(kNativeClass, module);
    else {
        // A hung UI may still reach the WndProc. Keep its code mapped rather
        // than unloading an outstanding callback; Windows releases it at exit.
        HMODULE pinned = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                          reinterpret_cast<LPCWSTR>(&messages), &pinned);
    }
    if (stopEvent) CloseHandle(stopEvent);
    hiding.store(false);
    EnumWindows(visibility, 0);
}
}  // namespace magnifier

#if !defined(PINCH_ZOOM_NATIVE_TEST) || defined(PINCH_ZOOM_GATE_TEST)
namespace native_gate {

// ABI verified against Microsoft public symbols for Windows 11 build 26200.
// TOUCHPAD_GESTURE_CONTEXT is passed as one 8-byte value in the x64 ABI.
struct Context { std::uint64_t value; };
using Start = HRESULT(*)(void*, int, int, unsigned, Context);
using Continue = HRESULT(*)(void*, int, int, int, int, unsigned);
using Finish = HRESULT(*)(void*, int, int, int, int, unsigned);
using Cancel = HRESULT(*)(void*);
Start originalStart = nullptr;
Continue originalContinue = nullptr;
Finish originalFinish = nullptr;
Cancel originalCancel = nullptr;
bridge::Mapping mapping;
bool shell = false;
bool symbolUnsupported = false;
bool hooksAttempted = false;
LONG capabilities = 0;
std::mutex mutex;
bool activateWindow(int direction) { return runtime::activateWindow(direction); }
bool (*switchWindow)(int) = activateWindow;
decltype(&GetPointerFrameInfo) readPointerFrame = GetPointerFrameInfo;
struct State {
    pinch::GestureGate gate{pinch::Phase::Pending};
    bool started = false;
    bool cancelled = false;
    bool classify = false;
    bool mayClassify = true;
    bool direct = false;
    bool pending = false;
    int x = 0, y = 0;
    unsigned device = 0;
    Context context{};
};
std::map<void*, State> states;
void pendingChange(int delta) {
    if (delta) if (auto* record = mapping.get()) { InterlockedExchangeAdd(&record->pendingGestures, delta); mapping.wake(); }
}
bool nativeDeclined() {
    const auto* record = mapping.get();
    return record && compatibility::nativeDeclined(static_cast<std::uint32_t>(record->nativeCaps), record->resolverReasonNative);
}
bool directSwipe(pinch::Phase phase) {
    auto* record = mapping.get();
    return phase == pinch::Phase::Swipe && record && record->instantSwitch && std::abs(record->swipeDirection) == 1;
}
HRESULT start(void* self, int x, int y, unsigned device, Context context) {
    UINT32 contacts = 0;
    readPointerFrame(device, &contacts, nullptr);
    // Raw HID is authoritative for a three-finger pinch when the pointer-frame
    // query is temporarily empty. An explicit four-finger frame always bypasses.
    const bool classify = contacts < 4 && mapping.get() && mapping.get()->enabled && GetTickCount64() - mapping.get()->heartbeat < 250;
    const auto phase = classify && !nativeDeclined() ? mapping.phase() : pinch::Phase::Swipe;
    State state;
    state.x = x; state.y = y; state.device = device; state.context = context;
    state.classify = classify;
    state.mayClassify = contacts < 4;
    state.gate.update(phase);
    state.direct = classify && directSwipe(phase);
    state.started = !state.direct && state.gate.mode() != pinch::GateMode::Suppress && !(contacts == 0 && phase == pinch::Phase::Pending);
    state.pending = state.classify && state.gate.mode() == pinch::GateMode::Pending;
    {
        std::lock_guard lock(mutex);
        const auto previous = states.find(self);
        pendingChange((state.pending ? 1 : 0) - (previous != states.end() && previous->second.pending ? 1 : 0));
        states.insert_or_assign(self, state);
    }
    if (auto* record = mapping.get()) InterlockedExchange(&record->reserved,
        static_cast<LONG>(contacts | (static_cast<unsigned>(state.gate.mode()) << 8)));
    if (state.direct) { switchWindow(mapping.get()->swipeDirection); return S_OK; }
    // Start at the real beginning: delayed/replayed StartSwipe changes the
    // desktop/task-view state from which Windows builds its animation.
    const HRESULT result = state.started ? originalStart(self, x, y, device, context) : S_OK;
    if (FAILED(result)) {
        std::lock_guard lock(mutex);
        if (auto found = states.find(self); found != states.end()) {
            if (found->second.pending) pendingChange(-1);
            found->second.pending = false; found->second.started = false; found->second.cancelled = true;
        }
    }
    return result;
}
HRESULT continueSwipe(void* self, int x, int y, int dx, int dy, unsigned device) {
    UINT32 contacts = 0;
    readPointerFrame(device, &contacts, nullptr);
    State state;
    bool tracked = false, cancelNow = false, switchNow = false, endPending = false, beginNow = false, uncertain = false;
    int direction = 0;
    {
        std::lock_guard lock(mutex);
        if (const auto found = states.find(self); found != states.end()) {
            tracked = true;
            auto& current = found->second;
            if (current.cancelled) return S_OK; // A cancelled Windows gesture is terminal until Finish/Cancel.
            if (contacts >= 4) { current.classify = false; current.mayClassify = false; current.gate = pinch::GestureGate(pinch::Phase::Swipe); }
            if (!current.classify && current.mayClassify && mapping.threeContacts()) current.classify = true;
            const auto phase = current.classify && !nativeDeclined() ? mapping.phase() : pinch::Phase::Swipe;
            if (nativeDeclined()) current.gate = pinch::GestureGate(pinch::Phase::Swipe);
            current.gate.update(phase);
            uncertain = current.classify && phase == pinch::Phase::Pending;
            if (current.pending && current.gate.mode() != pinch::GateMode::Pending) { current.pending = false; endPending = true; }
            if (!current.direct && current.classify && directSwipe(phase)) {
                current.direct = true;
                switchNow = true;
                direction = mapping.get()->swipeDirection;
            }
            if ((current.direct || current.gate.mode() == pinch::GateMode::Suppress) && current.started && !current.cancelled) {
                current.cancelled = true;
                cancelNow = true;
            }
            if (!current.started && !current.direct && current.gate.mode() == pinch::GateMode::Forward && !uncertain) {
                current.started = true;
                beginNow = true;
            }
            state = current;
            if (auto* record = mapping.get()) {
                const LONG previous = InterlockedCompareExchange(&record->reserved, 0, 0);
                InterlockedExchange(&record->reserved, (previous & 0xFF) | (static_cast<LONG>(current.gate.mode()) << 8) | (current.direct ? 0x10000 : 0));
            }
        }
    }
    if (!tracked) return originalContinue(self, x, y, dx, dy, device);
    if (cancelNow) originalCancel(self);
    if (endPending) pendingChange(-1);
    if (switchNow) switchWindow(direction);
    if (beginNow) {
        const HRESULT result = originalStart(self, state.x, state.y, state.device, state.context);
        if (FAILED(result)) {
            std::lock_guard lock(mutex);
            if (auto found = states.find(self); found != states.end()) {
                if (found->second.pending) pendingChange(-1);
                found->second.pending = false; found->second.started = false; found->second.cancelled = true;
            }
            return result;
        }
    }
    if (uncertain) return S_OK;
    if (state.direct || state.gate.mode() != pinch::GateMode::Forward) return S_OK;
    const HRESULT result = originalContinue(self, x, y, dx, dy, device);
    if (FAILED(result)) {
        bool cancelFailed = false;
        {
            std::lock_guard lock(mutex);
            if (auto found = states.find(self); found != states.end() && !found->second.cancelled) {
                found->second.cancelled = true; cancelFailed = found->second.started;
                if (found->second.pending) pendingChange(-1);
                found->second.pending = false;
            }
        }
        if (cancelFailed) originalCancel(self);
    }
    return result;
}
HRESULT finish(void* self, int x, int y, int dx, int dy, unsigned device) {
    std::optional<State> state;
    {
        std::lock_guard lock(mutex);
        if (const auto found = states.find(self); found != states.end()) { state = found->second; states.erase(found); }
    }
    if (!state) return originalFinish(self, x, y, dx, dy, device);
    if (state->cancelled) { if (state->pending) pendingChange(-1); return S_OK; }
    const auto phase = state->classify && !nativeDeclined() ? mapping.phase(true) : pinch::Phase::Swipe;
    if (nativeDeclined()) state->gate = pinch::GestureGate(pinch::Phase::Swipe);
    if (state->direct || state->gate.finish(phase) == pinch::GateMode::Suppress) {
        if (state->started && !state->cancelled) originalCancel(self);
        if (state->pending) pendingChange(-1);
        return S_OK;
    }
    const HRESULT result = state->started ? originalFinish(self, x, y, dx, dy, device) : S_OK;
    if (state->pending) pendingChange(-1);
    return result;
}
HRESULT cancel(void* self) {
    std::optional<State> state;
    {
        std::lock_guard lock(mutex);
        if (const auto found = states.find(self); found != states.end()) { state = found->second; states.erase(found); }
    }
    const HRESULT result = !state || (state->started && !state->cancelled) ? originalCancel(self) : S_OK;
    if (state && state->pending) pendingChange(-1);
    return result;
}
using Visibility = void(*)(void*, int);
Visibility originalShown = nullptr, originalDismissed = nullptr;
void shown(void* self, int type) {
    if (type & 2) mapping.shellReason(1, true);
    originalShown(self, type);
}
void dismissed(void* self, int type) {
    originalDismissed(self, type);
    if (type & 2) mapping.shellReason(1, false);
}
#ifndef PINCH_ZOOM_GATE_TEST
bool initialize() {
    if (!mapping.open()) return false;
    symbolUnsupported = false;
    hooksAttempted = false;
    shell = false;
    capabilities = 0;
    bridge::desktopRaised = nullptr;
    const auto unsupported = [] {
        symbolUnsupported = true;
        InterlockedExchange(&mapping.get()->gateReady, 0);
        InterlockedExchange(&mapping.get()->gateCaps, 0);
        InterlockedExchange(&mapping.get()->resolverReasonGate, ERROR_NOT_SUPPORTED);
        return false;
    };
    using GetVersion = LONG(WINAPI*)(OSVERSIONINFOW*);
    const auto getVersion = reinterpret_cast<GetVersion>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
    OSVERSIONINFOW version{};
    version.dwOSVersionInfoSize = sizeof(version);
    if (!getVersion || getVersion(&version) != 0 || version.dwBuildNumber != 26200) {
        Wh_Log(L"Native gate currently supports Windows build 26200 only; public host remains available.");
        return unsupported();
    }
    HMODULE module = GetModuleHandleW(L"twinui.pcshell.dll");
    if (!module) module = LoadLibraryExW(L"twinui.pcshell.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    // Retain the module while Windhawk can hold hooks into this shell component.
    if (!module) { Wh_Log(L"Could not load System32 twinui.pcshell.dll: %lu", GetLastError()); return unsupported(); }
    const wchar_t* names[] = {
        L"MultitaskingViewGestureHandler::StartSwipe(",
        L"MultitaskingViewGestureHandler::ContinueSwipe(",
        L"MultitaskingViewGestureHandler::FinishSwipe(",
        L"MultitaskingViewGestureHandler::CancelSwipe(",
        L"MultitaskingViewVisibilityService::ViewShown(",
        L"MultitaskingViewVisibilityService::ViewDismissed("};
    void* targets[6]{};
    const bool profiled = binary_profiles::resolve(module, binary_profiles::Kind::Shell, targets, std::size(targets));
    WH_FIND_SYMBOL symbol{};
    if (!profiled) {
        const HANDLE search = Wh_FindFirstSymbol(module, nullptr, &symbol);
        if (!search) return unsupported();
        do {
            for (int i = 0; i < 6; ++i) {
                if (std::wcsstr(symbol.symbol, names[i])) targets[i] = symbol.address;
            }
        } while (Wh_FindNextSymbol(search, &symbol));
        Wh_FindCloseSymbol(search);
    }
    InterlockedExchange(&mapping.get()->resolverModeGate, profiled ? 1 : 2);
    for (auto& target : targets) if (target) {
        const auto offset = reinterpret_cast<ULONG_PTR>(target) - reinterpret_cast<ULONG_PTR>(module);
        if (offset > MAXDWORD || !binary_profiles::permitted(module, static_cast<DWORD>(offset), false)) target = nullptr;
    }
    std::uint32_t found = 0;
    for (unsigned i = 0; i < std::size(targets); ++i) if (targets[i]) found |= compatibility::symbol(i);
    // All four gesture entrypoints are required before registering any hook.
    if (!compatibility::gatePlan(found, false).supported) {
        Wh_Log(L"Native swipe gate unavailable; preserving native gestures while public host remains active.");
        return unsupported();
    }
    const HMODULE explorerModule = GetModuleHandleW(nullptr);
    void* desktop[1]{};
    const bool desktopProfiled = binary_profiles::resolve(explorerModule, binary_profiles::Kind::Desktop, desktop, 1);
    if (desktopProfiled) bridge::desktopRaised = static_cast<volatile LONG*>(desktop[0]);
    else {
        const HANDLE explorerSymbols = Wh_FindFirstSymbol(GetModuleHandleW(nullptr), nullptr, &symbol);
        if (explorerSymbols) {
            do {
                if (std::wcsstr(symbol.symbol, L"g_fDesktopRaised")) { bridge::desktopRaised = static_cast<volatile LONG*>(symbol.address); break; }
            } while (Wh_FindNextSymbol(explorerSymbols, &symbol));
            Wh_FindCloseSymbol(explorerSymbols);
        }
        if (bridge::desktopRaised) {
            const auto offset = reinterpret_cast<ULONG_PTR>(bridge::desktopRaised) - reinterpret_cast<ULONG_PTR>(explorerModule);
            if (offset > MAXDWORD || !binary_profiles::permitted(explorerModule, static_cast<DWORD>(offset), true)) bridge::desktopRaised = nullptr;
        }
    }
    InterlockedExchange(&mapping.get()->resolverModeDesktop, bridge::desktopRaised ? (desktopProfiled ? 1 : 2) : 0);
    const auto plan = compatibility::gatePlan(found, bridge::desktopRaised != nullptr);
    hooksAttempted = true;
    const bool installed =
        Wh_SetFunctionHook(targets[0], reinterpret_cast<void*>(start), reinterpret_cast<void**>(&originalStart)) &&
        Wh_SetFunctionHook(targets[1], reinterpret_cast<void*>(continueSwipe), reinterpret_cast<void**>(&originalContinue)) &&
        Wh_SetFunctionHook(targets[2], reinterpret_cast<void*>(finish), reinterpret_cast<void**>(&originalFinish)) &&
        Wh_SetFunctionHook(targets[3], reinterpret_cast<void*>(cancel), reinterpret_cast<void**>(&originalCancel)) &&
        (!(plan.capabilities & compatibility::GateVisibility) ||
            (Wh_SetFunctionHook(targets[4], reinterpret_cast<void*>(shown), reinterpret_cast<void**>(&originalShown)) &&
             Wh_SetFunctionHook(targets[5], reinterpret_cast<void*>(dismissed), reinterpret_cast<void**>(&originalDismissed))));
    if (!installed) {
        InterlockedExchange(&mapping.get()->gateCaps, 0);
        InterlockedExchange(&mapping.get()->resolverReasonGate, ERROR_GEN_FAILURE);
        return false; // Actual hook failure must abort; never continue partially.
    }
    shell = true;
    capabilities = static_cast<LONG>(plan.capabilities);
    InterlockedExchange(&mapping.get()->gateCaps, capabilities);
    InterlockedExchange(&mapping.get()->resolverReasonGate, ERROR_SUCCESS);
    return true;
}
void ready() {
    if (auto* record = mapping.get()) {
        InterlockedExchange(&record->shellPid, static_cast<LONG>(GetCurrentProcessId()));
        InterlockedExchange(&record->pendingGestures, 0); // A restarted Explorer has no old native gestures.
        mapping.shellReason(1, false);
        InterlockedExchange(&record->gateCaps, capabilities);
        InterlockedExchange(&record->gateReady, shell ? 1 : 0);
        Wh_Log(L"Native swipe gate ready=%d; capabilities=0x%lx", shell, capabilities);
    }
}
void stop() {
    if (auto* record = mapping.get()) InterlockedExchange(&record->gateReady, 0);
}
#endif  // PINCH_ZOOM_GATE_TEST
}  // namespace native_gate

#ifndef PINCH_ZOOM_GATE_TEST
namespace controller {
HANDLE stopEvent = nullptr;
HANDLE thread = nullptr;
HANDLE readyEvent = nullptr;
std::atomic<HWND> window{nullptr};
std::atomic<bool> started{false};

pinch::Config settings() {
    pinch::Config config;
    config.enabled = Wh_GetIntSetting(L"enabled") != 0;
    config.observeOnly = Wh_GetIntSetting(L"observeOnly") != 0;
    config.sensitivity = std::clamp(Wh_GetIntSetting(L"sensitivity"), 25, 500) / 100.0;
    config.acceleration = std::clamp(Wh_GetIntSetting(L"acceleration"), 0, 150) / 100.0;
    config.invertPinch = Wh_GetIntSetting(L"invertPinch") != 0;
    config.threshold = std::clamp(Wh_GetIntSetting(L"threshold"), 2, 30) / 100.0;
    config.maxZoom = pinch::maximumZoom(Wh_GetIntSetting(L"maxZoom"));
    config.keyboardInterop = Wh_GetIntSetting(L"keyboardInterop") != 0;
    config.hideMagnifier = Wh_GetIntSetting(L"hideMagnifier") != 0;
    config.suppressStartupFeedback = Wh_GetIntSetting(L"suppressStartupFeedback") != 0;
    config.instantHorizontalSwitch = Wh_GetIntSetting(L"instantHorizontalSwitch") != 0;
    config.normalZoomInOverview = Wh_GetIntSetting(L"normalZoomInOverview") != 0;
    config.smoothWheel = Wh_GetIntSetting(L"smoothWheel") != 0;
    config.winWheel = Wh_GetIntSetting(L"winWheel") != 0;
    config.wheelWindowSwitch = Wh_GetIntSetting(L"wheelWindowSwitch") != 0;
    config.windowButtons = Wh_GetIntSetting(L"windowButtons") != 0;
    config.disableReading = Wh_GetIntSetting(L"disableReading") != 0;
    config.wheelStep = std::clamp(Wh_GetIntSetting(L"wheelStep"), 1, 100);
    config.wheelDuration = std::clamp(Wh_GetIntSetting(L"wheelDuration"), 40, 250);
    config.wheelMaxRate = std::clamp(Wh_GetIntSetting(L"wheelMaxRate"), 50, 2000);
    config.keyboardStep = std::clamp(Wh_GetIntSetting(L"keyboardStep"), 0, 1000);
    config.keyboardHoldMultiplier = std::clamp(Wh_GetIntSetting(L"keyboardHoldMultiplier"), 1, 4);
    config.edgeMargin = std::clamp(Wh_GetIntSetting(L"edgeMargin"), 0, 40) / 100.0;
    const auto follow = Wh_GetStringSetting(L"mouseFollow");
    if (follow) {
        if (wcscmp(follow, L"proportional") == 0) config.mouseFollow = 1;
        else if (wcscmp(follow, L"fixed") == 0) config.mouseFollow = 2;
        Wh_FreeStringSetting(follow);
    }
    config.smoothingMs = std::clamp(Wh_GetIntSetting(L"smoothing"), 0, 250);
    config.fallbackAspect = std::clamp(Wh_GetIntSetting(L"fallbackHeight"), 20, 150) / 100.0;
    config.debug = Wh_GetIntSetting(L"debugLogging") != 0;
    return config;
}


DWORD WINAPI worker(void* parameter) {
    std::unique_ptr<pinch::Config> config(static_cast<pinch::Config*>(parameter));
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&worker), &module);
    runtime::Host host(*config);
    if (host.initialize(module)) {
        window.store(host.window());
        started.store(true);
    }
    SetEvent(readyEvent);
    if (started.load()) host.loop(stopEvent);
    window.store(nullptr);
    return 0;
}
void start() {
    stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    readyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!stopEvent || !readyEvent) { Wh_Log(L"Runtime event creation failed."); return; }
    auto config = std::make_unique<pinch::Config>(settings());
    thread = CreateThread(nullptr, 0, worker, config.get(), 0, nullptr);
    if (thread) config.release();
    if (!thread || WaitForSingleObject(readyEvent, 10000) != WAIT_OBJECT_0 || !started.load()) {
        Wh_Log(L"Pinch runtime could not start; native gestures remain available.");
    }
}
void stop() {
    if (stopEvent) SetEvent(stopEvent);
    if (thread) { WaitForSingleObject(thread, INFINITE); CloseHandle(thread); }
    if (readyEvent) CloseHandle(readyEvent);
    if (stopEvent) CloseHandle(stopEvent);
}
}  // namespace controller

BOOL Wh_ModInit() {
    DWORD session = 0;
    if (!ProcessIdToSessionId(GetCurrentProcessId(), &session) || !session) return FALSE;
    wchar_t path[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, path, ARRAYSIZE(path))) return FALSE;
    const wchar_t* leaf = wcsrchr(path, L'\\');
    if (_wcsicmp(leaf ? leaf + 1 : path, L"magnify.exe") == 0) {
        using GetVersion = LONG(WINAPI*)(OSVERSIONINFOW*);
        const auto getVersion = reinterpret_cast<GetVersion>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
        OSVERSIONINFOW version{};
        version.dwOSVersionInfoSize = sizeof(version);
        const bool supportedBuild = getVersion && getVersion(&version) == 0 && version.dwBuildNumber == 26200;
        if (magnifier::initialize(supportedBuild)) return TRUE;
        return magnifier::symbolUnsupported && !magnifier::hooksAttempted;
    }
    // Folder windows can run in secondary Explorer processes. Only the actual
    // shell may own the shared gesture controller and its reset hotkeys.
    DWORD shellOwner = 0;
    if (const HWND shellWindow = GetShellWindow()) GetWindowThreadProcessId(shellWindow, &shellOwner);
    if (shellOwner && shellOwner != GetCurrentProcessId()) return FALSE;
    if (!shellOwner) {
        std::wstring command = GetCommandLineW();
        CharLowerBuffW(command.data(), static_cast<DWORD>(command.size()));
        if (command.find(L"/factory") != std::wstring::npos || command.find(L"-embedding") != std::wstring::npos) return FALSE;
    }
    if (native_gate::initialize()) return TRUE;
    // Missing/unverified symbols are a supported degraded mode. A real hook
    // failure must return FALSE so Windhawk can discard all registered hooks.
    return native_gate::symbolUnsupported && !native_gate::hooksAttempted;
}
void Wh_ModAfterInit() {
    if (magnifier::role) { magnifier::afterInit(); return; }
    native_gate::ready();
    controller::start();
}
BOOL Wh_ModSettingsChanged(BOOL* reload) {
    *reload = FALSE;
    if (magnifier::role) {
        if (!magnifier::available) return TRUE;
        if (const HWND target = magnifier::messageWindow.load()) PostMessageW(target, magnifier::kCancelKeyboardHold, 0, 0);
        magnifier::hiding.store(Wh_GetIntSetting(L"hideMagnifier") != 0 &&
            (magnifier::mapping.get()->magnifierReady || (magnifier::capabilities & compatibility::NativeBootstrapWindows)));
        magnifier::keepPointerStable.store(Wh_GetIntSetting(L"keepPointerStable") != 0);
        EnumWindows(magnifier::visibility, 0);
        return TRUE;
    }
    if (const HWND window = controller::window.load()) {
        const auto config = controller::settings();
        SendMessageW(window, runtime::kSettingsMessage, 0, reinterpret_cast<LPARAM>(&config));
    }
    return TRUE;
}
void Wh_ModUninit() {
    if (magnifier::role) { magnifier::stop(); return; }
    native_gate::stop();
    controller::stop();
}

#endif  // PINCH_ZOOM_GATE_TEST
#endif  // PINCH_ZOOM_NATIVE_TEST
#endif  // Native adapter or Windhawk
#endif  // PINCH_ZOOM_CORE_TEST
