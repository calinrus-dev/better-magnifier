// ==WindhawkMod==
// @id              kde-pinch-zoom
// @name            Better Visual
// @name:es-ES      Better Visual
// @description     Smooth desktop zoom and system-aware dark mode for Windows dialogs
// @description:es-ES Zoom fluido del escritorio y tema oscuro para diálogos de Windows
// @version         1.2.0
// @author          calinrus-dev
// @github          https://github.com/calinrus-dev
// @homepage        https://github.com/calinrus-dev/better-magnifier
// @include         explorer.exe
// @include         magnify.exe
// @include         control.exe
// @include         rundll32.exe
// @include         mmc.exe
// @include         regedit.exe
// @include         OptionalFeatures.exe
// @include         SystemPropertiesAdvanced.exe
// @include         SystemPropertiesComputerName.exe
// @include         SystemPropertiesHardware.exe
// @include         SystemPropertiesPerformance.exe
// @include         SystemPropertiesProtection.exe
// @include         SystemPropertiesRemote.exe
// @architecture    amd64
// @compilerOptions -DNOMINMAX -static -lhid -lwtsapi32 -lshell32 -ladvapi32 -ldwmapi -lole32 -lgdi32
// @license         GPL-3.0-or-later
// ==/WindhawkMod==

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 calinrus-dev
// Raw HID handling was informed by Three Finger Drag by
// Rodrigo Thomazi (GPL-3.0). See THIRD_PARTY.md.

// ==WindhawkModReadme==
/*
# Better Visual

Spread three fingers to magnify the desktop; pinch them together to zoom out.
A red screen outline indicates that mouse zoom controls are active. A
three-finger pinch keeps this mode active after release while zoomed in.
Returning to 100% by pinching turns the mode off. The wheel stays armed at
100% so you can reverse direction. Press both mouse buttons together to
toggle it without changing the current scale. In this mode the main wheel
zooms, its click resets, and the side wheel scrolls vertically. Win+plus/minus
also works with the numeric keypad. Use WGC to share at normal scale.

The optional dark theme applies to supported Windows property and Control
Panel dialogs. It follows Windows app mode and respects contrast themes.
Windows renders its menus.
Ctrl+Alt+wheel switches windows. The mouse profile is configurable.
Two-finger app zoom and Windows three/four-finger swipe settings are untouched.
Pinch zoom requires a Precision Touchpad and compatible Windows binaries.
Verified Windows 11 25H2 binaries work offline. A companion in
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
  $name: Desktop zoom core
  $name:es-ES: Núcleo de zoom del escritorio
  $description: Runs the Windows Magnifier integration and keyboard zoom. Touchpad and mouse controls can be selected separately.
- disableTouchpad: false
  $name: Disable touchpad controls
  $name:es-ES: Desactivar controles del touchpad
  $description: Turns off the three-finger pinch and window switcher using Better Visual. Windows manages touchpad gestures.
- disableMouse: false
  $name: Disable mouse controls
  $name:es-ES: Desactivar controles del ratón
  $description: Turns off Better Visual's wheel, button and window shortcuts. Touchpad and keyboard zoom remain available.
- disableMouseChord: false
  $name: Disable left/right click zoom toggle
  $name:es-ES: Desactivar acorde de clic izquierdo y derecho
  $description: Passes ordinary clicks directly to applications while keeping wheel zoom available.
- darkLegacyWindows: true
  $name: Dark mode for classic Windows dialogs
  $name:es-ES: Modo oscuro para diálogos clásicos de Windows
  $description: Follows Windows app mode for supported dialogs and respects contrast themes.
  $description:es-ES: Sigue el modo de aplicaciones de Windows en los diálogos compatibles y respeta los temas de contraste.
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
  $description: Uses continuous native zoom. The wheel scrolls normally with zoom mode off unless you hold the zoom modifier.
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
  $description: Relative change per 120 wheel units. High-resolution deltas adjust the zoom directly and retain their precision.
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
- hideZoomBorder: false
  $name: Hide the active zoom border
  $name:es-ES: Ocultar marco del zoom
  $description: Hides the frame while leaving zoom active.
- zoomBorderColor: red
  $name: Active zoom border color
  $name:es-ES: Color del marco del zoom
  $options:
  - red: Red
  - blue: Blue
  - yellow: Yellow
  $options:es-ES:
  - red: Rojo
  - blue: Azul
  - yellow: Amarillo
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
#include <span>
#include <utility>
#include <vector>

namespace pinch {

constexpr std::size_t kMaxContacts = 32;
constexpr std::uint64_t kPinchFrameTimeoutMs = 150;
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
    std::array<Contact, kMaxContacts> contacts_{};
    std::size_t size_ = 0;
    std::uint64_t started_ = 0;

public:
    void reset() {
        expected_ = 0;
        scan_.reset();
        size_ = 0;
    }
    // The returned view remains valid until the next call to feed.
    std::optional<std::span<const Contact>> feed(const Report& report, std::uint64_t now) {
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
                return std::span<const Contact>{};
            }
            return std::nullopt;
        }
        for (const auto& contact : report.contacts) {
            if (size_ == expected_) {
                break;  // Remaining descriptor slots are padding.
            }
            contacts_[size_++] = contact;
        }
        if (size_ < expected_) {
            return std::nullopt;
        }
        const std::span<const Contact> result{contacts_.data(), size_};
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
    bool touchpadControls = true;
    bool mouseControls = true;
    bool mouseChord = true;
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
    bool instantHorizontalSwitch = true;
    bool normalZoomInOverview = true;
    int keyboardStep = 0;
    int keyboardHoldMultiplier = 2;
    int mouseFollow = 0;  // edges, proportional, fixed
    double edgeMargin = 0.12;

    bool managesZoom() const { return enabled && !observeOnly && keyboardInterop; }
    bool mouseWheelEnabled() const { return mouseControls && smoothWheel; }
    bool operator==(const Config&) const = default;
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
    bool beganEnlarging = false;
};

inline double wheelZoomLevel(double base, int delta, int step, double maximum) {
    const double cap = std::clamp(maximum, 1.0, 16.0);
    const double upper = std::log(cap);
    const double notch = std::log1p(std::clamp(step, 1, 100) / 100.0);
    const double target = std::log(std::min(base, cap)) + delta / 120.0 * notch;
    return target <= 0 ? 1 : target >= upper ? cap : std::exp(target);
}

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
    std::optional<double> wheelZoom(int delta, int step, double maximum) {
        const auto previous = std::llround(zoom_ * 10000);
        zoom_ = wheelZoomLevel(zoom_, delta, step, maximum);
        return std::llround(zoom_ * 10000) != previous ? std::optional{zoom_} : std::nullopt;
    }
    Decision update(std::span<const Contact> frame, std::uint64_t now,
                    double currentZoom, const Config& config, double elapsedMs = -1) {
        std::array<Contact, 3> active{};
        std::size_t count = 0;
        for (const auto& contact : frame) {
            if (!contact.down) continue;
            if (count == active.size() || !contact.confident ||
                !std::isfinite(contact.point.x) || !std::isfinite(contact.point.y)) {
                reset();
                return {};
            }
            active[count++] = contact;
        }
        if (count != active.size()) {
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
            const double dx = c.point.x - center.x;
            const double dy = c.point.y - center.y;
            radius += dx * dx + dy * dy;
        }
        radius = std::sqrt(radius / 3);
        if (phase_ == Phase::Idle || ids != ids_ || now - lastTime_ > kPinchFrameTimeoutMs) {
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
        if (scale <= 0 || !std::isfinite(scale)) {
            phase_ = Phase::Rejected;
            return {phase_, std::nullopt, false};
        }
        const double scaleDelta = std::log(scale / previousScale_);
        if (std::abs(scaleDelta) > 0.5) {
            phase_ = Phase::Rejected;
            return {phase_, std::nullopt, false};
        }
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
        return {phase_, zoom_, began, began && ((scale > 1) != config.invertPinch)};
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
// Wheel units determine the full travel in logarithmic zoom space. A short
// interpolation hides the physical notches without changing the final level.
class WheelMotion {
    static constexpr double kDurationMs = 32;
    std::optional<double> goal_;
    double lastSubmitted_ = 1;
    double fromLog_ = 0, toLog_ = 0, startedMs_ = 0;
    bool active_ = false, retarget_ = false;
public:
    bool active() const { return active_; }
    double goal() const { return goal_.value_or(1); }
    void add(int delta, double current, int step, double maximum) {
        if (!delta || !std::isfinite(current) || current < 1) return;
        if (!goal_) lastSubmitted_ = current;
        const double next = wheelZoomLevel(goal_.value_or(current), delta, step, maximum);
        if (goal_ && next == *goal_) return;
        goal_ = next;
        active_ = next != lastSubmitted_;
        retarget_ = active_;
    }
    std::optional<double> sample(double nowMs, double frameMs = 10) {
        if (!active_) return std::nullopt;
        if (std::exchange(retarget_, false)) {
            fromLog_ = std::log(lastSubmitted_);
            toLog_ = std::log(*goal_);
            startedMs_ = nowMs;
        }
        const double time = std::clamp((nowMs - startedMs_) / kDurationMs, 0.0, 1.0);
        if (time >= 1) return goal_;
        const double first = std::clamp(frameMs / kDurationMs, 0.14, 0.34);
        const double progress = std::lerp(first, 1.0, time);
        return std::exp(std::lerp(fromLog_, toLog_, progress));
    }
    void submitted(double level) {
        lastSubmitted_ = level;
        if (level == *goal_) active_ = false;
    }
    void cancel() { goal_.reset(); active_ = retarget_ = false; }
};
// Revision and delta share a CAS packet. Zoom combines all signed wheel units
// within a frame; window switching keeps a separate, bounded queue.
constexpr int kWindowWheelPacketLimit = 1200;
inline std::uint64_t wheelPacket(int delta, std::uint32_t revision) {
    return (static_cast<std::uint64_t>(revision) << 32) | static_cast<std::uint32_t>(delta);
}
inline int wheelDelta(std::uint64_t packet) { return static_cast<std::int32_t>(static_cast<std::uint32_t>(packet)); }
inline std::uint32_t wheelRevision(std::uint64_t packet) { return static_cast<std::uint32_t>(packet >> 32); }
inline std::uint64_t mergeZoomWheel(std::uint64_t previous, int delta, std::uint32_t revision) {
    const int old = wheelRevision(previous) == revision ? wheelDelta(previous) : 0;
    const auto total = static_cast<std::int64_t>(old) + delta;
    return wheelPacket(static_cast<int>(std::clamp<std::int64_t>(total,
        std::numeric_limits<int>::min(), std::numeric_limits<int>::max())), revision);
}
inline std::uint64_t mergeWheel(std::uint64_t previous, int delta, std::uint32_t revision) {
    const int old = wheelRevision(previous) == revision ? wheelDelta(previous) : 0;
    const bool sameDirection = (old < 0) == (delta < 0);
    const auto total = static_cast<std::int64_t>(delta) + (sameDirection ? old : 0);
    return wheelPacket(static_cast<int>(std::clamp<std::int64_t>(total, -kWindowWheelPacketLimit, kWindowWheelPacketLimit)), revision);
}
// Stable order across repeated direct swipes, even though activation changes z-order.
class WindowCycle {
    std::vector<std::uintptr_t> order_;
    std::uintptr_t last_ = 0;
public:
    std::uintptr_t next(const std::vector<std::uintptr_t>& available, std::uintptr_t current, int step) {
        if (current != last_) { order_ = available; last_ = current; }
        else {
            std::erase_if(order_, [&](auto window) { return std::find(available.begin(), available.end(), window) == available.end(); });
            for (auto window : available) if (std::find(order_.begin(), order_.end(), window) == order_.end()) order_.push_back(window);
        }
        if (order_.size() < 2) return 0;
        const auto found = std::find(order_.begin(), order_.end(), current);
        const int size = static_cast<int>(order_.size());
        const int index = found == order_.end() ? (step > 0 ? -1 : 0) : static_cast<int>(found - order_.begin());
        return order_[static_cast<std::size_t>((index + step + size) % size)];
    }
    // Selecting a candidate is not activation. Failed or deferred attempts must
    // not change the cycle's acknowledged foreground window.
    void commit(std::uintptr_t window) { if (window) last_ = window; }
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
    bool maskIssued_ = false, mouseUsed_ = false;
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
            if (!wasWin && winDown()) { keyboardUsed_ = otherHeld(); maskIssued_ = false; mouseUsed_ = false; }
            else if (down) keyboardUsed_ = otherHeld(); // Repeated Win DOWN can re-arm Start.
            else if (!winDown()) { keyboardUsed_ = false; maskIssued_ = false; mouseUsed_ = false; }
        } else if (wasWin && down) keyboardUsed_ = true;
    }
    bool winDown() const { return held(leftWin) || held(rightWin); }
    bool winOnly() const { return winDown() && !otherHeld(); }
    bool idle() const { return !winDown() && !otherHeld(); }
    bool mouseUnmodified() const {
        // Generic Shift/Ctrl/Alt and their left/right-specific virtual keys.
        return !winDown() && !held(0x10) && !held(0x11) && !held(0x12) &&
               !held(0xa0) && !held(0xa1) && !held(0xa2) && !held(0xa3) && !held(0xa4) && !held(0xa5);
    }
    bool consume() {
        if (!winOnly()) return false;
        if (keyboardUsed_ || maskIssued_) return false;
        maskIssued_ = true;
        return true;
    }
    void consumedMouse() { if (winOnly()) mouseUsed_ = true; }
    bool maskRelease(unsigned vk) const {
        return (vk == leftWin || vk == rightWin) && held(vk) && mouseUsed_ && !keyboardUsed_;
    }
    void maskFailed() { maskIssued_ = false; mouseUsed_ = false; }
    void reset() { down_.fill(0); keyboardUsed_ = false; maskIssued_ = false; mouseUsed_ = false; }
};

// Window switching is discrete even for free-spinning/high-resolution wheels.
// A bounded queue preserves complete notches until foreground confirmation.
// One step is dispatched per frame; stale bursts expire rather than replay.
class WindowWheel {
    int residual_ = 0, queued_ = 0, direction_ = 0;
    std::uint64_t lastInput_ = 0, generation_ = 0;
    bool haveInput_ = false;
    void expire(std::uint64_t now) { if (haveInput_ && (now < lastInput_ || now - lastInput_ > 200)) reset(); }
public:
    void reset() { residual_ = queued_ = direction_ = 0; lastInput_ = 0; haveInput_ = false; ++generation_; }
    void offer(int delta, std::uint64_t now) {
        if (!delta) return;
        expire(now);
        const int direction = delta > 0 ? -1 : 1;
        if (direction_ && direction != direction_) reset();
        direction_ = direction; lastInput_ = now; haveInput_ = true;
        const auto total = static_cast<std::int64_t>(residual_) + delta;
        const auto steps = -(total / 120);
        residual_ = static_cast<int>(total % 120);
        queued_ = static_cast<int>(std::clamp<std::int64_t>(queued_ + steps, -6, 6));
    }
    int front(std::uint64_t now) { expire(now); return queued_ < 0 ? -1 : queued_ > 0 ? 1 : 0; }
    int pending(std::uint64_t now) { expire(now); return queued_; }
    std::uint64_t generation() const { return generation_; }
    void pop() { if (queued_) queued_ += queued_ < 0 ? 1 : -1; }
    std::optional<int> feed(int delta, std::uint64_t now) {
        offer(delta, now); const int direction = front(now);
        if (!direction) return std::nullopt;
        pop(); return direction;
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
    NativeBootstrapWindows = 32, NativeHideUi = 64, NativePointerGuard = 128,
    NativeUiLevel = 256
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
                              symbol(3) | symbol(5);
    constexpr auto windows = symbol(10) | symbol(11);
    if ((found & required) != required ||
        (!(found & symbol(4)) && (found & windows) != windows)) return {};
    std::uint32_t capabilities = NativeCore | NativeHideUi;
    if (found & symbol(4)) capabilities |= NativeBootstrapRun;
    if ((found & windows) == windows) capabilities |= NativeBootstrapWindows;
    if (found & symbol(6)) capabilities |= NativeKeyboardObserver;
    if (found & symbol(8)) capabilities |= NativeViewportRestore;
    if (found & symbol(9)) capabilities |= NativeWheel;
    if (found & symbol(7)) capabilities |= NativePointerGuard;
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
constexpr bool symbolAbiVerifiedOn(std::uint32_t build) { return build == 26200; }
constexpr bool nativeDeclined(std::uint32_t capabilities, long reason) {
    return reason != 0 && !(capabilities & NativeCore);
}
constexpr bool mouseZoomAllowed(std::uint32_t capabilities, long reason, long error) {
    return !nativeDeclined(capabilities, reason) && error != 50; // ERROR_NOT_SUPPORTED, including lens/docked.
}
constexpr bool overviewAllowed(std::uint32_t native, std::uint32_t gate) {
    return !(native & NativeUiLevel) && (native & NativeViewportRestore) &&
           (native & NativeKeyboardObserver) && (gate & GateVisibility);
}
}  // namespace compatibility

#ifndef PINCH_ZOOM_CORE_TEST

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11_1.h>
#include <dcomp.h>
#include <hidsdi.h>
#include <dwmapi.h>
#include <hidpi.h>
#include <shellapi.h>
#include <objbase.h>
#include <servprov.h>
#include <new>
#include <wtsapi32.h>
#include <wrl/client.h>
#include <atomic>
#include <cstdio>
#include <cwchar>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#ifndef PINCH_ZOOM_STANDALONE
#include <windhawk_utils.h>
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
constexpr Identity magnify9949{{0xc5,0x46,0x12,0xd6,0x1f,0x4c,0x82,0xa8,
    0x21,0x05,0xd4,0x0b,0xd8,0xf3,0x63,0xde}, 1, 0x14c000, 0x88b3d475};
struct Profile {
    Kind kind;
    Identity identity;
    std::array<DWORD, 12> rvas{};
    std::size_t count;
};
constexpr Profile profiles[] = {
    {Kind::Magnifier, {{0x99,0x46,0x73,0xd3,0xf7,0xad,0x69,0xbc,0x0b,0x62,0xa7,0xf4,0x56,0x60,0x23,0x4f},1,0x130000,0x5a1425a3}, {0x258ac,0x78108,0x77a7c,0x77b3c,0x547b0,0x52d3c,0x53330,0x1c2e8,0x2852c,0x52aa4,0x55a20,0x2a520}, 12},
    // Magnify.exe 10.0.26100.9949: matching Microsoft PDB, both static window procedures verified against GWLP_USERDATA.
    {Kind::Magnifier, magnify9949, {0x2605c,0x7bdf8,0x7b76c,0x7b82c,0x57750,0x55d48,0x56360,0x1bae8,0x29498,0x55ac4,0x58b20,0x2b460}, 12},
    {Kind::Shell, {{0xda,0xf3,0x07,0x95,0x7f,0xd4,0x02,0x5f,0x65,0x69,0xfc,0x50,0x27,0x3c,0x40,0xd7},1,0x9a5000,0xbc86bb4d}, {0x16bec0,0x16c2c0,0x16bde0,0x4d5ee0,0x2198b0,0x216860}, 6},
    {Kind::Desktop, {{0x27,0x6f,0x66,0x1d,0x2f,0xba,0x88,0x85,0x58,0x88,0xd3,0xe2,0xb9,0x4b,0x0f,0xd8},1,0x344000,0xbee7e13f}, {0x2839dc}, 1},
    // KB5132061. Matching Microsoft public PDBs establish the shell entry points and desktop int.
    {Kind::Shell, {{0xb1,0x64,0x02,0x0c,0x91,0xdc,0x0a,0x4a,0xcf,0x98,0xda,0x9b,0xd6,0xf2,0x02,0x29},1,0x9d3000,0x59290a19}, {0x1dc0b0,0x1dc3b0,0x1dbfd0,0x4f9250,0x2593f0,0x2549c0}, 6},
    {Kind::Desktop, {{0xeb,0x18,0x23,0x26,0x1b,0xdb,0x78,0x15,0x04,0x13,0x22,0xf5,0x6c,0x2f,0xf6,0x99},1,0x328000,0xc21d280b}, {0x2677d8}, 1},
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
#ifndef PINCH_ZOOM_STANDALONE
bool canUseSymbols() {
    using GetVersion = LONG(WINAPI*)(OSVERSIONINFOW*);
    const auto getVersion = reinterpret_cast<GetVersion>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
    OSVERSIONINFOW version{};
    version.dwOSVersionInfoSize = sizeof(version);
    return getVersion && getVersion(&version) == 0 && compatibility::symbolAbiVerifiedOn(version.dwBuildNumber);
}

template<std::size_t N>
bool resolveSymbols(HMODULE module, const wchar_t* const (&names)[N], void* (&targets)[N], bool data = false) {
    std::fill_n(targets, N, nullptr);
    if (!identify(module)) return false;
    std::array<WindhawkUtils::SYMBOL_HOOK, N> lookups;
    for (std::size_t i = 0; i < N; ++i) {
        lookups[i] = WindhawkUtils::SYMBOL_HOOK({names[i]}, &targets[i], nullptr, true);
    }
    // Resolve addresses only, using the local Windhawk cache or Microsoft PDBs.
    // Capability groups decide which complete sets are safe to hook.
    WH_HOOK_SYMBOLS_OPTIONS options{sizeof(options), nullptr, TRUE, L""};
    if (!WindhawkUtils::HookSymbols(module, lookups.data(), lookups.size(), &options)) {
        std::fill_n(targets, N, nullptr);
        return false;
    }
    for (auto& target : targets) if (target) {
        const auto offset = reinterpret_cast<ULONG_PTR>(target) - reinterpret_cast<ULONG_PTR>(module);
        if (offset > MAXDWORD || !permitted(module, static_cast<DWORD>(offset), data)) target = nullptr;
    }
    return std::any_of(std::begin(targets), std::end(targets), [](void* target) { return target != nullptr; });
}
#endif
} // namespace binary_profiles

namespace bridge {
enum class InputSource : LONG { None = 0, Pinch = 1, Wheel = 2, Keyboard = 3, Reset = 4, PinchWheel = 5 };
inline bool pinchSource(InputSource source) { return source == InputSource::Pinch || source == InputSource::PinchWheel; }
inline bool wheelSource(InputSource source) { return source == InputSource::Wheel || source == InputSource::PinchWheel; }
inline volatile LONG* desktopRaised = nullptr;
constexpr wchar_t kMapping[] = L"Local\\KdePinchZoom.NativeGate.v18";
constexpr wchar_t kWake[] = L"Local\\KdePinchZoom.NativeWake.v18";
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
    volatile LONG menuMaskInitialSent;
    volatile LONG menuMaskInitialFailed;
    volatile LONG menuMaskReleaseSent;
    volatile LONG windowSwitchAttempts;
    volatile LONG windowSwitchConfirmed;
    volatile LONG windowSwitchRejected;
    volatile LONG windowSwitchDeferredFailed;
    volatile LONG zoomModeActive;
    volatile LONG requestWriting;
    volatile LONG64 pinchFrameTime;
};
inline bool pinchDown(const Record& record) {
    return record.down && record.phase == static_cast<LONG>(pinch::Phase::Pinch);
}
inline bool pendingLevel(const Record& record) {
    return record.requestedSequence != record.appliedSequence && record.requestInputRevision == record.inputRevision &&
           record.requestRevision == record.zoomRevision;
}
inline bool pinchFrameFresh(Record& record, std::uint64_t now) {
    const auto captured = static_cast<std::uint64_t>(InterlockedCompareExchange64(&record.pinchFrameTime, 0, 0));
    return captured && (captured > now || now - captured <= pinch::kPinchFrameTimeoutMs);
}
// The mouse broker and native UI can produce wheel input. The host consumes one bounded
// packet per frame; no per-delta posted messages can starve WM_TIMER.
inline bool enqueueWheel(Record* record, int delta, LONG revision, std::uint64_t now) {
    if (!record || !delta || InterlockedCompareExchange(&record->inputRevision, 0, 0) != revision) return false;
    const auto previousTime = static_cast<std::uint64_t>(InterlockedExchange64(&record->wheelTime, static_cast<LONG64>(now)));
    const auto idle = now >= previousTime ? now - previousTime : 0;
    LONG64 observed = InterlockedCompareExchange64(&record->wheelPacket, 0, 0);
    for (;;) {
        if (InterlockedCompareExchange(&record->inputRevision, 0, 0) != revision) return false;
        const auto merged = pinch::mergeZoomWheel(idle > 200 ? 0 : static_cast<std::uint64_t>(observed),
                                                 delta, static_cast<std::uint32_t>(revision));
        const auto desired = static_cast<LONG64>(merged);
        const LONG64 actual = InterlockedCompareExchange64(&record->wheelPacket, desired, observed);
        if (actual == observed) return !previousTime || idle >= 32;
        observed = actual;
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
        if (!mapping_) return false;
        record_ = static_cast<Record*>(MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Record)));
        if (!record_) {
            CloseHandle(std::exchange(mapping_, nullptr));
            return false;
        }
        wake_ = CreateEventW(nullptr, FALSE, FALSE, kWake); // Native worker polls if the event is unavailable.
        return true;
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
        const bool zoomEnabled = config.managesZoom();
        const bool mouseEnabled = config.mouseControls && zoomEnabled;
        InterlockedExchange(&record_->controlEnabled, zoomEnabled);
        InterlockedExchange(&record_->wheelEnabled, mouseEnabled && config.smoothWheel);
        InterlockedExchange(&record_->hideUi, config.hideMagnifier ? 1 : 0);
        InterlockedExchange(&record_->winWheel, mouseEnabled && config.winWheel);
        InterlockedExchange(&record_->wheelWindowSwitch, mouseEnabled && config.wheelWindowSwitch);
        InterlockedExchange(&record_->disableReading, mouseEnabled && config.disableReading);
        InterlockedExchange(&record_->instantSwitch, config.touchpadControls && zoomEnabled && config.instantHorizontalSwitch);
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
        InterlockedExchange(&record_->requestWriting, 1);
        InterlockedExchange(&record_->requestRevision, revision);
        InterlockedExchange(&record_->requestedLevel, static_cast<LONG>(std::lround(std::clamp(level, 1.0, 16.0) * 10000)));
        InterlockedExchange(&record_->requestSource, static_cast<LONG>(source));
        InterlockedExchange(&record_->requestInputRevision, InterlockedCompareExchange(&record_->inputRevision, 0, 0));
        InterlockedIncrement(&record_->requestedSequence);
        InterlockedExchange(&record_->requestWriting, 0);
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
    void publish(pinch::Phase phase, bool down, bool enabled, std::uint64_t captured = 0) {
        if (!record_) return;
        InterlockedExchange64(&record_->pinchFrameTime,
                              down && phase == pinch::Phase::Pinch ? static_cast<LONG64>(captured) : 0);
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
constexpr WPARAM kToggleZoomAction = 4;
constexpr WPARAM kExitZoomAction = 5;
constexpr WPARAM kResetArmedZoomAction = 6;
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

// Keep the named object alive while this process owns the session role.
// No thread owns the mutex itself, so startup and shutdown can differ.
class SessionLease {
    HANDLE handle_ = nullptr;
public:
    SessionLease() = default;
    SessionLease(const SessionLease&) = delete;
    SessionLease& operator=(const SessionLease&) = delete;
    bool acquire(const wchar_t* name) {
        if (handle_) return true;
        SetLastError(ERROR_SUCCESS);
        const HANDLE candidate = CreateMutexW(nullptr, FALSE, name);
        if (!candidate) return false;
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(candidate);
            return false;
        }
        handle_ = candidate;
        return true;
    }
    void release() {
        if (const auto handle = std::exchange(handle_, nullptr)) CloseHandle(handle);
    }
    ~SessionLease() { release(); }
};

// A shared color pixel is stretched by DirectComposition. Its transform stays
// fractional when Windows applies fullscreen magnification to the desktop.
class ZoomBorder {
public:
    struct Edge {
        RECT bounds{};
        D2D_MATRIX_3X2_F transform{};
    };
    enum class Update : WPARAM { Transform, Display, Device };
    enum class Color : std::uint8_t { Red, Blue, Yellow };
    using ReadMonitorTransform = BOOL(WINAPI*)(HMONITOR, double*, int*, int*);
    struct Appearance {
        bool hidden = false;
        Color color = Color::Red;
        bool operator==(const Appearance&) const = default;
    };

    static Color colorFromSetting(const wchar_t* value) {
        if (value && !_wcsicmp(value, L"blue")) return Color::Blue;
        if (value && !_wcsicmp(value, L"yellow")) return Color::Yellow;
        return Color::Red;
    }

    static constexpr std::array<float, 4> pixelColor(Color color) {
        switch (color) {
            case Color::Blue: return {0, 140.0f / 255, 1, 1};
            case Color::Yellow: return {1, 221.0f / 255, 0, 1};
            default: return {1, 0, 0, 1};
        }
    }

private:
    using CreateInBand = HWND(WINAPI*)(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int,
                                       HWND, HMENU, HINSTANCE, LPVOID, DWORD);
    using ReadBand = BOOL(WINAPI*)(HWND, DWORD*);
    using ReadTransform = BOOL(WINAPI*)(float*, int*, int*);
    static constexpr wchar_t kClassName[] = L"BetterMagnifier.ZoomBorder";

    struct Projection {
        pinch::View view;
        bool monitorLocal;
        bool operator==(const Projection& other) const {
            return view.zoom == other.view.zoom && view.x == other.view.x && view.y == other.view.y &&
                   monitorLocal == other.monitorLocal;
        }
    };
    struct Monitor {
        HMONITOR handle;
        RECT bounds;
        std::optional<pinch::View> sampled;
        std::optional<Projection> projection;
    };
    struct Strip {
        D2D_MATRIX_3X2_F transform;
        Microsoft::WRL::ComPtr<IDCompositionVisual> visual;
    };
    struct Graphics {
        struct SystemModule {
            HMODULE handle;
            explicit SystemModule(const wchar_t* name)
                : handle(LoadLibraryExW(name, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32)) {}
            ~SystemModule() { if (handle) FreeLibrary(handle); }
            SystemModule(const SystemModule&) = delete;
            SystemModule& operator=(const SystemModule&) = delete;
        };
        // Members release objects before unloading the libraries that implement them.
        SystemModule d3d11{L"d3d11.dll"};
        SystemModule dcomp{L"dcomp.dll"};
        Microsoft::WRL::ComPtr<ID3D11Device> device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext1> context;
        Microsoft::WRL::ComPtr<IDCompositionDevice> compositor;
        Microsoft::WRL::ComPtr<IDCompositionSurface> pixel;
        Microsoft::WRL::ComPtr<IDCompositionTarget> target;
        Microsoft::WRL::ComPtr<IDCompositionVisual> root;
        Color color = Color::Red;
    };
    HINSTANCE module_ = nullptr;
    HWND target_ = nullptr;
    UINT topologyMessage_ = 0;
    HWND overlay_ = nullptr;
    RECT desktop_{};
    std::shared_ptr<Graphics> graphics_;
    std::vector<Strip> strips_;
    std::vector<Monitor> monitors_;
    std::vector<D2D_MATRIX_3X2_F> frame_;
    bool registered_ = false;
    bool displayPosted_ = false, devicePosted_ = false;
    bool active_ = false;
    bool unavailable_ = false;
    bool updating_ = false, closeDeferred_ = false;

    class PerMonitorDpi {
        DPI_AWARENESS_CONTEXT previous_ = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    public:
        explicit operator bool() const { return previous_ != nullptr; }
        ~PerMonitorDpi() { if (previous_) SetThreadDpiAwarenessContext(previous_); }
    };

    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM w, LPARAM l) {
        if (message == WM_NCCREATE)
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams));
        if (message == WM_NCHITTEST) return HTTRANSPARENT;
        if (message == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
        if (message == WM_DISPLAYCHANGE || message == WM_DWMCOMPOSITIONCHANGED || message == WM_PAINT) {
            auto* self = reinterpret_cast<ZoomBorder*>(GetWindowLongPtrW(window, GWLP_USERDATA));
            if (self && self->target_) {
                const bool paint = message == WM_PAINT;
                auto& posted = paint ? self->devicePosted_ : self->displayPosted_;
                const auto update = paint ? Update::Device : Update::Display;
                if (!posted) posted = PostMessageW(self->target_, self->topologyMessage_, static_cast<WPARAM>(update), 0) != FALSE;
            }
        }
        return DefWindowProcW(window, message, w, l);
    }

    bool registerClass() {
        if (registered_) return true;
        WNDCLASSW cls{};
        cls.hInstance = module_;
        cls.lpfnWndProc = windowProc;
        cls.lpszClassName = kClassName;
        if (!RegisterClassW(&cls)) return false;
        registered_ = true;
        return true;
    }

    static BOOL CALLBACK addMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM data) {
        MONITORINFO info{};
        info.cbSize = sizeof(info);
        if (!GetMonitorInfoW(monitor, &info)) return FALSE;
        auto& monitors = *reinterpret_cast<std::vector<Monitor>*>(data);
        monitors.push_back({monitor, info.rcMonitor, {}, {}});
        return TRUE;
    }

    static std::optional<pinch::View> validatedView(double scale, int x, int y) {
        if (!std::isfinite(scale) || scale < 1 || scale > 16) return std::nullopt;
        if (scale == 1) return pinch::View{};
        if (x < -262144 || x > 262144 || y < -262144 || y > 262144) return std::nullopt;
        return pinch::View{scale, x, y};
    }

    bool sampleViews(ReadTransform read, ReadMonitorTransform readMonitor, bool expectsMonitorZoom) {
        std::optional<pinch::View> legacy;
        float scale = 1;
        int x = 0, y = 0;
        if (read && read(&scale, &x, &y)) legacy = validatedView(scale, x, y);
        const bool spansDisplays = legacy && legacy->zoom > 1;
        bool independent = expectsMonitorZoom;
        for (auto& monitor : monitors_) {
            monitor.sampled.reset();
            if (!spansDisplays && readMonitor) {
                double factor = 1;
                int offsetX = 0, offsetY = 0;
                if (readMonitor(monitor.handle, &factor, &offsetX, &offsetY))
                    monitor.sampled = validatedView(factor, offsetX, offsetY);
                if (monitor.sampled && monitor.sampled->zoom > 1) independent = true;
            }
        }
        bool changed = false;
        for (auto& monitor : monitors_) {
            std::optional<Projection> next;
            if (monitor.sampled) next = Projection{*monitor.sampled, true};
            else if (legacy && (spansDisplays || !independent)) next = Projection{*legacy, false};
            if (next != monitor.projection) {
                monitor.projection = next;
                changed = true;
            }
        }
        return changed;
    }

    void clearOverlay() {
        const auto graphics = graphics_;
        strips_.clear();
        if (graphics) {
            graphics->target.Reset();
            graphics->root.Reset();
        }
        if (const HWND window = std::exchange(overlay_, nullptr)) DestroyWindow(window);
    }

    HWND createOverlay(ReadBand readBand, CreateInBand createInBand) {
        constexpr DWORD ex = WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT |
                             WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP;
        // WindowFromPoint runs outside this thread for the mouse broker.
        // Exclude the canvas from hit-testing even in cross-thread queries.
        constexpr DWORD style = WS_POPUP | WS_DISABLED;
        constexpr DWORD uiAccessBand = 2;
        const auto width = desktop_.right - desktop_.left, height = desktop_.bottom - desktop_.top;
        DWORD band = 0;
        // Use Magnify's existing UIAccess band to stay above desktop windows.
        auto window = CreateWindowExW(ex, kClassName, L"", style, desktop_.left, desktop_.top, width, height,
                                      nullptr, nullptr, module_, this);
        if (window && readBand(window, &band) && band == uiAccessBand) return window;
        if (window) DestroyWindow(window);
        if (!createInBand) return nullptr;
        window = createInBand(ex, kClassName, L"", style, desktop_.left, desktop_.top, width, height,
                              nullptr, nullptr, module_, this, uiAccessBand);
        if (window && readBand(window, &band) && band == uiAccessBand) return window;
        if (window) DestroyWindow(window);
        return nullptr;
    }

    static bool paintPixel(Graphics& graphics, Color color) {
        Microsoft::WRL::ComPtr<IDXGISurface> surface;
        POINT offset{};
        if (FAILED(graphics.pixel->BeginDraw(nullptr, IID_PPV_ARGS(&surface), &offset))) return false;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target;
        const bool draw = SUCCEEDED(surface.As(&texture)) &&
                          SUCCEEDED(graphics.device->CreateRenderTargetView(texture.Get(), nullptr, &target));
        if (draw) {
            const RECT pixel{offset.x, offset.y, offset.x + 1, offset.y + 1};
            const auto rgba = pixelColor(color);
            graphics.context->ClearView(target.Get(), rgba.data(), &pixel, 1);
        }
        if (FAILED(graphics.pixel->EndDraw()) || !draw) return false;
        graphics.color = color;
        return true;
    }

    bool createGraphics(Color color) {
        const auto graphics = std::make_shared<Graphics>();
        if (!graphics->d3d11.handle || !graphics->dcomp.handle) return false;
        const auto createD3D = reinterpret_cast<decltype(&D3D11CreateDevice)>(
            GetProcAddress(graphics->d3d11.handle, "D3D11CreateDevice"));
        const auto createComposition = reinterpret_cast<decltype(&DCompositionCreateDevice)>(
            GetProcAddress(graphics->dcomp.handle, "DCompositionCreateDevice"));
        if (!createD3D || !createComposition) return false;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgi;
        // Draw one texel at setup and on palette changes using software WARP.
        if (FAILED(createD3D(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                             nullptr, 0, D3D11_SDK_VERSION, &graphics->device, nullptr, &context)) ||
            FAILED(graphics->device.As(&dxgi)) || FAILED(context.As(&graphics->context)) ||
            FAILED(createComposition(dxgi.Get(), IID_PPV_ARGS(&graphics->compositor))) ||
            FAILED(graphics->compositor->CreateSurface(1, 1, DXGI_FORMAT_B8G8R8A8_UNORM,
                                                       DXGI_ALPHA_MODE_PREMULTIPLIED, &graphics->pixel)))
            return false;
        if (!paintPixel(*graphics, color) || !target_) return false;
        graphics_ = graphics;
        return true;
    }

    bool build(Color color) {
        const HMODULE user = GetModuleHandleW(L"user32.dll");
        if (!user) return false;
        const auto readBand = reinterpret_cast<ReadBand>(GetProcAddress(user, "GetWindowBand"));
        if (!readBand) return false;
        const auto createInBand = reinterpret_cast<CreateInBand>(GetProcAddress(user, "CreateWindowInBand"));
        if (!registerClass() || (!graphics_ && !createGraphics(color))) return false;
        const auto graphics = graphics_;
        const auto frame = frame_;
        overlay_ = createOverlay(readBand, createInBand);
        if (!overlay_ || !target_) return false;
        DWORD affinity = WDA_NONE;
        if (!SetLayeredWindowAttributes(overlay_, 0, 255, LWA_ALPHA) ||
            !SetWindowDisplayAffinity(overlay_, WDA_EXCLUDEFROMCAPTURE) ||
            !GetWindowDisplayAffinity(overlay_, &affinity) || affinity != WDA_EXCLUDEFROMCAPTURE ||
            FAILED(graphics->compositor->CreateTargetForHwnd(overlay_, TRUE, &graphics->target)) ||
            FAILED(graphics->compositor->CreateVisual(&graphics->root))) return false;
        for (const auto& transform : frame) {
            Microsoft::WRL::ComPtr<IDCompositionVisual> visual;
            if (FAILED(graphics->compositor->CreateVisual(&visual)) ||
                FAILED(visual->SetContent(graphics->pixel.Get())) ||
                FAILED(visual->SetBorderMode(DCOMPOSITION_BORDER_MODE_HARD)) ||
                FAILED(visual->SetBitmapInterpolationMode(DCOMPOSITION_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR)) ||
                FAILED(visual->SetTransform(transform)) ||
                FAILED(graphics->root->AddVisual(visual.Get(), TRUE, nullptr)) || !target_) return false;
            strips_.push_back({transform, std::move(visual)});
        }
        if (FAILED(graphics->target->SetRoot(graphics->root.Get())) ||
            FAILED(graphics->compositor->Commit()) || !target_) return false;
        ShowWindow(overlay_, SW_SHOWNOACTIVATE);
        return true;
    }

    static bool sameTransform(const D2D_MATRIX_3X2_F& first, const D2D_MATRIX_3X2_F& second) {
        return first._11 == second._11 && first._22 == second._22 && first._31 == second._31 && first._32 == second._32;
    }

    bool move() {
        const auto graphics = graphics_;
        bool changedTransform = false;
        for (std::size_t index = 0; index < strips_.size(); ++index) {
            const auto& strip = strips_[index];
            const auto& next = frame_[index];
            if (!sameTransform(strip.transform, next)) {
                if (FAILED(strip.visual->SetTransform(next))) return false;
                changedTransform = true;
            }
        }
        // Panning and scaling move visuals in the same compositor transaction.
        // The capture-excluded HWND never moves while the zoom transform changes.
        if (changedTransform && FAILED(graphics->compositor->Commit())) return false;
        for (std::size_t index = 0; index < strips_.size(); ++index) strips_[index].transform = frame_[index];
        return true;
    }

    void resetFrame() {
        clearOverlay();
        graphics_.reset();
        desktop_ = {};
        monitors_.clear();
        frame_.clear();
    }

    void failed() {
        unavailable_ = true;
        if (target_) KillTimer(target_, kTimer);
        resetFrame();
        log(L"Could not draw the capture-excluded zoom border.");
    }

public:
    static constexpr UINT_PTR kTimer = 0x4b5a;

    static ReadMonitorTransform resolveMonitorTransform() {
        // Windows 11 25H2 .9974. Microsoft symbols and the Magnification.dll
        // caller verify the private getter's amd64 signature on this image.
        constexpr binary_profiles::Identity identity{{
            0x73, 0x42, 0xa9, 0x8a, 0xf6, 0x08, 0x65, 0xa3, 0xe0, 0xa1, 0x03, 0x63, 0xd9, 0x9d, 0xf8, 0x15},
            1, 0x1de000, 0x1bfb6b85};
        constexpr DWORD rva = 0x8b9c0;
        const auto user = GetModuleHandleW(L"user32.dll");
        if (!user || binary_profiles::identify(user) != identity) return nullptr;
        const auto entry = GetProcAddress(user, "GetMagnificationDesktopMonitorMagnification");
        if (reinterpret_cast<ULONG_PTR>(entry) != reinterpret_cast<ULONG_PTR>(user) + rva ||
            !binary_profiles::permitted(user, rva, false)) return nullptr;
        return reinterpret_cast<ReadMonitorTransform>(entry);
    }

    static std::array<Edge, 4> edges(const RECT& monitor, pinch::View view = {}, bool monitorLocal = false) {
        constexpr LONG thickness = 5;
        // Independent zoom offsets are measured within the source monitor.
        // The legacy transform instead uses the primary desktop origin.
        const double originX = monitorLocal ? monitor.left : 0;
        const double originY = monitorLocal ? monitor.top : 0;
        const auto sourceX = [&](LONG x) { return originX + view.x + (x - originX) / view.zoom; };
        const auto sourceY = [&](LONG y) { return originY + view.y + (y - originY) / view.zoom; };
        const auto makeEdge = [&](const RECT& pixels) -> Edge {
            const double x = sourceX(pixels.left), y = sourceY(pixels.top);
            const double left = std::floor(x), top = std::floor(y);
            Edge edge;
            edge.bounds = {static_cast<LONG>(left), static_cast<LONG>(top),
                           static_cast<LONG>(std::ceil(sourceX(pixels.right))),
                           static_cast<LONG>(std::ceil(sourceY(pixels.bottom)))};
            edge.transform._11 = static_cast<float>((pixels.right - pixels.left) / view.zoom);
            edge.transform._22 = static_cast<float>((pixels.bottom - pixels.top) / view.zoom);
            edge.transform._31 = static_cast<float>(x - left);
            edge.transform._32 = static_cast<float>(y - top);
            return edge;
        };
        const auto [left, top, right, bottom] = monitor;
        if (right - left <= 2 * thickness || bottom - top <= 2 * thickness)
            return {makeEdge(monitor), {}, {}, {}};
        return {makeEdge({left, top, right, top + thickness}),
                makeEdge({left, bottom - thickness, right, bottom}),
                makeEdge({left, top + thickness, left + thickness, bottom - thickness}),
                makeEdge({right - thickness, top + thickness, right, bottom - thickness})};
    }
    static D2D_MATRIX_3X2_F positionOnOverlay(const Edge& edge, const RECT& desktop) {
        auto result = edge.transform;
        result._31 += static_cast<float>(edge.bounds.left - desktop.left);
        result._32 += static_cast<float>(edge.bounds.top - desktop.top);
        return result;
    }
    void initialize(HINSTANCE module, HWND target, UINT topologyMessage) {
        module_ = module;
        target_ = target;
        topologyMessage_ = topologyMessage;
    }
    void refresh(bool active, ReadTransform read, Update update = Update::Transform, Color color = Color::Red,
                 ReadMonitorTransform readMonitor = nullptr, bool expectsMonitorZoom = false) {
        if (update == Update::Display) displayPosted_ = false;
        if (update == Update::Device) devicePosted_ = false;
        if (!target_ || updating_) return;
        updating_ = true;
        struct FinishUpdate {
            ZoomBorder& border;
            ~FinishUpdate() {
                border.updating_ = false;
                if (std::exchange(border.closeDeferred_, false)) border.close();
            }
        } finish{*this};
        if (!active) {
            if (std::exchange(active_, false)) {
                KillTimer(target_, kTimer);
                resetFrame();
            }
            unavailable_ = false;
            return;
        }
        if (!active_) {
            // Native panning and animation also happen without a mod command.
            // Read the applied transform on the UI thread only while shown.
            if (!SetTimer(target_, kTimer, 16, nullptr)) return;
            active_ = true;
        }
        if (unavailable_) return;
        if (update == Update::Device) {
            if (const auto graphics = graphics_) {
                BOOL ready = FALSE;
                if ((FAILED(graphics->compositor->CheckDeviceState(&ready)) || !ready) && target_) failed();
            }
            return;
        }
        if (update == Update::Display) resetFrame();
        if (!target_) return;
        if (const auto graphics = graphics_; graphics && graphics->color != color) {
            if (!paintPixel(*graphics, color) || FAILED(graphics->compositor->Commit())) {
                if (target_) failed();
                return;
            }
            if (!target_) return;
        }
        if (monitors_.empty()) {
            const PerMonitorDpi dpi;
            if (!dpi || !EnumDisplayMonitors(nullptr, nullptr, addMonitor, reinterpret_cast<LPARAM>(&monitors_)) ||
                monitors_.empty()) {
                failed();
                return;
            }
            desktop_ = monitors_.front().bounds;
            for (const auto& monitor : monitors_) UnionRect(&desktop_, &desktop_, &monitor.bounds);
        }
        if (!sampleViews(read, readMonitor, expectsMonitorZoom) || !target_) return;
        frame_.clear();
        for (const auto& monitor : monitors_) {
            if (!monitor.projection) continue; // Keep the frame off where the transform is unknown.
            for (const auto& edge : edges(monitor.bounds, monitor.projection->view, monitor.projection->monitorLocal)) {
                if (edge.bounds.left < edge.bounds.right && edge.bounds.top < edge.bounds.bottom)
                    frame_.push_back(positionOnOverlay(edge, desktop_));
            }
        }
        if (frame_.size() == strips_.size()) {
            if (!move() && target_) failed();
        } else {
            clearOverlay();
            if (target_ && !frame_.empty()) {
                const PerMonitorDpi dpi;
                if ((!dpi || !build(color)) && target_) failed();
            }
        }
    }
    void close() {
        if (target_ && active_) KillTimer(target_, kTimer);
        target_ = nullptr;
        if (updating_) { closeDeferred_ = true; return; }
        resetFrame();
        if (registered_ && !UnregisterClassW(kClassName, module_)) {
            // An outstanding window can still call its window procedure.
            HMODULE pinned = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                               reinterpret_cast<LPCWSTR>(&windowProc), &pinned);
        }
        registered_ = false;
        active_ = false;
        unavailable_ = false;
        displayPosted_ = devicePosted_ = false;
    }
};

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
    pinch::Report report;
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
    double pinchStartLevel(bool enlarging) const {
        const double acknowledged = current();
        if (!nativeActive()) return acknowledged;
        const auto& state = *native_.get();
        if (state.overviewSavedLevel > 0) return acknowledged;
        const auto wheel = static_cast<LONG>(bridge::InputSource::Wheel);
        if (enlarging || state.currentSource == wheel ||
            (state.requestSource == wheel && state.requestedSequence != state.appliedSequence)) {
            const auto visible = read();
            if (visible && std::isfinite(visible->zoom) && visible->zoom >= 1 && visible->zoom <= 16) {
                // Per-monitor zoom can leave the global transform at 100%.
                if ((state.nativeCaps & compatibility::NativeUiLevel) && visible->zoom == 1) return acknowledged;
                // A composition sample can lag the accepted native target.
                // Taking over an outward gesture must never request a decrease.
                return enlarging ? std::max(acknowledged, visible->zoom) : visible->zoom;
            }
        }
        return acknowledged;
    }
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
    bool submitTarget(double value, bridge::InputSource source) {
        if (!nativeActive()) return target(value, true, source);
        const auto* state = native_.get();
        const LONG sequence = state->requestedSequence;
        return native_.requestLevel(value, keyboardRevision_, source) &&
               state->requestedSequence != sequence;
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

enum class WindowActivation { Failed, Busy, Pending, Confirmed };
struct WindowAttempt { std::uint64_t ticket = 0; WindowActivation state = WindowActivation::Failed; };
class WindowSwitcher {
#ifdef PINCH_ZOOM_RUNTIME_TEST
    friend struct WindowSwitcherTestAccess;
    inline static decltype(&EnumWindows) apiEnumerate = EnumWindows;
    inline static decltype(&GetForegroundWindow) apiForeground = GetForegroundWindow;
    inline static decltype(&SetForegroundWindow) apiActivate = SetForegroundWindow;
    inline static decltype(&GetAncestor) apiAncestor = GetAncestor;
    inline static decltype(&IsWindow) apiValid = IsWindow;
    inline static decltype(&IsIconic) apiIconic = IsIconic;
    inline static decltype(&ShowWindowAsync) apiShow = ShowWindowAsync;
#else
    inline static const auto apiEnumerate = EnumWindows;
    inline static const auto apiForeground = GetForegroundWindow;
    inline static const auto apiActivate = SetForegroundWindow;
    inline static const auto apiAncestor = GetAncestor;
    inline static const auto apiValid = IsWindow;
    inline static const auto apiIconic = IsIconic;
    inline static const auto apiShow = ShowWindowAsync;
#endif
    pinch::WindowCycle cycle_;
    std::mutex mutex_;
    std::uint64_t serial_ = 0, pending_ = 0, started_ = 0;
    HWND target_ = nullptr, origin_ = nullptr;
    std::array<WindowAttempt, 16> completed_{};
    void finish(WindowActivation state) {
        if (state == WindowActivation::Confirmed) cycle_.commit(reinterpret_cast<std::uintptr_t>(target_));
        completed_[pending_ % completed_.size()] = {pending_, state}; pending_ = 0;
    }
    void pollLocked(std::uint64_t now) {
        if (!pending_) return;
        const HWND current = apiAncestor(apiForeground(), GA_ROOT);
        if (current == target_) finish(WindowActivation::Confirmed);
        else if (!apiValid(target_) || current != origin_ || now < started_ || now - started_ >= 150) finish(WindowActivation::Failed);
    }
    static BOOL CALLBACK collect(HWND window, LPARAM parameter) {
        if (!IsWindowVisible(window) || !IsWindowEnabled(window)) return TRUE;
        const LONG_PTR style = GetWindowLongPtrW(window, GWL_EXSTYLE);
        if (style & (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE)) return TRUE;
        if (window == GetShellWindow() || window == GetDesktopWindow()) return TRUE;
        wchar_t name[96]{};
        GetClassNameW(window, name, ARRAYSIZE(name));
        for (const auto* excluded : {L"Shell_TrayWnd", L"Shell_SecondaryTrayWnd", L"Progman", L"WorkerW", L"XamlExplorerHostIslandWindow", L"#32768", L"ComboLBox", L"tooltips_class32"})
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
    void poll(std::uint64_t now) { std::lock_guard lock(mutex_); pollLocked(now); }
    WindowActivation status(std::uint64_t ticket, std::uint64_t now) {
        std::lock_guard lock(mutex_); pollLocked(now);
        if (pending_ == ticket) return WindowActivation::Pending;
        const auto& result = completed_[ticket % completed_.size()];
        return result.ticket == ticket ? result.state : WindowActivation::Failed;
    }
    WindowAttempt request(int direction, std::uint64_t now) {
        std::vector<std::uintptr_t> windows;
        if (!apiEnumerate(collect, reinterpret_cast<LPARAM>(&windows))) return {};
        HWND target = nullptr; std::uint64_t ticket = 0;
        {
            std::lock_guard lock(mutex_); pollLocked(now);
            if (pending_) return {0, WindowActivation::Busy};
            const HWND current = apiAncestor(apiForeground(), GA_ROOT);
            target = reinterpret_cast<HWND>(cycle_.next(windows, reinterpret_cast<std::uintptr_t>(current), direction < 0 ? 1 : -1));
            if (!target || target == current || !apiValid(target)) return {};
            ticket = ++serial_; pending_ = ticket; target_ = target; origin_ = current; started_ = now;
        }
        // Foreground activation can be asynchronous across GUI threads. Never
        // wait here or hold the cycle mutex while calling another window.
        if (apiIconic(target)) apiShow(target, SW_RESTORE);
        const BOOL accepted = apiActivate(target);
        std::lock_guard lock(mutex_);
        if (pending_ == ticket) {
            pollLocked(now);
            if (pending_ == ticket && !accepted) finish(WindowActivation::Failed);
        }
        if (pending_ == ticket) return {ticket, WindowActivation::Pending};
        const auto& result = completed_[ticket % completed_.size()];
        return result.ticket == ticket ? result : WindowAttempt{ticket, WindowActivation::Failed};
    }
    bool activate(int direction) {
        const auto result = request(direction, GetTickCount64());
        return result.state == WindowActivation::Pending || result.state == WindowActivation::Confirmed;
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
    inline static decltype(&keybd_event) apiKeyEvent = keybd_event;
    inline static decltype(&PostMessageW) apiPost = PostMessageW;
    inline static decltype(&PostThreadMessageW) apiPostThread = PostThreadMessageW;
    inline static decltype(&GetCursorPos) apiCursor = GetCursorPos;
    inline static decltype(&CallNextHookEx) apiNext = CallNextHookEx;
    inline static decltype(&SetTimer) apiSetTimer = SetTimer;
    inline static decltype(&KillTimer) apiKillTimer = KillTimer;
#else
    inline static const auto apiReadKey = GetAsyncKeyState;
    inline static const auto apiSendInput = SendInput;
    inline static const auto apiKeyEvent = keybd_event;
    inline static const auto apiPost = PostMessageW;
    inline static const auto apiPostThread = PostThreadMessageW;
    inline static const auto apiCursor = GetCursorPos;
    inline static const auto apiNext = CallNextHookEx;
    inline static const auto apiSetTimer = SetTimer;
    inline static const auto apiKillTimer = KillTimer;
#endif

    HANDLE thread_ = nullptr, ready_ = nullptr, stop_ = nullptr;
    HANDLE controllerStop_ = nullptr; // Borrowed until this broker thread joins.
    HMODULE module_ = nullptr;
    HWND target_ = nullptr;
    bridge::Record* state_ = nullptr;
    std::atomic<unsigned> options_{0};
    std::atomic<unsigned> resync_{0}; // Owner thread applies key/button resets before the next physical input.
    std::atomic<bool> installed_{false};
    std::atomic<unsigned> plainWheel_{0}; // Low bit is the mode, other bits identify its activation.
    DWORD threadId_ = 0;
    inline static thread_local MouseInputBroker* owner_ = nullptr;
    pinch::WinMenuMask menu_;
    pinch::MouseButtonOwnership buttons_;
    static constexpr ULONG_PTR kMaskTag = 0x424d3037;
    static constexpr ULONG_PTR kSideScrollTag = 0x42565357;
    static constexpr UINT kSideScrollMessage = WM_APP + 1; // Private thread message, no HWND.
    static constexpr DWORD kChordTime = 45;
    static constexpr unsigned kMouseChordFlag = 32u;
    UINT_PTR chordTimer_ = 0;
    unsigned primaryDown_ = 0, pendingButton_ = 0;
    bool chordOwned_ = false, escapeDown_ = false;
    POINT pendingPoint_{};
    DWORD pendingTime_ = 0;
    struct IntegrityCache { DWORD pid = 0, level = 0; };
    IntegrityCache pointerIntegrity_, foregroundIntegrity_;
    DWORD ownIntegrity_ = 0;

    static DWORD integrity(HANDLE process) {
        HANDLE token = nullptr;
        if (!OpenProcessToken(process, TOKEN_QUERY, &token)) return 0;
        alignas(TOKEN_MANDATORY_LABEL) std::array<BYTE, 128> data{};
        DWORD size = 0, result = 0;
        if (GetTokenInformation(token, TokenIntegrityLevel, data.data(), static_cast<DWORD>(data.size()), &size)) {
            PSID sid = reinterpret_cast<const TOKEN_MANDATORY_LABEL*>(data.data())->Label.Sid;
            if (IsValidSid(sid)) {
                const unsigned count = *GetSidSubAuthorityCount(sid);
                if (count) result = *GetSidSubAuthority(sid, count - 1);
            }
        }
        CloseHandle(token);
        return result;
    }
    static DWORD windowIntegrity(HWND window, IntegrityCache& cache) {
        DWORD pid = 0;
        GetWindowThreadProcessId(window, &pid);
        if (!pid) return 0;
        if (pid != cache.pid) {
            cache.pid = pid;
            HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            cache.level = process ? integrity(process) : 0;
            if (process) CloseHandle(process);
        }
        return cache.level;
    }
    bool canReplayAt(POINT point) {
#ifdef PINCH_ZOOM_RUNTIME_TEST
        if (!thread_) return true;
#endif
        // SendInput is constrained by UIPI. Forward physical input untouched
        // when the application beneath it is elevated or its level is unknown.
        const DWORD pointer = windowIntegrity(WindowFromPoint(point), pointerIntegrity_);
        const HWND foreground = GetForegroundWindow();
        const DWORD focused = foreground ? windowIntegrity(foreground, foregroundIntegrity_) : ownIntegrity_;
        return pointer && pointer <= ownIntegrity_ && focused && focused <= ownIntegrity_;
    }
    bool sideScrollAvailable(unsigned mode) const {
        return (mode & 1u) && (options_.load() & 1u) && menu_.mouseUnmodified() && state_->magnifierReady &&
               state_->nativeViewMode != 1 && state_->nativeViewMode != 3;
    }
    void replaySideScroll(int delta, unsigned mode, DWORD queuedAt, DWORD now) {
        if (now - queuedAt > 100 || plainWheel_.load() != mode || !sideScrollAvailable(mode)) return;
        POINT point{};
        if (!apiCursor(&point) || !canReplayAt(point)) return;
        INPUT input{};
        input.type = INPUT_MOUSE;
        input.mi.mouseData = static_cast<DWORD>(std::min(-delta, static_cast<int>(std::numeric_limits<SHORT>::max())));
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.dwExtraInfo = kSideScrollTag;
        if (apiSendInput(1, &input, sizeof(input)) == 0) {
            // The original event was consumed only once it was posted. If
            // vertical insertion fails, try its original horizontal delta.
            input.mi.mouseData = static_cast<DWORD>(delta);
            input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
            apiSendInput(1, &input, sizeof(input));
        }
    }
    unsigned takePendingButton() {
        if (chordTimer_) apiKillTimer(nullptr, std::exchange(chordTimer_, 0));
        return std::exchange(pendingButton_, 0);
    }
    bool replayButton(bool released = false) {
        const auto button = takePendingButton();
        if (!button) return false;
        INPUT input[2]{};
        for (auto& item : input) {
            item.type = INPUT_MOUSE;
            item.mi.dwExtraInfo = kMaskTag;
        }
        input[0].mi.dwFlags = button == 1 ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN;
        input[0].mi.time = pendingTime_;
        input[1].mi.dwFlags = button == 1 ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP;
        const UINT count = released ? 2 : 1;
        const UINT sent = apiSendInput(count, input, sizeof(INPUT));
        if (released && sent == 1) return apiSendInput(1, &input[1], sizeof(INPUT)) == 1;
        if (!released && !sent) pendingButton_ = button; // Retry the pair on physical release.
        return released || sent == count;
    }
    bool movedFromDeferredClick(POINT point) const {
        return std::abs(point.x - pendingPoint_.x) >= 4 || std::abs(point.y - pendingPoint_.y) >= 4;
    }
    bool primaryEvent(WPARAM message, const MSLLHOOKSTRUCT& event, bool mayChord) {
        const bool down = message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN;
        const unsigned button = message == WM_LBUTTONDOWN || message == WM_LBUTTONUP ? 1 : 2;
        const bool wasDown = (primaryDown_ & button) != 0;
        if (down) primaryDown_ |= button;
        else primaryDown_ &= ~button;
        if (chordOwned_) {
            if (!primaryDown_) chordOwned_ = false;
            return true;
        }
        if (!down) {
            if (pendingButton_ != button) return false;
            return replayButton(true); // A failed synthetic up can still use the real one.
        }
        if (wasDown) return pendingButton_ == button;
        if (pendingButton_) {
            if (chordTimer_ && mayChord && event.time - pendingTime_ <= kChordTime && !movedFromDeferredClick(event.pt) &&
                apiPost(target_, kMouseActionMessage, kToggleZoomAction, 0)) {
                takePendingButton();
                chordOwned_ = true;
                return true;
            }
            replayButton();
            return false;
        }
        if (!mayChord || primaryDown_ != button || !canReplayAt(event.pt)) return false;
        chordTimer_ = apiSetTimer(nullptr, 0, kChordTime, nullptr);
        if (!chordTimer_) return false;
        pendingButton_ = button;
        pendingPoint_ = event.pt;
        pendingTime_ = event.time;
        return true;
    }
    void consumeResync() {
        if (!resync_.load(std::memory_order_relaxed)) return;
        const unsigned requested = resync_.exchange(0, std::memory_order_acq_rel);
        if (!requested) return;
        menu_.reset();
        // Generic modifier aliases would remain down after an L/R-specific UP.
        // The neutral injected mask is never a physical shortcut modifier.
        for (unsigned vk = 8; vk < 256; ++vk) {
            if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU || vk == 0xe8) continue;
            if (apiReadKey(static_cast<int>(vk)) & 0x8000) menu_.key(vk, true);
        }
        if (requested & 2u) {
            takePendingButton();
            buttons_.reset();
            chordOwned_ = escapeDown_ = false;
            primaryDown_ = ((apiReadKey(VK_LBUTTON) & 0x8000) ? 1u : 0u) |
                           ((apiReadKey(VK_RBUTTON) & 0x8000) ? 2u : 0u);
        }
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
        if (sent == 2) { InterlockedIncrement(&state_->menuMaskInitialSent); return true; }
        InterlockedIncrement(&state_->menuMaskInitialFailed);
        if (sent == 1) apiSendInput(1, &input[1], sizeof(INPUT));
        menu_.maskFailed();
        return false; // UIPI can reject input: leave this event to Windows.
    }
    void zoomWheel(int delta) {
        const LONG revision = InterlockedCompareExchange(&state_->inputRevision, 0, 0);
        if (bridge::enqueueWheel(state_, delta, revision, GetTickCount64())) apiPost(target_, kWheelMessage, 0, 0);
        InterlockedIncrement(&state_->wheelEvents);
    }
    static LRESULT CALLBACK keyboard(int code, WPARAM message, LPARAM data) {
        if (code == HC_ACTION && owner_) {
            const auto* event = reinterpret_cast<const KBDLLHOOKSTRUCT*>(data);
            const bool down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
            if (!(event->flags & LLKHF_INJECTED)) {
                owner_->consumeResync();
                if (down && owner_->chordTimer_) owner_->replayButton();
                if (event->vkCode == VK_ESCAPE) {
                    if (down && !owner_->escapeDown_ && owner_->menu_.winOnly() && (owner_->options_.load() & 1u)) {
                        owner_->setPlainWheel(false);
                        apiPost(owner_->target_, kMouseActionMessage, kExitZoomAction, 0);
                    }
                    owner_->escapeDown_ = down;
                }
            }
            // Disguise a consumed mouse chord at physical Win release as well:
            // Win auto-repeat can invalidate the earlier mouse-side mask.
            // keybd_event runs this neutral pair before CallNext exposes Win UP;
            // SendInput from this callback can queue it after the release.
            if (!(event->flags & LLKHF_INJECTED) && !down && owner_->menu_.maskRelease(event->vkCode)) {
                apiKeyEvent(0xe8, 0, 0, kMaskTag);
                apiKeyEvent(0xe8, 0, KEYEVENTF_KEYUP, kMaskTag);
                InterlockedIncrement(&owner_->state_->menuMaskReleaseSent);
            }
            owner_->menu_.key(event->vkCode, down, (event->flags & LLKHF_INJECTED) != 0);
        }
        return apiNext(nullptr, code, message, data);
    }
    static LRESULT CALLBACK mouse(int code, WPARAM message, LPARAM data) {
        auto* broker = owner_;
        if (code != HC_ACTION || !broker) return apiNext(nullptr, code, message, data);
        const auto* event = reinterpret_cast<const MSLLHOOKSTRUCT*>(data);
        if ((event->flags & LLMHF_INJECTED) || event->dwExtraInfo == kSideScrollTag)
            return apiNext(nullptr, code, message, data);
        // Windows labels the mouse messages produced by a pen or touch screen.
        constexpr ULONG_PTR tabletSignature = 0xff515700;
        constexpr ULONG_PTR tabletMask = 0xffffff00;
        if ((event->dwExtraInfo & tabletMask) == tabletSignature) return apiNext(nullptr, code, message, data);
        broker->consumeResync();
        const unsigned flags = broker->options_.load();
        if (message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_RBUTTONDOWN || message == WM_RBUTTONUP) {
            constexpr unsigned chordRequirements = 9u | kMouseChordFlag;
            const bool mayChord = (flags & chordRequirements) == chordRequirements && broker->menu_.idle() &&
                broker->state_->nativeViewMode != 1 && broker->state_->nativeViewMode != 3 &&
                compatibility::mouseZoomAllowed(static_cast<std::uint32_t>(broker->state_->nativeCaps),
                                                 broker->state_->resolverReasonNative, broker->state_->nativeError);
            if (broker->primaryEvent(message, *event, mayChord)) return 1;
        } else if (broker->chordTimer_ &&
                   (message != WM_MOUSEMOVE || broker->movedFromDeferredClick(event->pt))) {
            // A drag or another mouse action needs the original button down immediately.
            broker->replayButton();
        }
        if (message == WM_MOUSEHWHEEL) {
            const unsigned mode = broker->plainWheel_.load();
            const int delta = static_cast<SHORT>(HIWORD(event->mouseData));
            if (delta && broker->sideScrollAvailable(mode) && broker->canReplayAt(event->pt) &&
                apiPostThread(broker->threadId_, kSideScrollMessage, static_cast<WPARAM>(static_cast<USHORT>(delta)), mode))
                return 1;
            return apiNext(nullptr, code, message, data);
        }
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
        if (!(flags & 1)) return forward();
        const bool zoomUsable = broker->state_->nativeViewMode != 1 && broker->state_->nativeViewMode != 3 &&
            compatibility::mouseZoomAllowed(static_cast<std::uint32_t>(broker->state_->nativeCaps),
                                             broker->state_->resolverReasonNative, broker->state_->nativeError);
        const bool pinching = bridge::pinchDown(*broker->state_);
        const bool mouseMode = (broker->plainWheel_.load() & 1u) && broker->menu_.mouseUnmodified() &&
                               zoomUsable && broker->state_->magnifierReady && (flags & 8u);
        if (message == WM_MOUSEWHEEL && (mouseMode ||
            (pinching && !broker->escapeDown_ && zoomUsable && (flags & 8u) && broker->menu_.mouseUnmodified() &&
             bridge::pinchFrameFresh(*broker->state_, GetTickCount64())))) {
            const int delta = static_cast<SHORT>(HIWORD(event->mouseData));
            if (delta) broker->zoomWheel(delta);
            return 1;
        }
        if (message == WM_MBUTTONDOWN && mouseMode &&
            apiPost(broker->target_, kMouseActionMessage, kResetArmedZoomAction, 0)) {
            broker->buttons_.press(1, true);
            return 1;
        }
        const bool ctrl = (apiReadKey(VK_CONTROL) & 0x8000) != 0;
        const bool alt = (apiReadKey(VK_MENU) & 0x8000) != 0;
        const bool shift = (apiReadKey(VK_SHIFT) & 0x8000) != 0;
        const bool win = broker->menu_.winDown();
        const bool winZoom = (flags & 2) && win && !ctrl && !alt && !shift && broker->menu_.winOnly();
        const bool ctrlAlt = ctrl && alt && !win && !shift;
        const bool otherMode = broker->state_->nativeViewMode == 1 || broker->state_->nativeViewMode == 3;
        const bool zoom = zoomUsable && (flags & 8) && ((flags & 2) ? winZoom : ctrlAlt);
        if (message == WM_MOUSEWHEEL) {
            const int delta = static_cast<SHORT>(HIWORD(event->mouseData));
            if (delta && zoom && (!(flags & 2) || broker->maskStart())) {
                broker->zoomWheel(delta);
                if (flags & 2) broker->menu_.consumedMouse();
                return 1;
            }
            if (delta && ctrlAlt && (flags & 2) && (flags & 4) && !otherMode) {
                bridge::enqueueWindowWheel(broker->state_, delta, GetTickCount64());
                return 1;
            }
        }
        if (zoomUsable && message == WM_MBUTTONDOWN && ((flags & 2) ? winZoom : ctrlAlt)) {
            if ((!(flags & 2) || broker->maskStart()) && apiPost(broker->target_, kMouseActionMessage, 1, 0)) {
                if (flags & 2) broker->menu_.consumedMouse();
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
        broker->threadId_ = GetCurrentThreadId();
        const HANDLE stopSignals[]{broker->stop_, broker->controllerStop_};
        const DWORD stopCount = broker->controllerStop_ ? 2 : 1;
        const auto stopping = [&] { return WaitForMultipleObjects(stopCount, stopSignals, FALSE, 0) != WAIT_TIMEOUT; };
        broker->ownIntegrity_ = integrity(GetCurrentProcess());
        broker->primaryDown_ = ((apiReadKey(VK_LBUTTON) & 0x8000) ? 1u : 0u) |
                               ((apiReadKey(VK_RBUTTON) & 0x8000) ? 2u : 0u);
        broker->menu_.key(VK_LWIN, (apiReadKey(VK_LWIN) & 0x8000) != 0);
        broker->menu_.key(VK_RWIN, (apiReadKey(VK_RWIN) & 0x8000) != 0);
        MSG message{};
        PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
        HHOOK keys = !stopping() ? SetWindowsHookExW(WH_KEYBOARD_LL, keyboard, broker->module_, 0) : nullptr;
        HHOOK pointer = keys && !stopping() ? SetWindowsHookExW(WH_MOUSE_LL, mouse, broker->module_, 0) : nullptr;
        broker->installed_.store(keys && pointer);
        InterlockedExchange(&broker->state_->mouseHooksReady, keys && pointer ? 1 : 0);
        SetEvent(broker->ready_);
        bool running = keys && pointer;
        while (running && MsgWaitForMultipleObjectsEx(stopCount, stopSignals, INFINITE, QS_ALLINPUT,
                                                       MWMO_INPUTAVAILABLE) == WAIT_OBJECT_0 + stopCount) {
            for (unsigned processed = 0; processed < 64 && !stopping() &&
                 PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE); ++processed) {
                if (message.message == WM_QUIT) { running = false; break; }
                if (!message.hwnd && message.message == kSideScrollMessage) {
                    if (!stopping()) broker->replaySideScroll(static_cast<SHORT>(message.wParam),
                        static_cast<unsigned>(message.lParam), message.time, GetTickCount());
                    continue;
                }
                if (!message.hwnd && message.message == WM_TIMER && message.wParam == broker->chordTimer_) {
                    if (!stopping()) broker->replayButton();
                    continue;
                }
                TranslateMessage(&message); DispatchMessageW(&message);
            }
        }
        broker->takePendingButton(); // Teardown must not inject a delayed physical click.
        if (pointer) UnhookWindowsHookEx(pointer);
        if (keys) UnhookWindowsHookEx(keys);
        broker->installed_.store(false);
        InterlockedExchange(&broker->state_->mouseHooksReady, 0);
        owner_ = nullptr;
        return 0;
    }
public:
    bool start(HMODULE module, HWND target, bridge::Record* state, HANDLE controllerStop) {
        if (thread_) return installed_.load();
        module_ = module; target_ = target; state_ = state; controllerStop_ = controllerStop;
        requestResync(true);
        ready_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        stop_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!ready_ || !stop_) { stop(); return false; }
        thread_ = CreateThread(nullptr, 0, run, this, 0, nullptr);
        if (!thread_) { stop(); return false; }
        WaitForSingleObject(ready_, INFINITE);
        if (installed_.load()) return true;
        stop();
        return false;
    }
    void configure(const pinch::Config& config, bool interactive) {
        const unsigned next = (interactive && config.mouseControls && config.managesZoom() ? 1u : 0u) |
            (config.winWheel ? 2u : 0u) | (config.wheelWindowSwitch ? 4u : 0u) |
            (config.smoothWheel ? 8u : 0u) | (config.windowButtons ? 16u : 0u) |
            (config.mouseChord ? kMouseChordFlag : 0u);
        const unsigned previous = options_.exchange(next);
        if ((next & 1u) && !(previous & 1u)) requestResync();
    }
    void requestResync(bool clearButtons = false) {
        resync_.fetch_or(clearButtons ? 3u : 1u, std::memory_order_release);
    }
    void setPlainWheel(bool active) {
        auto previous = plainWheel_.load();
        while (static_cast<bool>(previous & 1u) != active &&
               !plainWheel_.compare_exchange_weak(previous, previous + 1u)) {}
    }
    void stop() {
        options_.store(0);
        if (thread_) {
            SetEvent(stop_);
            WaitForSingleObject(thread_, INFINITE); // The controller holds this object and its module until the broker exits.
            CloseHandle(thread_); thread_ = nullptr;
        }
        if (stop_) { CloseHandle(stop_); stop_ = nullptr; }
        if (ready_) { CloseHandle(ready_); ready_ = nullptr; }
    }
    ~MouseInputBroker() { stop(); }
};

class Host {
#ifdef PINCH_ZOOM_RUNTIME_TEST
    friend struct HostTestAccess;
#endif
    pinch::Config config_;
    pinch::Config (*readSettings_)() = nullptr;
    SessionLease runtimeLease_;
    HINSTANCE windowClassModule_ = nullptr;
    HANDLE controllerStop_ = nullptr;
    bool resetHotkeyRegistered_ = false, pauseHotkeyRegistered_ = false;
    std::map<HANDLE, Device> devices_;
    std::vector<BYTE> rawInputBuffer_;
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
    std::uint64_t wheelDeadline_ = 0;
    HANDLE wheelFrameTimer_ = nullptr;
    LONGLONG wheelFramePeriod_ = 0;
    bool triedWheelFrameTimer_ = false, wheelFrameScheduled_ = false;
    MouseInputBroker mouseInput_;
    pinch::WindowWheel windowWheel_;
    WindowSwitcher* windowSwitcher_ = &switcher;
    std::uint64_t windowTicket_ = 0, windowGeneration_ = 0, windowRetry_ = 0;
    int coldWheelDelta_ = 0;
    std::uint64_t coldWheelDeadline_ = 0;
    LONG coldWheelCommands_ = 0;
    bool zoomModeLatched_ = false, zoomModePublished_ = false;
    bool previousMagnified_ = false, pinchReachedNormal_ = false;
    struct ModeSuppression { LONG level, commands, sequence; };
    std::optional<ModeSuppression> modeSuppression_;
    bool zoomInputEnabled() const {
        return sessionActive_ && !paused_ && config_.managesZoom();
    }
    void publishZoomMode(bool active) {
        mouseInput_.setPlainWheel(active && config_.mouseWheelEnabled());
        if (active == zoomModePublished_) return;
        zoomModePublished_ = active;
        if (auto* state = bridge_.get()) {
            InterlockedExchange(&state->zoomModeActive, active ? 1 : 0);
            bridge_.wake();
        }
    }
    void disarmZoomMode() {
        zoomModeLatched_ = false;
        if (const auto* state = bridge_.get())
            modeSuppression_ = ModeSuppression{state->overviewSavedLevel > 0 ? state->overviewSavedLevel : state->nativeLevel,
                                               state->nativeCommands, state->requestedSequence};
        publishZoomMode(false);
    }
    void updateZoomMode() {
        auto* state = bridge_.get();
        if (!state || !zoomInputEnabled() || state->nativeViewMode == 1 ||
            state->nativeViewMode == 3 || !bridge_.magnifierAvailable()) {
            publishZoomMode(false);
            return;
        }
        const LONG level = state->overviewSavedLevel > 0 ? state->overviewSavedLevel : state->nativeLevel;
        const bool magnified = level > 10000;
        if (!config_.mouseWheelEnabled()) {
            zoomModeLatched_ = false;
            modeSuppression_.reset();
            previousMagnified_ = magnified;
            publishZoomMode(magnified);
            return;
        }
        const auto source = static_cast<bridge::InputSource>(state->currentSource);
        if (!magnified && previousMagnified_) {
            if (bridge::pendingLevel(*state)) return; // The applied source can lag the level until the native update is acknowledged.
            zoomModeLatched_ = !modeSuppression_ && bridge::wheelSource(source);
            pinchReachedNormal_ = !zoomModeLatched_;
            modeSuppression_.reset();
        }
        previousMagnified_ = magnified;
        if (modeSuppression_ && magnified && level != modeSuppression_->level && !state->pendingCommand) {
            const bool freshKeyboard = source == bridge::InputSource::Keyboard && state->nativeCommands != modeSuppression_->commands;
            const bool freshWheel = source == bridge::InputSource::Wheel &&
                state->appliedSequence != modeSuppression_->sequence && state->appliedSequence == state->requestedSequence &&
                state->requestInputRevision == state->inputRevision && state->requestSource == static_cast<LONG>(source);
            if (freshKeyboard || freshWheel) modeSuppression_.reset();
        }
        publishZoomMode(!modeSuppression_ && (zoomModeLatched_ || magnified ||
                        (bridge::pinchDown(*state) && !pinchReachedNormal_)));
    }
    void toggleZoomMode(std::uint64_t now) {
        if (!zoomInputEnabled() || !config_.mouseWheelEnabled()) return;
        updateZoomMode();
        if (zoomModePublished_ || zoomModeLatched_) {
            disarmZoomMode();
            cancelWheel();
        } else {
            ensureNativeMagnifier(now, true);
            zoomModeLatched_ = true;
            modeSuppression_.reset();
            updateZoomMode();
        }
    }
    void resetFromMouse() {
        disarmZoomMode();
        restartGestureTracking();
        zoom_.resetToNormal();
    }
    void stopWheelFrames() {
        if (wheelFrameTimer_ && std::exchange(wheelFrameScheduled_, false)) CancelWaitableTimer(wheelFrameTimer_);
    }
    LONGLONG wheelFramePeriod() {
        if (!wheelFramePeriod_) {
            DWM_TIMING_INFO timing{};
            timing.cbSize = sizeof(timing);
            double periodMs = 16;
            if (SUCCEEDED(DwmGetCompositionTimingInfo(nullptr, &timing)) &&
                timing.rateCompose.uiNumerator && timing.rateCompose.uiDenominator) {
                periodMs = 1000.0 * timing.rateCompose.uiDenominator / timing.rateCompose.uiNumerator;
            }
            wheelFramePeriod_ = std::llround(std::clamp(periodMs, 4.0, 16.0) * 10000);
        }
        return wheelFramePeriod_;
    }
    void scheduleWheelFrame() {
        if (!wheel_.active()) {
            stopWheelFrames();
            return;
        }
        if (!window_ || wheelFrameScheduled_) return;
        if (!triedWheelFrameTimer_) {
            triedWheelFrameTimer_ = true;
            wheelFrameTimer_ = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                                      SYNCHRONIZE | TIMER_MODIFY_STATE);
        }
        if (!wheelFrameTimer_) return; // The normal host timer keeps a fallback cadence.
        LARGE_INTEGER due{};
        due.QuadPart = -wheelFramePeriod();
        wheelFrameScheduled_ = SetWaitableTimer(wheelFrameTimer_, &due, 0, nullptr, nullptr, FALSE) != FALSE;
    }
    static double preciseMilliseconds(std::uint64_t fallback) {
        static const double counterToMs = [] {
            LARGE_INTEGER frequency{};
            return QueryPerformanceFrequency(&frequency) && frequency.QuadPart > 0
                ? 1000.0 / static_cast<double>(frequency.QuadPart) : 0.0;
        }();
        LARGE_INTEGER counter{};
        return counterToMs > 0 && QueryPerformanceCounter(&counter)
            ? static_cast<double>(counter.QuadPart) * counterToMs : static_cast<double>(fallback);
    }
    void cancelWheel(bool invalidate = true) {
        wheel_.cancel();
        stopWheelFrames();
        wheelDeadline_ = 0;
        if (invalidate) coldWheelDelta_ = 0;
        if (auto* state = bridge_.get()) {
            InterlockedExchange(&state->wheelActive, 0);
            if (invalidate) {
                const auto epoch = static_cast<std::uint32_t>(InterlockedIncrement(&state->inputRevision));
                LONG64 queued = InterlockedCompareExchange64(&state->wheelPacket, 0, 0);
                while (queued && pinch::wheelRevision(static_cast<std::uint64_t>(queued)) != epoch) {
                    const LONG64 actual = InterlockedCompareExchange64(&state->wheelPacket, 0, queued);
                    if (actual == queued) break;
                    queued = actual;
                }
                for (auto& [id, dev] : devices_) { (void)id; dev.coldPinchSeed.reset(); }
            } // A new generation already published by native keys keeps its own pending input.
        }
    }
    void wheelInput(int delta, LONG revision, std::uint64_t now) {
        auto* state = bridge_.get();
        if (!delta || !state || !zoomInputEnabled() || !config_.mouseWheelEnabled() ||
            state->nativeViewMode == 1 || state->nativeViewMode == 3) return;
        if (revision == -1 && bridge_.magnifierAvailable()) return; // Legacy wake-only messages never become zoom.
        if (revision != -1 && revision != state->inputRevision) return;
        if (!bridge_.magnifierAvailable()) {
            ensureNativeMagnifier(now, true);
            // Preserve at most one notch while the native UI starts, never a
            // free-spin queue. A close/session change discards this intent.
            if (now > coldWheelDeadline_) coldWheelDelta_ = 0;
            coldWheelDelta_ = static_cast<int>(std::clamp<std::int64_t>(
                static_cast<std::int64_t>(coldWheelDelta_) + delta, -120, 120));
            coldWheelDeadline_ = now + 1500;
            coldWheelCommands_ = state->nativeCommands;
            return;
        }
        if (bridge::pinchDown(*state)) {
            const auto active = devices_.find(activeDevice_);
            if (active == devices_.end() || !bridge_.gateAvailable()) return;
            if (const auto level = active->second.recognizer.wheelZoom(delta, config_.wheelStep, config_.maxZoom))
                zoom_.target(*level, false, bridge::InputSource::PinchWheel);
            return;
        }
        if (wheelRevision_ != state->inputRevision) cancelWheel(false);
        wheelRevision_ = state->inputRevision;
        const bool pinchQueued = bridge::pinchSource(static_cast<bridge::InputSource>(state->requestSource)) &&
                                 bridge::pendingLevel(*state);
        const double base = pinchQueued ? state->requestedLevel / 10000.0 : zoom_.current();
        wheel_.add(delta, base, config_.wheelStep, config_.maxZoom);
        wheelDeadline_ = now + 200;
    }
    void renderWheel(bridge::Record& state, std::uint64_t now) {
        const LONG epoch = InterlockedCompareExchange(&state.inputRevision, 0, 0);
        if (epoch != wheelRevision_) {
            cancelWheel(false);
            wheelRevision_ = epoch;
        }
        const bool enabled = zoomInputEnabled() && config_.mouseWheelEnabled() &&
                             state.nativeViewMode != 1 && state.nativeViewMode != 3;
        if (!enabled || !state.magnifierReady || (wheel_.active() && now > wheelDeadline_)) {
            wheel_.cancel();
            wheelDeadline_ = 0;
        }
        if (coldWheelDelta_) {
            if (now > coldWheelDeadline_ || !enabled || coldWheelCommands_ != state.nativeCommands) coldWheelDelta_ = 0;
            else if (bridge_.magnifierAvailable()) {
                const int delta = std::exchange(coldWheelDelta_, 0);
                wheelInput(delta, epoch, now);
            }
        }
        const auto packet = static_cast<std::uint64_t>(InterlockedExchange64(&state.wheelPacket, 0));
        const auto captured = static_cast<std::uint64_t>(InterlockedCompareExchange64(&state.wheelTime, 0, 0));
        if (pinch::wheelDelta(packet) && (captured >= now || now - captured <= 200))
            wheelInput(pinch::wheelDelta(packet), static_cast<LONG>(pinch::wheelRevision(packet)), now);
        const auto outstanding = [&]() {
            return state.requestedSequence != state.appliedSequence && state.requestInputRevision == wheelRevision_ &&
                   state.requestSource == static_cast<LONG>(bridge::InputSource::Wheel);
        };
        if (wheel_.active() && !state.pendingCommand) {
            const double motionTime = window_ ? preciseMilliseconds(now) : static_cast<double>(now);
            const double frameMs = window_ ? wheelFramePeriod() / 10000.0 : 10;
            if (const auto level = wheel_.sample(motionTime, frameMs)) {
                const long long previous = outstanding() ? state.requestedLevel : std::llround(zoom_.current() * 10000);
                if (std::llround(*level * 10000) == previous ||
                    zoom_.submitTarget(*level, bridge::InputSource::Wheel)) wheel_.submitted(*level);
            }
        }
        scheduleWheelFrame();
        const auto active = enabled && state.magnifierReady && (wheel_.active() || outstanding()) ? 1 : 0;
        if (state.wheelActive != active) InterlockedExchange(&state.wheelActive, active);
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
            if (zoom_.submitTarget(seed, bridge::InputSource::Pinch)) {
                dev.coldPinchSeed.reset(); // Clear only after publishing, then resume from the original seed.
                dev.recognizer.adoptZoom(seed);
                dev.waitingForNative = false;
            }
        }
    }
    void releaseAbandonedPinch(HANDLE device) {
        if (activeDevice_ != device) return;
        activeDevice_ = nullptr;
        bridge_.publish(pinch::Phase::Idle, false, zoomInputEnabled());
        zoom_.end();
        bridge_.syncNative();
    }
    void releaseExpiredPinch(std::uint64_t now) {
        auto* state = bridge_.get();
        if (!state || !bridge::pinchDown(*state) || bridge::pinchFrameFresh(*state, now)) return;
        if (const auto active = devices_.find(activeDevice_); active != devices_.end()) {
            auto& device = active->second;
            device.activeContacts = 0;
            device.recognizer.reset();
            device.assembler.reset();
            device.coldPinchSeed.reset();
            device.waitingForNative = false;
        }
        activeDevice_ = nullptr;
        zoom_.end();
        bridge_.publish(pinch::Phase::Idle, false, zoomInputEnabled());
        bridge_.syncNative();
    }
    void render(std::uint64_t now, bool deliverInput = true) {
        if (deliverInput) releaseExpiredPinch(now);
        if (!zoom_.tick(static_cast<double>(now - lastTick_), config_.smoothingMs,
                        config_.mouseFollow, config_.edgeMargin)) restartGestureTracking();
        updateZoomMode();
        if (observedRevision_ != zoom_.revision()) {
            observedRevision_ = zoom_.revision();
            const double level = zoom_.current();
            for (auto& [id, dev] : devices_) { (void)id; dev.recognizer.adoptZoom(level); }
        }
        if (auto* state = bridge_.get()) {
            if (deliverInput) renderWheel(*state, now);
            const auto windowPacket = static_cast<std::uint64_t>(InterlockedExchange64(&state->windowWheelPacket, 0));
            const auto windowTime = static_cast<std::uint64_t>(InterlockedCompareExchange64(&state->windowWheelTime, 0, 0));
            const bool windowInput = zoomInputEnabled() && config_.mouseControls && config_.winWheel && config_.wheelWindowSwitch;
            if (windowInput && (windowTime >= now || now - windowTime <= 200))
                windowWheel_.offer(pinch::wheelDelta(windowPacket), now);
            else if (!windowInput) windowWheel_.reset();
            windowSwitcher_->poll(now);
            if (windowTicket_) {
                const auto result = windowSwitcher_->status(windowTicket_, now);
                if (result != WindowActivation::Pending) {
                    if (result == WindowActivation::Confirmed) {
                        InterlockedIncrement(&state->windowSwitchConfirmed);
                        if (windowGeneration_ == windowWheel_.generation()) windowWheel_.pop();
                    } else if (result == WindowActivation::Failed) {
                        InterlockedIncrement(&state->windowSwitchDeferredFailed);
                        windowRetry_ = now + 32;
                    }
                    windowTicket_ = 0;
                }
            }
            if (windowInput && !windowTicket_ && now >= windowRetry_) {
                if (const int direction = windowWheel_.front(now)) {
                    const auto attempt = windowSwitcher_->request(-direction, now);
                    if (attempt.state != WindowActivation::Busy) InterlockedIncrement(&state->windowSwitchAttempts);
                    if (attempt.state == WindowActivation::Confirmed) {
                        InterlockedIncrement(&state->windowSwitchConfirmed);
                        windowWheel_.pop();
                    }
                    else if (attempt.state == WindowActivation::Pending) {
                        windowTicket_ = attempt.ticket; windowGeneration_ = windowWheel_.generation();
                    } else if (attempt.state == WindowActivation::Failed) {
                        InterlockedIncrement(&state->windowSwitchRejected);
                        windowRetry_ = now + 32;
                    }
                }
            }
            if (deliverInput) applyColdPinch(now);

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
            disarmZoomMode();
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
            disarmZoomMode();
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
    static bool readTouchpadRegistration(RAWINPUTDEVICE& registration) {
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1)) return false;
        if (!count) return true;
        std::vector<RAWINPUTDEVICE> devices(count);
        const auto actual = GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE));
        if (actual == UINT(-1) || actual > devices.size()) return false;
        for (unsigned index = 0; index < actual; ++index) {
            const auto& device = devices[index];
            if (device.usUsagePage == kDigitizer && device.usUsage == kTouchpad) {
                registration = device;
                break;
            }
        }
        return true;
    }
    bool registerTouchpadInput() {
        RAWINPUTDEVICE existing{};
        if (!readTouchpadRegistration(existing) || existing.usUsage) return false;
        RAWINPUTDEVICE registration{kDigitizer, kTouchpad, RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, window_};
        inputRegistered_ = RegisterRawInputDevices(&registration, 1, sizeof(registration)) != FALSE;
        return inputRegistered_;
    }
    void unregisterTouchpadInput() {
        if (!inputRegistered_) return;
        RAWINPUTDEVICE registered{};
        if (!readTouchpadRegistration(registered)) return;
        if (registered.hwndTarget == window_) {
            const RAWINPUTDEVICE removal{kDigitizer, kTouchpad, RIDEV_REMOVE, nullptr};
            if (!RegisterRawInputDevices(&removal, 1, sizeof(removal))) return;
        }
        inputRegistered_ = false;
    }
    bool touchpadInputSelected() const {
        return config_.enabled && config_.touchpadControls && (config_.observeOnly || config_.keyboardInterop);
    }
    bool touchpadBackendAvailable() const {
#ifdef PINCH_ZOOM_STANDALONE
        return true;
#else
        return config_.observeOnly || bridge_.gateAvailable();
#endif
    }
    void syncOptionalInputs() {
        if (touchpadInputSelected() && touchpadBackendAvailable()) {
            if (!inputRegistered_ && !registerTouchpadInput())
                log(L"Touchpad input unavailable; other zoom controls remain available.");
        } else {
            unregisterTouchpadInput();
            devices_.clear();
            waitForRelease_ = false;
        }

        mouseInput_.configure(config_, sessionActive_ && !paused_);
        if (!config_.mouseControls || !config_.managesZoom()) {
            mouseInput_.stop();
        } else if (!mouseInput_.start(windowClassModule_, window_, bridge_.get(), controllerStop_)) {
            log(L"Mouse shortcut hooks unavailable; native keyboard controls remain available.");
        }
    }
    bool syncCoreHotkeys() {
        if (!config_.enabled) {
            if (std::exchange(resetHotkeyRegistered_, false)) UnregisterHotKey(window_, kResetHotkey);
            if (std::exchange(pauseHotkeyRegistered_, false)) UnregisterHotKey(window_, kPauseHotkey);
            return true;
        }
        constexpr UINT modifiers = MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT;
        if (!resetHotkeyRegistered_) resetHotkeyRegistered_ = RegisterHotKey(window_, kResetHotkey, modifiers, VK_F10) != FALSE;
        if (!resetHotkeyRegistered_) {
            log(L"Emergency reset hotkey unavailable; desktop zoom could not start.");
            return false;
        }
        if (!pauseHotkeyRegistered_) pauseHotkeyRegistered_ = RegisterHotKey(window_, kPauseHotkey, modifiers, VK_F9) != FALSE;
        return true;
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
            dev.report.contacts.reserve(dev.slots.size());
            ++statistics_.devices;
            log(L"Compatible Precision Touchpad HID descriptor found.");
        }
        return dev.valid ? &dev : nullptr;
    }
    void frame(HANDLE handle, Device& dev, std::span<const pinch::Contact> contacts, ULONGLONG now, double elapsedMs) {
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
        if (paused_ || !sessionActive_ || !touchpadInputSelected() || !config_.keyboardInterop) {
            bridge_.publish(pinch::Phase::Idle, false, false);
            return;
        }
        if (waitForRelease_) {
            if (dev.activeContacts < 3) {
                waitForRelease_ = false;
            }
            return;
        }
        if (activeDevice_ && activeDevice_ != handle) return;
        render(now, false); // Classify this HID frame before delivering either pending input.
        const auto before = dev.recognizer.phase();
        if (dev.waitingForNative) {
            dev.recognizer.adoptZoom(zoom_.current());
            dev.waitingForNative = !bridge_.magnifierAvailable();
        }
        auto effective = config_;
        if (config_.keyboardInterop) effective.maxZoom = std::min(effective.maxZoom, 16.0);
        auto decision = dev.recognizer.update(contacts, now, zoom_.current(), effective, elapsedMs);
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
        if (auto* state = bridge_.get(); state && before == pinch::Phase::Pinch && phase != pinch::Phase::Pinch) {
            // Release pointer protection before processing any wheel input from before lift.
            InterlockedExchange64(&state->pinchFrameTime, 0);
            renderWheel(*state, now); // Fold input captured before the physical lift into this pinch's final target.
        }
        bridge_.publish(phase, dev.activeContacts == 3, !config_.observeOnly, now);
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
            if (!config_.observeOnly && phase == pinch::Phase::Pinch) {
                const double base = std::clamp(zoom_.pinchStartLevel(decision.beganEnlarging), 1.0, effective.maxZoom);
                cancelWheel();
                modeSuppression_.reset();
                pinchReachedNormal_ = false;
                dev.recognizer.adoptZoom(base);
                decision.zoom = base;
            }
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
        updateZoomMode();
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
            const auto* state = bridge_.get();
            // Further reports at the same floor must not overwrite the wheel
            // as the actor that reached it before the native update can land.
            const bool sameWheelFloor = state && !decision.began && std::llround(*decision.zoom * 10000) == 10000 &&
                state->requestedLevel == 10000 && state->requestInputRevision == state->inputRevision &&
                state->requestRevision == state->zoomRevision &&
                state->requestSource == static_cast<LONG>(bridge::InputSource::PinchWheel);
            if (!config_.observeOnly && !sameWheelFloor && !zoom_.target(*decision.zoom, decision.began)) {
                dev.recognizer.reset();
                releaseAbandonedPinch(handle);
            }
            // Apply on the input frame, without waiting for the 16 ms timer.
            if (!config_.observeOnly) render(now);
        } else if (auto* state = bridge_.get(); state && !bridge::pinchDown(*state)) {
            renderWheel(*state, now);
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
        auto& decoded = dev.report;
        decoded.contacts.clear();
        decoded.count = *contactCount;
        decoded.scan = dev.hasScan ? value(kDigitizer, dev.scanCollection, kScanTime) : std::nullopt;
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
                dev.assembler.reset();
                dev.recognizer.reset();
                releaseAbandonedPinch(handle);
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
            const double elapsed = dev.timing.advance(dev.frameScan, preciseMilliseconds(now));
            frame(handle, dev, *complete, now, elapsed);
        }
    }
    void rawInput(HRAWINPUT handle) {
        UINT size = 0;
        if (GetRawInputData(handle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0 ||
            size < offsetof(RAWINPUT, data.hid.bRawData)) {
            return;
        }
        if (size > rawInputBuffer_.size()) rawInputBuffer_.resize(size);
        const UINT received = GetRawInputData(handle, RID_INPUT, rawInputBuffer_.data(), &size, sizeof(RAWINPUTHEADER));
        if (received > rawInputBuffer_.size() || received < offsetof(RAWINPUT, data.hid.bRawData)) {
            return;
        }
        auto* raw = reinterpret_cast<RAWINPUT*>(rawInputBuffer_.data());
        if (raw->header.dwType != RIM_TYPEHID) {
            return;
        }
        ++statistics_.rawPackets;
        auto& hid = raw->data.hid;
        const auto payload = static_cast<std::size_t>(received) - offsetof(RAWINPUT, data.hid.bRawData);
        if (!hid.dwSizeHid || hid.dwCount > payload / hid.dwSizeHid) {
            return;
        }
        if (auto* dev = device(raw->header.hDevice)) {
            for (DWORD i = 0; i < hid.dwCount; ++i) {
                ++statistics_.hidReports;
                report(raw->header.hDevice, *dev,
                       reinterpret_cast<char*>(hid.bRawData) + i * hid.dwSizeHid,
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
                if (!host->zoomInputEnabled() || !host->config_.mouseControls) return 0;
                if (w == 1 || w == kResetArmedZoomAction) {
                    host->resetFromMouse();
                } else if (w == kToggleZoomAction) {
                    host->toggleZoomMode(GetTickCount64());
                } else if (w == kExitZoomAction) {
                    host->disarmZoomMode();
                    host->restartGestureTracking();
                } else if (host->config_.windowButtons && host->config_.winWheel && (w == 2 || w == 3)) {
                    HWND target = reinterpret_cast<HWND>(l);
                    if (target == GetAncestor(GetForegroundWindow(), GA_ROOT) && WindowSwitcher::eligible(target))
                        ShowWindowAsync(target, w == 3 ? SW_MINIMIZE : IsZoomed(target) ? SW_RESTORE : SW_MAXIMIZE);
                }
                return 0;
            case kWheelMessage:
                host->render(GetTickCount64());
                return 0;
            case WM_INPUT:
                if (host->inputRegistered_ && host->touchpadInputSelected())
                    host->rawInput(reinterpret_cast<HRAWINPUT>(l));
                // DefWindowProc is required to clean up foreground WM_INPUT.
                return DefWindowProcW(window, message, w, l);
            case WM_INPUT_DEVICE_CHANGE:
                if (!host->inputRegistered_ || !host->touchpadInputSelected()) return 0;
                host->devices_.erase(reinterpret_cast<HANDLE>(l));
                host->resetGestures();
                if (w == GIDC_REMOVAL) host->zoom_.restore();
                return 0;
            case WM_DISPLAYCHANGE:
                host->wheelFramePeriod_ = 0;
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
                host->disarmZoomMode();
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
            case kSettingsMessage: {
                if (!host->readSettings_) return 0;
                const auto next = host->readSettings_();
                if (next == host->config_) return 0;
                host->zoom_.end();
                host->restartGestureTracking();
                host->config_ = next;
                if (!host->syncCoreHotkeys()) host->config_.enabled = false;
                host->bridge_.configure(host->interactiveConfig());
                host->syncOptionalInputs();
                host->nativeLaunch_.resetAttempts();
                if (!host->zoomInputEnabled()) host->disarmZoomMode();
                else host->updateZoomMode();
                if (!host->config_.enabled || host->config_.observeOnly) host->zoom_.resetToNormal();
                host->bridge_.syncNative();
                return 0;
            }
            case kStopMessage:
            case WM_CLOSE:
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProcW(window, message, w, l);
    }

public:
    explicit Host(pinch::Config config, pinch::Config (*readSettings)() = nullptr)
        : config_(config), readSettings_(readSettings) {}
    bool initialize(HINSTANCE module, HANDLE controllerStop = nullptr) {
        if (!runtimeLease_.acquire(L"Local\\BetterMagnifier.HostOwner")) {
            log(L"A Better Visual host already owns this session.");
            return false;
        }
#ifdef PINCH_ZOOM_STANDALONE
        if (FindWindowW(kClassName, nullptr)) {
            log(L"The Windhawk host is already running. Use --status for an active installation.");
            return false;
        }
#endif
        if (!bridge_.open()) return false;
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        if (!zoom_.initialize()) {
            log(L"MagInitialize failed.");
            return false;
        }
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = windowProc;
        windowClass.hInstance = module;
        windowClass.lpszClassName = kClassName;
        if (!RegisterClassW(&windowClass)) return false;
        windowClassModule_ = module;
        // Hidden top-level window also receives session and power broadcasts.
        window_ = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, kClassName, L"Better Visual",
                                  WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, module, this);
        if (!window_) return false;
        controllerStop_ = controllerStop;
        if (!touchpadBackendAvailable()) nativeLaunch_.observe(false, false, true);
        // Keep an emergency reset available before allowing any zoom changes.
        if (!syncCoreHotkeys()) return false;
        if (!SetTimer(window_, kTimer, 16, nullptr)) return false;
        InterlockedExchange64(&bridge_.get()->hostWindow, reinterpret_cast<LONG64>(window_));
        refreshSession(GetTickCount64(), true);
        bridge_.configure(interactiveConfig());
        syncOptionalInputs();
        bridge_.publish(pinch::Phase::Pending, false, inputRegistered_ && zoomInputEnabled());
        lastTick_ = GetTickCount64();
        lastDiagnostic_ = lastTick_;
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
        log(config_.observeOnly ? L"Ready: observation only." : L"Ready: desktop zoom controller.");
        return true;
    }
    HWND window() const { return window_; }
    const Statistics& statistics() const { return statistics_; }
    void loop(HANDLE stop, unsigned seconds = 0) {
        const auto end = seconds ? GetTickCount64() + seconds * 1000ULL : 0;
        bool running = true;
        while (running) {
            const DWORD timeout = end ? static_cast<DWORD>(std::min<ULONGLONG>(1000, end > GetTickCount64() ? end - GetTickCount64() : 0)) : INFINITE;
            HANDLE signals[2]{};
            DWORD count = 0;
            if (stop) signals[count++] = stop;
            const DWORD wheelIndex = count;
            if (wheelFrameTimer_) signals[count++] = wheelFrameTimer_;
            const DWORD result = MsgWaitForMultipleObjectsEx(count, signals, timeout, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            if ((stop && result == WAIT_OBJECT_0) || result == WAIT_FAILED ||
                (end && GetTickCount64() >= end)) {
                break;
            }
            const bool wheelDue = wheelFrameTimer_ && result == WAIT_OBJECT_0 + wheelIndex;
            if (wheelDue) wheelFrameScheduled_ = false;
            MSG message{};
            for (unsigned processed = 0; processed < 64 && PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE); ++processed) {
                if (message.message == WM_QUIT) {
                    running = false;
                    break;
                }
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            if (running && wheelDue && !wheelFrameScheduled_ && wheel_.active()) render(GetTickCount64());
        }
    }
    ~Host() {
        mouseInput_.stop();
        auto* record = bridge_.get();
        if (window_ && record &&
            InterlockedCompareExchange64(&record->hostWindow, 0, 0) == reinterpret_cast<LONG64>(window_)) {
            disarmZoomMode();
            cancelWheel();
            bridge_.publish(pinch::Phase::Idle, false, false);
            InterlockedExchange(&record->controlEnabled, 0);
            InterlockedExchange64(&record->hostWindow, 0);
            bridge_.wake();
        }
        zoom_.restore();
        if (nativeProcess_) CloseHandle(nativeProcess_);
        if (wheelFrameTimer_) CloseHandle(wheelFrameTimer_);
        if (window_) {
            KillTimer(window_, kTimer);
            if (resetHotkeyRegistered_) UnregisterHotKey(window_, kResetHotkey);
            if (pauseHotkeyRegistered_) UnregisterHotKey(window_, kPauseHotkey);
            if (wtsRegistered_) {
                WTSUnRegisterSessionNotification(window_);
            }
            unregisterTouchpadInput();
            DestroyWindow(window_);
        }
        if (windowClassModule_) UnregisterClassW(kClassName, windowClassModule_);
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
        std::printf("Pointer protection: available=%d; blocked=%ld; lastOriginRva=0x%lx; threeFingersDown=%ld\n",
                    !!(record->nativeCaps & compatibility::NativePointerGuard),
                    record->blockedCursorMoves, static_cast<unsigned long>(record->lastCursorOrigin), record->down);
        std::printf("Compatibility: nativeCaps=0x%lx; gateCaps=0x%lx; nativeReason=%ld; gateReason=%ld\n", record->nativeCaps, record->gateCaps, record->resolverReasonNative, record->resolverReasonGate);
        std::printf("Resolvers: native=%ld; shell=%ld; desktop=%ld (1=offline verified, 2=symbols)\n", record->resolverModeNative, record->resolverModeGate, record->resolverModeDesktop);
        std::printf("Keyboard hold: progressiveSpeed=%ld; replacedRepeats=%ld; releaseFreezes=%ld\n", record->keyboardHoldMultiplier, record->keyboardHoldRepeats, record->keyboardHoldFreezes);
        std::printf("Mouse profile: winWheel=%ld; hooksReady=%ld; ctrlAltWindowWheel=%ld; nativeViewMode=%ld\n", record->winWheel, record->mouseHooksReady, record->wheelWindowSwitch, record->nativeViewMode);
        std::printf("Start menu mask: initialSent=%ld; initialFailed=%ld; releaseAttempted=%ld\n", record->menuMaskInitialSent, record->menuMaskInitialFailed, record->menuMaskReleaseSent);
        std::printf("Window wheel: attempts=%ld; confirmed=%ld; immediateFailures=%ld; deferredFailures=%ld\n", record->windowSwitchAttempts, record->windowSwitchConfirmed, record->windowSwitchRejected, record->windowSwitchDeferredFailed);
        std::printf("Read-from-point: disabled=%ld; verifiedHooks=%ld\n", record->disableReading, record->readingReady);
        std::printf("Wheel: enabled=%ld; active=%ld; nativeEvents=%ld; zoomMode=%ld\n", record->wheelEnabled, record->wheelActive, record->wheelEvents, record->zoomModeActive);
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
runtime::ZoomBorder::ReadMonitorTransform readMonitorTransform = nullptr;
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
// A Magnify.exe launch can be only a short IPC relay to another instance.
// Resolve hooks locally, but do not publish any native state until this process
// proves that it owns a real Magnifier UI window and acquires the process lease.
class NativeMapping : public bridge::Mapping {
    mutable bridge::Record staged_{};
    std::atomic<bool> owned_{false};
    std::atomic<DWORD> ownerPid_{0};
    HANDLE lease_ = nullptr;
    std::mutex leaseMutex_;
    bool initialNormalize_ = false;

    void seedStartupPreferences() {
        const auto* shared = sharedRecord();
        if (!shared) return;
        staged_.hideUi = shared->hideUi;
        staged_.keyboardStep = shared->keyboardStep;
        staged_.keyboardHoldMultiplier = shared->keyboardHoldMultiplier;
        staged_.maxZoomPercent = shared->maxZoomPercent;
        staged_.disableReading = shared->disableReading;
        staged_.startAtNormal = shared->startAtNormal;
        initialNormalize_ = staged_.startAtNormal != 0;
    }

    void publishClaim(DWORD pid) {
        auto* shared = sharedRecord();
        // Copy only native publisher metadata. Never copy host configuration,
        // pending requests, sources, sequences or accumulated statistics.
        InterlockedExchange(&shared->nativeCaps, staged_.nativeCaps);
        InterlockedExchange(&shared->resolverReasonNative, staged_.resolverReasonNative);
        InterlockedExchange(&shared->resolverModeNative, staged_.resolverModeNative);
        InterlockedExchange(&shared->nativeError, staged_.nativeError);
        InterlockedExchange(&shared->nativeViewMode, staged_.nativeViewMode);
        InterlockedExchange(&shared->readingReady, staged_.readingReady);
        if (staged_.nativeLevel >= 10000 && staged_.nativeLevel <= 160000)
            InterlockedExchange(&shared->nativeLevel, staged_.nativeLevel);
        if (initialNormalize_ && !staged_.startAtNormal)
            InterlockedCompareExchange(&shared->startAtNormal, 0, 1);
        InterlockedExchange(&shared->magnifierReady, 0);
        InterlockedIncrement(&shared->inputRevision);
        InterlockedIncrement(&shared->zoomRevision);
        ownerPid_.store(pid);
        InterlockedExchange(&shared->magnifierPid, static_cast<LONG>(pid));
        owned_.store(true);
    }

public:
    bool open() {
        if (!bridge::Mapping::open()) return false;
        // Read-only startup preferences may be needed before UI construction.
        // All gesture/wheel control remains disabled in this private record.
        seedStartupPreferences();
        return true;
    }
    bridge::Record* sharedRecord() const { return bridge::Mapping::get(); }
    bridge::Record* get() const { return owned_.load() ? sharedRecord() : &staged_; }
    bool owns() const { return owned_.load(); }
    DWORD ownerPid() const { return ownerPid_.load(); }
    void reaffirmPid() {
        if (owns() && sharedRecord()) InterlockedExchange(&sharedRecord()->magnifierPid, static_cast<LONG>(ownerPid()));
    }
    bool claim(HWND window) {
        std::lock_guard lock(leaseMutex_);
        if (owns()) { reaffirmPid(); return true; }
        DWORD pid = 0;
        wchar_t name[96]{};
        if (!sharedRecord() || !window || !GetWindowThreadProcessId(window, &pid) || pid != GetCurrentProcessId() ||
            !GetClassNameW(window, name, ARRAYSIZE(name)) ||
            (wcscmp(name, L"MagUIClass") && wcscmp(name, L"ScreenMagnifierUIWnd"))) return false;
        // Existence is the lease. Never own a mutex on Windhawk's temporary
        // injection thread, and never ReleaseMutex from a different thread.
        HANDLE candidate = CreateMutexW(nullptr, FALSE, L"Local\\KdePinchZoom.NativeOwner.v14");
        if (!candidate) return false;
        if (GetLastError() == ERROR_ALREADY_EXISTS) { CloseHandle(candidate); return false; }
        lease_ = candidate;
        publishClaim(pid);
        return true;
    }
    void retire(bool releaseLease) {
        std::lock_guard lock(leaseMutex_);
        if (owned_.exchange(false) && sharedRecord() &&
            static_cast<DWORD>(sharedRecord()->magnifierPid) == ownerPid())
            InterlockedExchange(&sharedRecord()->magnifierReady, 0);
        if (releaseLease && lease_) { CloseHandle(lease_); lease_ = nullptr; }
        // If a UI callback could not drain, its pinned DLL retains the lease
        // until process exit, preventing a newer instance from racing that code.
    }
    ~NativeMapping() { retire(true); }
#ifdef PINCH_ZOOM_NATIVE_TEST
    void useTestRecord(bridge::Record* record) {
        bridge::Mapping::useTestRecord(record);
        ownerPid_.store(GetCurrentProcessId()); owned_.store(true);
    }
    void useStagedTestRecord(bridge::Record* record) {
        bridge::Mapping::useTestRecord(record);
        staged_ = {}; initialNormalize_ = false; ownerPid_.store(0); owned_.store(false);
        seedStartupPreferences();
    }
    bool claimStagedTestRecord(DWORD pid, bool authenticUi, bool leaseAvailable) {
        if (!authenticUi || !leaseAvailable || !sharedRecord()) return false;
        if (!owns()) publishClaim(pid);
        return true;
    }
#endif
};
NativeMapping mapping;
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
    return mapping.owns() && readingHooksReady.load() && !stopping.load() && state && state->disableReading;
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
    // PDB-verified ABI for these exact images only:
    // FocusTracker::RegisterReadFromPointInput() void(this), and both private
    // MousePointerStateManager::SetReadFromPointMouseCursors...() bool(const this).
    // Keep these identities independent of the core profiles: updating a core
    // profile must not silently approve unreviewed reading ABI/offsets.
    struct ReadingProfile {
        binary_profiles::Identity identity;
        std::array<DWORD, 3> rvas;
    };
    constexpr ReadingProfile profiles[] = {
        {{{0x99,0x46,0x73,0xd3,0xf7,0xad,0x69,0xbc,0x0b,0x62,0xa7,0xf4,0x56,0x60,0x23,0x4f},
          1, 0x130000, 0x5a1425a3}, {0x72840, 0x898f8, 0x899bc}},
        {{{0xc5,0x46,0x12,0xd6,0x1f,0x4c,0x82,0xa8,0x21,0x05,0xd4,0x0b,0xd8,0xf3,0x63,0xde},
          1, 0x14c000, 0x88b3d475}, {0x76190, 0x8fdac, 0x8fe6c}},
    };
    const auto identity = binary_profiles::identify(module);
    if (!identity) return false;
    for (const auto& profile : profiles) {
        if (profile.identity != *identity) continue;
        for (const auto rva : profile.rvas) if (!binary_profiles::permitted(module, rva, false)) return false;
        for (std::size_t i = 0; i < targets.size(); ++i) targets[i] = reinterpret_cast<BYTE*>(module) + profile.rvas[i];
        return true;
    }
    return false;
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
bool useAuthoritativeUiLevel = false; // Exact native image with an independent per-monitor zoom lifecycle.
std::atomic<bool> hiding{true};
std::atomic<bool> visibilityPending{false};
std::mutex stylesMutex;
std::map<HWND, LONG_PTR> styles;
HANDLE stopEvent = nullptr;
HANDLE thread = nullptr;
HANDLE ownerProbeThread = nullptr;
std::atomic<bool> afterInitComplete{false};
std::atomic<bool> ownerWorkerStarted{false};
std::mutex workerLifecycle;
#ifdef PINCH_ZOOM_NATIVE_TEST
unsigned ownerWorkerStartCount = 0;
#endif
std::atomic<HWND> messageWindow{nullptr};
std::atomic<bool> stopping{false};
std::atomic<unsigned> activeUpdates{0};
constexpr wchar_t kNativeClass[] = L"KdePinchZoomNativeTarget.v18";
std::atomic<DWORD> uiThread{0};
std::atomic<void*> nativeSelf{nullptr}; // Dereferenced only on the native rendering thread.
std::atomic<bool> uiControlReady{false};
std::atomic<bool> uiPrimeBlocked{false}; // A failed UI prime waits for a real native callback/command.
thread_local bool applyingPinch = false;
thread_local bool applyingTouchpadPinch = false;
thread_local unsigned nativeCommandDepth = 0;
thread_local bridge::InputSource nativeCommandSource = bridge::InputSource::None;
thread_local bool nativeTargetObserved = false;
thread_local float nativeObservedTarget = 1;
// A renderer callback is not necessarily a new command: native animation can
// deliver it after a newer managed objective has already been accepted.
struct ManagedTargetLease {
    bridge::InputSource source = bridge::InputSource::None;
    LONG revision = 0;
    float target = 1;
    void* renderer = nullptr;
};
std::mutex managedTargetMutex;
std::optional<ManagedTargetLease> managedTarget;
bool observesNativeCommands() {
    const auto* state = mapping.get();
    return state && (state->nativeCaps & compatibility::NativeKeyboardObserver);
}
void releaseManagedTarget() {
    std::lock_guard lock(managedTargetMutex);
    managedTarget.reset();
}
void acceptManagedTarget(bridge::InputSource source, float level) {
    std::lock_guard lock(managedTargetMutex);
    const auto* state = mapping.get();
    if (!state || !observesNativeCommands() || stopping.load() || !nativeSelf ||
        !std::isfinite(level) || level < 1 || level > 16) {
        managedTarget.reset();
        return;
    }
    managedTarget = ManagedTargetLease{source, state->inputRevision, level, nativeSelf.load()};
}
bool isManagedTargetEcho(void* renderer) {
    std::lock_guard lock(managedTargetMutex);
    if (!managedTarget) return false;
    const auto* state = mapping.get();
    if (!state || !observesNativeCommands() || stopping.load() ||
        (!state->controlEnabled && managedTarget->source != bridge::InputSource::Reset) ||
        renderer != managedTarget->renderer || nativeSelf.load() != managedTarget->renderer) {
        managedTarget.reset();
        return false;
    }
    const LONG revision = state->inputRevision;
    if (revision != managedTarget->revision) {
        // A new pinch invalidates the previous epoch before its first target
        // reaches the UI thread. Keep blocking old managed callbacks in that
        // gap; explicit native commands release their leases independently.
        if (!bridge::pinchDown(*state) ||
            static_cast<std::uint32_t>(revision) - static_cast<std::uint32_t>(managedTarget->revision) != 1u) {
            managedTarget.reset();
            return false;
        }
        managedTarget->revision = revision;
    }
    return true;
}
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
#ifdef PINCH_ZOOM_NATIVE_TEST
decltype(&PostMessageW) postRecoveredPrime = PostMessageW;
#else
const auto postRecoveredPrime = PostMessageW;
#endif
void cancelKeyboardHold(bool seal = true);
bool freezeKeyboardHold();
void pollKeyboardHold();
const UINT updateMessage = RegisterWindowMessageW(L"KdePinchZoom.NativeTarget.v18");
constexpr UINT kClaimNativeOwner = WM_APP + 0x62;
constexpr UINT kRefreshZoomBorder = WM_APP + 0x63;
constexpr UINT kRefreshNativeVisibility = WM_APP + 0x64;
runtime::ZoomBorder zoomBorder;
std::atomic<runtime::ZoomBorder::Appearance> borderAppearance{runtime::ZoomBorder::Appearance{}};
bool wantsZoomBorder(bridge::Record& state, std::uint64_t now, bool hidden = false) {
    if (hidden || !state.zoomModeActive || !state.magnifierReady || !state.controlEnabled || !state.hostWindow) return false;
    const auto heartbeat = static_cast<std::uint64_t>(InterlockedCompareExchange64(&state.heartbeat, 0, 0));
    return heartbeat && (heartbeat >= now || now - heartbeat <= 250);
}
#ifndef PINCH_ZOOM_NATIVE_TEST
void refreshBorderSettings() {
    const auto value = Wh_GetStringSetting(L"zoomBorderColor");
    const runtime::ZoomBorder::Appearance next{
        Wh_GetIntSetting(L"hideZoomBorder") != 0, runtime::ZoomBorder::colorFromSetting(value)};
    if (value) Wh_FreeStringSetting(value);
    if (borderAppearance.exchange(next) == next) return;
    if (const HWND target = messageWindow.load()) PostMessageW(target, kRefreshZoomBorder, 0, 0);
}
#endif
LRESULT CALLBACK messages(HWND window, UINT message, WPARAM w, LPARAM l);
DWORD WINAPI worker(void*);
BOOL CALLBACK visibility(HWND, LPARAM);
void queueVisibility() {
    if (const HWND target = messageWindow.load(); target && visibilityPending.exchange(false) &&
        !PostMessageW(target, kRefreshNativeVisibility, 0, 0)) {
        visibilityPending.store(true);
    }
}
void setHiding(bool hidden) {
    if (hiding.exchange(hidden) != hidden) visibilityPending.store(true);
    queueVisibility();
}
bool nativeUiShouldBeHidden(const NativeMapping& owner) {
    auto* state = owner.sharedRecord();
    return owner.owns() && state &&
        static_cast<DWORD>(InterlockedCompareExchange(&state->magnifierPid, 0, 0)) == owner.ownerPid() &&
        InterlockedCompareExchange(&state->hideUi, 0, 0) &&
        InterlockedCompareExchange(&state->magnifierReady, 0, 0) &&
        InterlockedCompareExchange(&state->controlEnabled, 0, 0) &&
        owner.hostAvailable();
}
bool currentNativeUiShouldBeHidden() {
    return !stopping.load() && messageWindow.load() && uiControlReady.load() && nativeUiShouldBeHidden(mapping);
}
void refreshNativeVisibility() {
    setHiding(currentNativeUiShouldBeHidden());
}
bool suppressNativeVisibility() {
    return hiding.load() && currentNativeUiShouldBeHidden();
}
void activateNativeOwner() {
    std::lock_guard lifecycle(workerLifecycle);
    if (!mapping.owns() || !afterInitComplete.load() || !available || stopping.load()) return;
    bool expected = false;
    if (!ownerWorkerStarted.compare_exchange_strong(expected, true)) return;
#ifdef PINCH_ZOOM_NATIVE_TEST
    ++ownerWorkerStartCount; // No native thread/window actions in adapter tests.
#else
    refreshNativeVisibility();
    if (stopEvent) thread = CreateThread(nullptr, 0, worker, stopEvent, 0, nullptr);
    if (!thread) ownerWorkerStarted.store(false);
#endif
}
struct NativeUiSearch { DWORD pid; bool foreign; HWND result = nullptr; };
BOOL CALLBACK findNativeUiWindow(HWND window, LPARAM parameter) {
    auto& search = *reinterpret_cast<NativeUiSearch*>(parameter);
    DWORD pid = 0;
    wchar_t name[96]{};
    if (!GetWindowThreadProcessId(window, &pid) || (search.foreign ? pid == search.pid : pid != search.pid) ||
        !GetClassNameW(window, name, ARRAYSIZE(name))) return TRUE;
    if (wcscmp(name, L"MagUIClass") && wcscmp(name, L"ScreenMagnifierUIWnd")) return TRUE;
    search.result = window;
    return FALSE;
}
HWND findNativeUi(bool foreign = false) {
    NativeUiSearch search{GetCurrentProcessId(), foreign};
    EnumWindows(findNativeUiWindow, reinterpret_cast<LPARAM>(&search));
    return search.result;
}
bool claimNativeUi() {
    {
        std::lock_guard lifecycle(workerLifecycle);
        if (stopping.load()) return false;
        if (!mapping.owns()) {
            const HWND window = findNativeUi();
            if (!window || !mapping.claim(window)) return false;
        }
        mapping.reaffirmPid();
    }
    activateNativeOwner();
    return true;
}
void publish(float level) {
    if (mapping.get()) InterlockedExchange(&mapping.get()->nativeLevel, static_cast<LONG>(std::lround(level * 10000)));
}
// Option4 mode names in this verified image:1 Docked,2 Fullscreen,3 Lens
// (native dispatcher RVA7E858). Only the owner thread inspects native options.
// Control readiness does not require a fullscreen renderer callback at 100%.
bool refreshControlReady() {
    if (!claimNativeUi()) { uiControlReady.store(false); return false; }
    auto* state = mapping.get();
    const auto window = messageWindow.load();
    const auto ui = nativeUi.load();
    if (!state || stopping.load() || !ui || !window || uiThread.load() != GetCurrentThreadId() || !getKeeper || !getOption) return false;
    void* keeper = nullptr;
    if (FAILED(getKeeper(&keeper)) || !keeper) {
        uiControlReady.store(false);
        InterlockedExchange(&state->magnifierReady, 0);
        InterlockedExchange(&state->nativeError, ERROR_RETRY);
        refreshNativeVisibility();
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
    refreshNativeVisibility();
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
            zoomBorder.initialize(module, messageWindow.load(), kRefreshZoomBorder);
            refreshControlReady();
            queueVisibility();
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
    const bool observed = observesNativeCommands();
    const bool synchronousCommand = !applyingPinch && nativeCommandDepth != 0;
    if (!applyingPinch && !nativeCommandDepth && isManagedTargetEcho(self)) {
        // This setter physically retargets the native animator. Replaying an
        // old callback would reverse the accepted objective. OnMouse/MoveTo
        // still perform native mouse tracking without going through this path.
        return;
    }
    const bool awaitingPrime = observed && !applyingPinch && !nativeCommandDepth && uiPrimeBlocked.load() &&
                               uiThread.load() == GetCurrentThreadId();
    const bool unobservedCommand = !observed && !applyingPinch && !nativeCommandDepth;
    if (unobservedCommand) cancelKeyboardHold();
    // Without the optional observer preserve the original native fallback.
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
    if (synchronousCommand) {
        nativeTargetObserved = true;
        nativeObservedTarget = level;
    }
    originalChange(self, level, point); // Native animation retains ownership until managed takeover.
    auto* state = mapping.get();
    const bool recoveredPrime = awaitingPrime && !stopping.load() && std::isfinite(level) && level > 1 && level <= 16;
    if (synchronousCommand || unobservedCommand)
        InterlockedExchange(&state->currentSource, static_cast<LONG>(nativeCommandSource));
    if (!observed || applyingPinch || synchronousCommand || recoveredPrime) publish(level);
    rendererGeneration.fetch_add(1);
    if (unobservedCommand && std::abs(level * 10000.0 - before) >= 1) {
        InterlockedExchange(&mapping.get()->appliedSequence, mapping.get()->requestedSequence);
        InterlockedIncrement(&mapping.get()->zoomRevision);
        InterlockedIncrement(&mapping.get()->inputRevision);
        trackingInitialized.store(level > 1);
    }
    if (recoveredPrime) trackingInitialized.store(true);
    uiPrimeBlocked.store(false);
    const bool ready = refreshControlReady();
    // A queued request may have coalesced while the native UI was opening.
    // Resume through the normal mailbox so its current owner and revisions
    // are checked again on the UI thread before publishing a managed target.
    if (recoveredPrime && ready && state->controlEnabled && !state->requestWriting &&
        !state->pendingCommand && bridge::pendingLevel(*state)) {
        if (const HWND target = messageWindow.load()) postRecoveredPrime(target, updateMessage, 0, 0);
    }
}
long readOption(void* self, int name) {
    if (name == 20 && mapping.get() && mapping.get()->keyboardStep > 0) return mapping.get()->keyboardStep;
    return getOption(self, name); // Holding never changes the configured native increment.
}

bool holdPointer() {
    if (!keepPointerStable.load() || stopping.load() || nativeCommandDepth ||
        (applyingPinch && !applyingTouchpadPinch)) return false;
    auto* state = mapping.get();
    const auto now = GetTickCount64();
    return state && state->controlEnabled && state->enabled && bridge::pinchDown(*state) &&
        bridge::pinchFrameFresh(*state, now) &&
        now - static_cast<ULONGLONG>(InterlockedCompareExchange64(&state->heartbeat, 0, 0)) < 250;
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
    bridge::InputSource previousSource = nativeCommandSource;
    unsigned rendererBefore = rendererGeneration.load();
    explicit NativeCommandScope(bridge::InputSource source = bridge::InputSource::Keyboard) {
        if (!nativeCommandDepth) {
            nativeTargetObserved = false;
            nativeCommandSource = source;
        }
        ++nativeCommandDepth; applyingPinch = false;
    }
    ~NativeCommandScope() { applyingPinch = previous; nativeCommandSource = previousSource; --nativeCommandDepth; }
};
void nativeCommandComplete(unsigned /*rendererBefore*/) {
    if (applyingPinch || nativeCommandDepth != 1) return; // One epoch per outermost command.
    void* keeper = nullptr;
    if (FAILED(getKeeper(&keeper)) || !keeper) return;
    // A synchronous setter is an intentional command result, even if the UI
    // cache lags behind it. Only unscoped animation callbacks are stale echoes.
    const float level = nativeTargetObserved ? nativeObservedTarget : getOption(keeper, 14) / 100.0f;
    InterlockedExchange(&mapping.get()->currentSource, static_cast<LONG>(nativeCommandSource));
    publish(level);
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
    if (!nativeCommandDepth) { cancelKeyboardHold(); releaseManagedTarget(); }
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
    return state && !(state->nativeCaps & compatibility::NativeUiLevel) &&
        !stopping.load() && state->controlEnabled && state->magnifierReady &&
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
    acceptManagedTarget(bridge::InputSource::Keyboard, level);
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
    publish(level);
    acceptManagedTarget(bridge::InputSource::Keyboard, level);
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
    acceptManagedTarget(bridge::InputSource::Keyboard, static_cast<float>(level));
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
    releaseManagedTarget(); // Explicit native input wins before Windows handles it.
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
            if (bridge::enqueueWheel(state, delta, revision, GetTickCount64()))
                PostMessageW(reinterpret_cast<HWND>(InterlockedCompareExchange64(&state->hostWindow, 0, 0)),
                             runtime::kWheelMessage, 0, 0);
            InterlockedIncrement(&state->wheelEvents);
            return;
        }
    }
    releaseManagedTarget();
    resumeForNativeCommand();
    NativeCommandScope scope(bridge::InputSource::Wheel);
    originalWheel(self, delta); // Lens, opt-out and an absent host retain native behavior.
    nativeCommandComplete(scope.rendererBefore);
}

void option(void* self, int name, long value, bool save) {
    if (applyingPinch) save = false; // No registry writes for every HID frame.
    const long previousMode = name == 4 ? getOption(self, 4) : 0;
    originalOption(self, name, value, save);
    if (name == 4 && mapping.get() && previousMode != getOption(self, 4)) {
        cancelKeyboardHold();
        releaseManagedTarget();
        // Mode changes can destroy the fullscreen renderer. Do not keep using it.
        InterlockedExchange(&mapping.get()->magnifierReady, 0);
        nativeSelf = nullptr;
        rendererThread.store(0);
        uiControlReady.store(false);
        uiPrimeBlocked.store(false);
        if (uiThread.load() == GetCurrentThreadId()) refreshControlReady();
        else refreshNativeVisibility();
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
    acceptManagedTarget(suspend ? bridge::InputSource::Reset : static_cast<bridge::InputSource>(state->currentSource),
                        state->nativeLevel / 10000.0f);
    InterlockedExchange(&state->shellApplied, state->shellRevision);
    return true;
}
LRESULT CALLBACK messages(HWND window, UINT message, WPARAM w, LPARAM l) {
    if (message == kClaimNativeOwner) {
        struct ActiveClaim {
            ActiveClaim() { activeUpdates.fetch_add(1); }
            ~ActiveClaim() { activeUpdates.fetch_sub(1); }
        } active;
        if (!stopping.load()) refreshControlReady();
        return 0;
    }
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
    if (message == kRefreshZoomBorder || (message == WM_TIMER && w == runtime::ZoomBorder::kTimer)) {
        struct ActiveBorder {
            ActiveBorder() { activeUpdates.fetch_add(1); }
            ~ActiveBorder() { activeUpdates.fetch_sub(1); }
        } active;
        if (!stopping.load()) {
            auto* state = mapping.get();
            const auto appearance = borderAppearance.load();
            const auto update = message == kRefreshZoomBorder ? static_cast<runtime::ZoomBorder::Update>(w)
                                                             : runtime::ZoomBorder::Update::Transform;
            const bool expectsMonitorZoom = state && (state->nativeCaps & compatibility::NativeUiLevel) &&
                                            state->nativeLevel > 10000;
            zoomBorder.refresh(state && wantsZoomBorder(*state, GetTickCount64(), appearance.hidden),
                               readTransform, update, appearance.color, readMonitorTransform, expectsMonitorZoom);
        }
        return 0;
    }
    if (message == kRefreshNativeVisibility) {
        struct ActiveVisibility {
            ActiveVisibility() { activeUpdates.fetch_add(1); }
            ~ActiveVisibility() { activeUpdates.fetch_sub(1); }
        } active;
        if (!stopping.load() && mapping.owns()) EnumWindows(visibility, suppressNativeVisibility() ? 1 : 0);
        return 0;
    }
    if (message == WM_CLOSE) {
        cancelKeyboardHold();
        const bool reentrant = activeUpdates.load() != 0;
        DestroyWindow(window);
        return reentrant ? 1 : 0;
    }
    if (message == WM_NCDESTROY) {
        cancelKeyboardHold();
        zoomBorder.close();
        if (stopping.load()) EnumWindows(visibility, 0);
        messageWindow.store(nullptr);
        return DefWindowProcW(window, message, w, l);
    }
    if (message == updateMessage) {
        if (stopping.load()) return 0;
        struct ActiveUpdate {
            ActiveUpdate() { activeUpdates.fetch_add(1); }
            ~ActiveUpdate() { activeUpdates.fetch_sub(1); }
        } active;
        auto* state = mapping.get();
        if (w == 1) { updateOverview(); return 0; }
        const LONG sequence = InterlockedCompareExchange(&state->requestedSequence, 0, 0);
        const LONG previousAck = InterlockedCompareExchange(&state->appliedSequence, 0, 0);
        const LONG action = InterlockedExchange(&state->pendingCommand, 0);
        const auto source = action == 3 ? bridge::InputSource::Reset : action ? bridge::InputSource::Keyboard :
            static_cast<bridge::InputSource>(InterlockedCompareExchange(&state->requestSource, 0, 0));
        const LONG requestLevel = state->requestedLevel;
        const LONG requestRevision = state->requestRevision;
        const LONG requestInputRevision = state->requestInputRevision;
        const auto sameRequest = [&] {
            return !state->requestWriting && state->requestedSequence == sequence;
        };
        const auto stillOwnsInput = [&] {
            return sameRequest() && state->inputRevision == requestInputRevision &&
                   (source != bridge::InputSource::Wheel || !bridge::pinchDown(*state));
        };
        const auto acknowledge = [&] { InterlockedCompareExchange(&state->appliedSequence, sequence, previousAck); };
        void* keeper = nullptr;
        if ((action || sequence != previousAck) && (state->controlEnabled || action == 3)) {
            if (!action && !sameRequest()) return 0;
            const bool stale = !action && (requestRevision != state->zoomRevision || !stillOwnsInput());
            const bool explicitSource = bridge::pinchSource(source) || source == bridge::InputSource::Wheel ||
                                        source == bridge::InputSource::Keyboard || source == bridge::InputSource::Reset;
            if (stale || !explicitSource) {
                acknowledge();
                return 0; // Old input cannot dismiss overview, cancel another actor or acquire ownership.
            }
            if (!nativeUi || !messageWindow || uiThread.load() != GetCurrentThreadId()) {
                InterlockedExchange(&state->nativeError, ERROR_INVALID_THREAD_ID);
                return 0;
            }
            if (FAILED(getKeeper(&keeper)) || !keeper) {
                uiControlReady.store(false);
                InterlockedExchange(&state->magnifierReady, 0);
                InterlockedExchange(&state->nativeError, ERROR_RETRY);
                refreshNativeVisibility();
                return 0;
            }
            if (getOption(keeper, 4) != 2) {
                uiControlReady.store(false);
                InterlockedExchange(&state->magnifierReady, 0);
                InterlockedExchange(&state->nativeError, ERROR_NOT_SUPPORTED);
                refreshNativeVisibility();
                return 0;
            }
            if (uiPrimeBlocked.load() && !nativeSelf && action != 3) {
                InterlockedExchange(&state->nativeError, ERROR_RETRY);
                return 0; // No repeated setters for an unacknowledged cold request.
            }
            // Native UI queries above can reenter and accept a newer actor.
            if (!action && (!stillOwnsInput() || requestRevision != state->zoomRevision)) {
                if (sameRequest()) acknowledge();
                return 0;
            }
            // Only an objective validated before restoration can override the
            // temporary overview. Our restoration itself may advance epochs.
            cancelKeyboardHold();
            if (source == bridge::InputSource::Reset) {
                InterlockedExchange(&state->wheelActive, 0);
                InterlockedExchange64(&state->wheelPacket, 0);
            }
            if (source == bridge::InputSource::Reset && overviewSuspended.load()) {
                InterlockedExchange(&state->overviewBypassed, 1);
                overviewSuspended.store(false);
                overviewView.reset();
                InterlockedExchange(&state->overviewSavedLevel, 0);
            } else {
                resumeForNativeCommand(true);
            }
            // The authorized overview restoration above may change zoomRevision.
            if (!action && !stillOwnsInput()) {
                if (sameRequest()) acknowledge();
                return 0;
            }
            double level = requestLevel / 10000.0;
            if (action) {
                const double current = getOption(keeper, 14) / 100.0;
                const double step = getOption(keeper, 20) / 100.0;
                level = action == 3 ? 1 : current + (action == 1 ? step : -step);
            }
            const double cap = std::clamp(state->maxZoomPercent / 100.0, 1.0, 16.0);
            level = std::clamp(level, 1.0, cap);
            const bool hasRenderer = nativeSelf != nullptr;
            if (!useAuthoritativeUiLevel && hasRenderer && rendererThread.load() != GetCurrentThreadId()) {
                InterlockedExchange(&state->nativeError, ERROR_INVALID_THREAD_ID);
                return 0;
            }
            const double previous = state->nativeLevel / 10000.0;
            const bool prime = level > 1 && (!hasRenderer || !trackingInitialized.load() || previous <= 1);
            const bool reset = level <= 1 && (previous > 1 || action == 3);
            const long uiPercent = useAuthoritativeUiLevel
                ? std::max(level > 1 ? 101L : 100L, (std::lround(level * 10000) + 50) / 100) : 0;
            double delivered = useAuthoritativeUiLevel ? uiPercent / 100.0 : level;
            if (prime || reset || std::abs(delivered - previous) >= 0.00005) {
                struct Applying {
                    bool previous = applyingPinch;
                    bool previousTouchpad = applyingTouchpadPinch;
                    explicit Applying(bridge::InputSource source) {
                        applyingPinch = true;
                        applyingTouchpadPinch = bridge::pinchSource(source);
                    }
                    ~Applying() { applyingPinch = previous; applyingTouchpadPinch = previousTouchpad; }
                } applying(source);
                if (applyingTouchpadPinch && originalClipCursor && holdPointer()) originalClipCursor(nullptr);
                long percent = static_cast<long>(std::lround(level * 100));
                if (prime) percent = std::max(101L, percent);
                if (useAuthoritativeUiLevel) {
                    // The verified native UI dispatches through the active
                    // viewport and needs the previous option for its transition.
                    const auto expectedUi = nativeUi.load();
                    const auto expectedWindow = messageWindow.load();
                    releaseManagedTarget(); // Let the UI finish its deferred renderer callbacks.
                    InterlockedExchange(&state->startAtNormal, 0);
                    if (reset) uiPrimeBlocked.store(false);
                    setLevel(expectedUi, uiPercent);
                    if (stopping.load() || nativeUi.load() != expectedUi || messageWindow.load() != expectedWindow ||
                        (!action && !stillOwnsInput())) return 0;
                    if (getOption(keeper, 14) != uiPercent) {
                        InterlockedExchange(&state->nativeError, ERROR_RETRY);
                        return 0; // The active UI did not accept this level.
                    }
                    trackingInitialized.store(delivered > 1 && nativeSelf);
                } else if (prime || reset) {
                    // UI owns the first transition even when the fullscreen
                    // window/renderer has not yet been observed at cold 100%.
                    InterlockedExchange(&state->startAtNormal, 0);
                    if (reset) uiPrimeBlocked.store(false);
                    originalOption(keeper, 14, percent, false);
                    const unsigned generationBeforeUi = rendererGeneration.load();
                    setLevel(nativeUi, percent);
                    delivered = percent / 100.0;
                    if (prime && (!nativeSelf || rendererThread.load() != GetCurrentThreadId())) {
                        uiPrimeBlocked.store(true);
                        uiControlReady.store(false);
                        trackingInitialized.store(false);
                        InterlockedExchange(&state->magnifierReady, 0);
                        InterlockedExchange(&state->nativeError, ERROR_RETRY);
                        refreshNativeVisibility();
                        return 0; // Keep sequence unacknowledged; no NULL call or automatic retry loop.
                    }
                    const bool rendererDeferred = nativeSelf && rendererGeneration.load() == generationBeforeUi;
                    const bool precisePrime = prime && !action && source != bridge::InputSource::Reset &&
                                              std::abs(delivered - level) >= 0.000001;
                    if (rendererDeferred || precisePrime) {
                        // Native UI can queue the renderer callback in either
                        // direction. Deliver it before accepting the lease,
                        // which would otherwise filter that same callback.
                        if (stopping.load() || !messageWindow || rendererThread.load() != GetCurrentThreadId()) {
                            InterlockedExchange(&state->nativeError, ERROR_INVALID_THREAD_ID);
                            return 0;
                        }
                        originalChange(nativeSelf, static_cast<float>(precisePrime ? level : delivered), nullptr);
                    }
                    if (precisePrime) {
                        originalOption(keeper, 14, static_cast<long>(std::lround(level * 100)), false);
                        delivered = level;
                    }
                    trackingInitialized.store(prime && nativeSelf);
                } else {
                    if (!nativeSelf || rendererThread.load() != GetCurrentThreadId()) {
                        InterlockedExchange(&state->nativeError, ERROR_RETRY);
                        return 0;
                    }
                    originalOption(keeper, 14, percent, false);
                    originalChange(nativeSelf, static_cast<float>(level), nullptr);
                }
            }
            InterlockedExchange(&state->currentSource, static_cast<LONG>(source));
            publish(static_cast<float>(delivered));
            if (prime) InterlockedIncrement(&state->zoomRevision);
            InterlockedExchange(&state->nativeError, 0);
            acknowledge();
            if (action) { InterlockedIncrement(&state->nativeCommands); InterlockedIncrement(&state->zoomRevision); InterlockedIncrement(&state->inputRevision); }
            if (!useAuthoritativeUiLevel)
                acceptManagedTarget(action == 3 ? bridge::InputSource::Reset : source, static_cast<float>(delivered));
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
BOOL CALLBACK visibility(HWND window, LPARAM hideWindows) {
    if (!isUi(window)) return TRUE;
    if (hideWindows) {
        if (suppressNativeVisibility()) hide(window);
    }
    else {
        std::optional<LONG_PTR> saved;
        {
            std::lock_guard lock(stylesMutex);
            if (auto found = styles.find(window); found != styles.end()) { saved = found->second; styles.erase(found); }
        }
        if (saved) {
            SetWindowLongPtrW(window, GWL_EXSTYLE, *saved);
            wchar_t name[128]{};
            GetClassNameW(window, name, ARRAYSIZE(name));
            if (wcscmp(name, L"MagUIClass") == 0 || wcscmp(name, L"ScreenMagnifierUIWnd") == 0)
                ShowWindow(window, SW_SHOWNOACTIVATE);
        }
    }
    return TRUE;
}
BOOL WINAPI show(HWND window, int command) {
    return originalShow(window, suppressNativeVisibility() && isUi(window) ? SW_HIDE : command);
}
BOOL WINAPI position(HWND window, HWND after, int x, int y, int width, int height, UINT flags) {
    if (suppressNativeVisibility() && isUi(window)) flags = (flags & ~SWP_SHOWWINDOW) | SWP_HIDEWINDOW;
    return originalPosition(window, after, x, y, width, height, flags);
}
LONG_PTR WINAPI style(HWND window, int index, LONG_PTR value) {
    if (index == GWL_EXSTYLE && suppressNativeVisibility() && isUi(window)) {
        { std::lock_guard lock(stylesMutex); styles.try_emplace(window, GetWindowLongPtrW(window, GWL_EXSTYLE)); }
        value = (value & ~WS_EX_APPWINDOW) | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    }
    return originalStyle(window, index, value);
}
HWND WINAPI create(DWORD ex, LPCWSTR name, LPCWSTR title, DWORD flags, int x, int y, int width, int height,
                   HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter) {
    const bool named = reinterpret_cast<ULONG_PTR>(name) > 0xFFFF;
    if (named &&
        (wcscmp(name, L"MagUIClass") == 0 || wcscmp(name, L"ScreenMagnifierUIWnd") == 0 ||
         (title && wcsstr(title, L"Magnifier Touch"))) && suppressNativeVisibility()) flags &= ~WS_VISIBLE;
    const HWND window = originalCreate(ex, name, title, flags, x, y, width, height, parent, menu, instance, parameter);
    if (window && named && (wcscmp(name, L"MagUIClass") == 0 || wcscmp(name, L"ScreenMagnifierUIWnd") == 0)) {
        // Defer recovery until native window construction has completed.
        if (const HWND target = messageWindow.load()) PostMessageW(target, kClaimNativeOwner, 0, 0);
        PostMessageW(window, WM_NULL, 0, 0);
    }
    if (window && suppressNativeVisibility() && isUi(window)) hide(window);
    return window;
}

void postNativeUpdate(bridge::Record& state, HWND target, LONG& last, LONG& lastCommand,
                      decltype(&PostMessageW) post = PostMessageW) {
    if (!target) return;
    const LONG sequence = InterlockedCompareExchange(&state.requestedSequence, 0, 0);
    const LONG command = InterlockedCompareExchange(&state.syncRequest, 0, 0);
    if (sequence == last && command == lastCommand) return;

    // The producer increments the sequence before clearing its write marker.
    const bool writing = InterlockedCompareExchange(&state.requestWriting, 0, 0) != 0;
    const bool pendingCommand = InterlockedCompareExchange(&state.pendingCommand, 0, 0) != 0;
    if ((pendingCommand || (sequence != last && sequence && !writing)) && post(target, updateMessage, 0, 0)) {
        if (!writing) last = sequence;
        lastCommand = command;
    } else if (sequence == last && !pendingCommand) {
        lastCommand = command;
    }
}

DWORD WINAPI worker(void* stopParameter) {
    const HANDLE localStop = static_cast<HANDLE>(stopParameter);
    LONG last = mapping.get()->appliedSequence, lastCommand = -1;
    bool lastBorder = false;
    HWND lastBorderTarget = nullptr;
    const HANDLE events[] = {localStop, mapping.wakeEvent()};
    for (;;) {
        const DWORD result = events[1] ? WaitForMultipleObjects(2, events, FALSE, 250) : WaitForSingleObject(localStop, 4);
        if (result == WAIT_OBJECT_0 || result == WAIT_FAILED) break;
        auto* state = mapping.get();
        if (!state) continue;
        refreshNativeVisibility();
        const HWND target = messageWindow.load();
        const bool border = wantsZoomBorder(*state, GetTickCount64(), borderAppearance.load().hidden);
        if (target && (lastBorderTarget != target || lastBorder != border) &&
            PostMessageW(target, kRefreshZoomBorder, FALSE, 0)) {
            lastBorder = border;
            lastBorderTarget = target;
        }
        const bool shouldSuspend = !state->overviewBypassed && overviewAvailable() && state->controlEnabled && state->normalInOverview && (state->shellMask & 1) != 0 && (overviewSuspended.load() || !state->pendingGestures);
        if (target && (overviewAvailable() || overviewSuspended.load()) &&
            (state->shellApplied != state->shellRevision || shouldSuspend != overviewSuspended.load()))
            PostMessageW(target, updateMessage, 1, 0);
        // Coalesce pending HID updates; process them on Windows' own UI thread.
        postNativeUpdate(*state, target, last, lastCommand);
    }
    return 0;
}
DWORD WINAPI probeNativeUiOwner(void* stopParameter) {
    const HANDLE localStop = static_cast<HANDLE>(stopParameter);
    // No reading of native C++ objects on this background thread. Supported
    // instances marshal the claim to their UI helper; unsupported images only
    // publish their static fallback metadata after a real UI is found.
    for (unsigned attempt = 0; attempt < 100 && !stopping.load(); ++attempt) {
        if (mapping.owns()) return 0;
        if (const HWND nativeWindow = findNativeUi()) {
            if (!available) {
                std::lock_guard lifecycle(workerLifecycle);
                if (stopping.load()) return 0;
                if (mapping.claim(nativeWindow)) return 0;
            } else if (const HWND target = messageWindow.load()) {
                if (PostMessageW(target, kClaimNativeOwner, 0, 0)) return 0;
            } else {
                PostMessageW(nativeWindow, WM_NULL, 0, 0);
            }
        } else if (findNativeUi(true)) {
            return 0; // A relay with no own UI has nothing to publish.
        }
        if (!localStop || WaitForSingleObject(localStop, 50) != WAIT_TIMEOUT) break;
    }
    return 0;
}
#ifndef PINCH_ZOOM_NATIVE_TEST
bool initialize() {
    // Windows may start this executable only to relay Win-plus to its existing
    // UI. Such a process must not install hooks or reset the owner's mapping.
    if (!findNativeUi() && findNativeUi(true)) return false;
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
        InterlockedExchange(&mapping.get()->resolverModeNative, 0);
        InterlockedExchange(&mapping.get()->resolverReasonNative, ERROR_NOT_SUPPORTED);
        InterlockedExchange(&mapping.get()->nativeError, ERROR_NOT_SUPPORTED);
        return false; // No hook was attempted: native Windows remains untouched.
    };
    const HMODULE module = GetModuleHandleW(nullptr);
    constexpr const wchar_t* names[]{
        L"?ChangeMagnificationFactor@FullscreenMagnifier@@AEAAXMPEAVCPoint@@@Z",
        L"?SetOption@OptionsKeeper@@QEAAXW4OPTIONNAME@@J_N@Z",
        L"?GetOption@OptionsKeeper@@QEBAJW4OPTIONNAME@@@Z",
        L"?GetOptionsKeeper@OptionsKeeper@@SAJPEAPEAV1@@Z",
        L"?Run@MagnifierUI@@UEAAXXZ",
        L"?HandleSetZoomLevel@MagnifierUI@@AEAAXJ@Z",
        L"?HandleZoomCommand@MagnifierUI@@UEAAXH@Z",
        L"?MoveMouse@Magnifier@@IEAAXVCPoint@@@Z",
        L"?MoveTo@FullscreenMagnifier@@AEAAXVCRect@@_N@Z",
        L"?HandleMouseWheelZoom@MagnifierUI@@AEAAXH@Z",
    };
    void* targets[12]{};
    const bool profiled = binary_profiles::resolve(module, binary_profiles::Kind::Magnifier, targets, std::size(targets));
    useAuthoritativeUiLevel = profiled && binary_profiles::identify(module) == binary_profiles::magnify9949;
    if (!profiled) {
        // Static WndProc names do not prove the window data layout.
        void* symbols[std::size(names)]{};
        if (!binary_profiles::canUseSymbols() || !binary_profiles::resolveSymbols(module, names, symbols))
            return unsupported();
        std::copy(std::begin(symbols), std::end(symbols), std::begin(targets));
    }
    InterlockedExchange(&mapping.get()->resolverModeNative, profiled ? 1 : 2);
    std::uint32_t found = 0;
    for (unsigned i = 0; i < std::size(targets); ++i) if (targets[i]) found |= compatibility::symbol(i);
    const auto plan = compatibility::nativePlan(found);
    if (!plan.supported) {
        Wh_Log(L"Native core/bootstrap unavailable; retaining native Windows shortcuts and wheel.");
        return unsupported();
    }
    // Magnify.exe imports this DLL. Its reference covers the native callbacks,
    // including a UI helper that may finish after the mod is disabled.
    const HMODULE api = GetModuleHandleW(L"Magnification.dll");
    if (!api) return unsupported();
    readTransform = reinterpret_cast<ReadTransform>(GetProcAddress(api, "MagGetFullscreenTransform"));
    if (!readTransform) return unsupported();
    if (useAuthoritativeUiLevel) {
        readMonitorTransform = runtime::ZoomBorder::resolveMonitorTransform();
        if (!readMonitorTransform) Wh_Log(L"Per-monitor zoom border transform is unverified on this Windows image.");
    }
    moveTo = reinterpret_cast<MoveTo>(targets[8]);
    getOption = reinterpret_cast<GetOption>(targets[2]);
    getKeeper = reinterpret_cast<GetKeeper>(targets[3]);
    hiding.store(false); // A staged process has not proved ownership of any UI.
    keepPointerStable.store(Wh_GetIntSetting(L"keepPointerStable") != 0);
    hooksAttempted = true;
    // Missing optional groups are skipped before any member is installed. An
    // actual hook failure is fatal, so Windhawk rolls back this initialization.
    const bool installed =
        Wh_SetFunctionHook(targets[5], reinterpret_cast<void*>(uiLevel), reinterpret_cast<void**>(&setLevel)) &&
        (!(plan.capabilities & compatibility::NativePointerGuard) ||
            (Wh_SetFunctionHook(targets[7], reinterpret_cast<void*>(moveMouse), reinterpret_cast<void**>(&originalMoveMouse)) &&
             Wh_SetFunctionHook(reinterpret_cast<void*>(ClipCursor), reinterpret_cast<void*>(clipCursor), reinterpret_cast<void**>(&originalClipCursor)))) &&
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
    capabilities = static_cast<LONG>(plan.capabilities | (useAuthoritativeUiLevel ? compatibility::NativeUiLevel : 0u));
    InterlockedExchange(&mapping.get()->nativeCaps, capabilities);
    InterlockedExchange(&mapping.get()->resolverReasonNative, ERROR_SUCCESS);
    InterlockedExchange(&mapping.get()->nativeError, ERROR_SUCCESS);
    Wh_Log(L"Native Magnifier capabilities: 0x%lx", capabilities);
    return true;
}
#endif
void afterInit() {
    if (!mapping.sharedRecord()) return;
    bool first = false;
    {
        std::lock_guard lifecycle(workerLifecycle);
        if (stopping.load()) return;
        first = !afterInitComplete.exchange(true);
#ifndef PINCH_ZOOM_NATIVE_TEST
        if (first) stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (first && !mapping.owns() && stopEvent)
            ownerProbeThread = CreateThread(nullptr, 0, probeNativeUiOwner, stopEvent, 0, nullptr);
#else
        (void)first;
#endif
    }
#ifndef PINCH_ZOOM_NATIVE_TEST
    if (first && !stopping.load()) EnumWindows(wakeExistingTree, 0);
#endif
    activateNativeOwner();
}
void stop() {
    releaseManagedTarget();
    HANDLE localStop = nullptr, localProbe = nullptr, localThread = nullptr;
    {
        std::lock_guard lifecycle(workerLifecycle);
        stopping.store(true);
        mapping.retire(false); // Every later callback is redirected to the shadow.
        localStop = stopEvent; localProbe = ownerProbeThread; localThread = thread;
        stopEvent = nullptr; ownerProbeThread = nullptr; thread = nullptr;
        if (localStop) SetEvent(localStop);
    }
    // Never join while holding workerLifecycle: a probe can be finishing its
    // guarded claim and a UI callback can be completing activation.
    if (localProbe) { WaitForSingleObject(localProbe, INFINITE); CloseHandle(localProbe); }
    if (localThread) { WaitForSingleObject(localThread, INFINITE); CloseHandle(localThread); }
    hiding.store(false);
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
        const bool received = SendMessageTimeoutW(target, WM_CLOSE, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 500, &result) != 0;
        drained = received && result == 0;
        if (!received) PostMessageW(target, WM_CLOSE, 0, 0);
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
    if (localStop) CloseHandle(localStop);
    mapping.retire(drained);
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
runtime::SessionLease shellLease;
HMODULE shellModule = nullptr;

void releaseShellModule() {
    if (const auto module = std::exchange(shellModule, nullptr)) FreeLibrary(module);
}
bool initialize() {
    if (!shellLease.acquire(L"Local\\BetterMagnifier.ShellOwner")) return false;
    if (!mapping.open()) return false;
    symbolUnsupported = false;
    hooksAttempted = false;
    shell = false;
    capabilities = 0;
    bridge::desktopRaised = nullptr;
    const auto unsupported = [] {
        releaseShellModule();
        symbolUnsupported = true;
        InterlockedExchange(&mapping.get()->gateReady, 0);
        InterlockedExchange(&mapping.get()->gateCaps, 0);
        InterlockedExchange(&mapping.get()->resolverModeGate, 0);
        InterlockedExchange(&mapping.get()->resolverModeDesktop, 0);
        InterlockedExchange(&mapping.get()->resolverReasonGate, ERROR_NOT_SUPPORTED);
        return false;
    };
    shellModule = LoadLibraryExW(L"twinui.pcshell.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    // Retain this module until Windhawk removes the hooks.
    const HMODULE module = shellModule;
    if (!module) { Wh_Log(L"Could not load System32 twinui.pcshell.dll: %lu", GetLastError()); return unsupported(); }
    constexpr const wchar_t* names[]{
        L"?StartSwipe@MultitaskingViewGestureHandler@@UEAAJHHIUTOUCHPAD_GESTURE_CONTEXT@@@Z",
        L"?ContinueSwipe@MultitaskingViewGestureHandler@@UEAAJHHHHI@Z",
        L"?FinishSwipe@MultitaskingViewGestureHandler@@UEAAJHHHHI@Z",
        L"?CancelSwipe@MultitaskingViewGestureHandler@@UEAAJXZ",
        L"?ViewShown@MultitaskingViewVisibilityService@@UEAAXW4MULTITASKING_VIEW_TYPES@@@Z",
        L"?ViewDismissed@MultitaskingViewVisibilityService@@UEAAXW4MULTITASKING_VIEW_TYPES@@@Z",
    };
    void* targets[std::size(names)]{};
    const bool profiled = binary_profiles::resolve(module, binary_profiles::Kind::Shell, targets, std::size(targets));
    if (!profiled && (!binary_profiles::canUseSymbols() || !binary_profiles::resolveSymbols(module, names, targets)))
        return unsupported();
    InterlockedExchange(&mapping.get()->resolverModeGate, profiled ? 1 : 2);
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
    constexpr const wchar_t* desktopNames[]{L"?g_fDesktopRaised@@3HA"};
    if (desktopProfiled ||
        (binary_profiles::canUseSymbols() && binary_profiles::resolveSymbols(explorerModule, desktopNames, desktop, true))) {
        bridge::desktopRaised = static_cast<volatile LONG*>(desktop[0]);
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
    if (auto* record = mapping.get(); record && record->shellPid == static_cast<LONG>(GetCurrentProcessId())) {
        InterlockedExchange(&record->gateReady, 0);
        InterlockedExchange(&record->shellPid, 0);
    }
    releaseShellModule();
    shellLease.release();
}
#endif  // PINCH_ZOOM_GATE_TEST
}  // namespace native_gate

#ifndef PINCH_ZOOM_GATE_TEST
namespace controller {
HANDLE stopEvent = nullptr;
HANDLE thread = nullptr;
std::atomic<HWND> window{nullptr};
std::mutex settingsMutex;
pinch::Config latestSettings;

pinch::Config settings() {
    pinch::Config config;
    config.enabled = Wh_GetIntSetting(L"enabled") != 0;
    config.touchpadControls = Wh_GetIntSetting(L"disableTouchpad") == 0;
    config.mouseControls = Wh_GetIntSetting(L"disableMouse") == 0;
    config.mouseChord = Wh_GetIntSetting(L"disableMouseChord") == 0;
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
    if (const int step = Wh_GetIntSetting(L"wheelStep"); step > 0) config.wheelStep = std::min(step, 100);
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


pinch::Config currentSettings() {
    std::lock_guard lock(settingsMutex);
    return latestSettings;
}
void reloadSettings() {
    const auto next = settings();
    {
        std::lock_guard lock(settingsMutex);
        if (latestSettings == next) return;
        latestSettings = next;
    }
    if (const HWND target = window.load()) PostMessageW(target, runtime::kSettingsMessage, 0, 0);
}
DWORD WINAPI worker(void* parameter) {
    const auto module = static_cast<HMODULE>(parameter);
    {
        runtime::Host host(currentSettings(), currentSettings);
        if (WaitForSingleObject(stopEvent, 0) == WAIT_TIMEOUT && host.initialize(module, stopEvent)) {
            window.store(host.window());
            PostMessageW(host.window(), runtime::kSettingsMessage, 0, 0);
            host.loop(stopEvent);
            window.store(nullptr);
        } else if (WaitForSingleObject(stopEvent, 0) == WAIT_TIMEOUT) {
            Wh_Log(L"Pinch runtime could not start; native gestures remain available.");
        }
    }
    FreeLibraryAndExitThread(module, 0);
}
void requestStop() {
    if (stopEvent) SetEvent(stopEvent);
}
bool stop() {
    requestStop();
    if (thread && WaitForSingleObject(thread, 2000) != WAIT_OBJECT_0) {
        // The worker may still own windows, a hook or the shell lease. Keep
        // this version and its state alive until Explorer exits.
        HMODULE pinned = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                           reinterpret_cast<LPCWSTR>(&worker), &pinned);
        Wh_Log(L"Pinch runtime did not stop in time; retaining its state until Explorer exits.");
        return false;
    }
    if (const auto handle = std::exchange(thread, nullptr)) CloseHandle(handle);
    if (const auto handle = std::exchange(stopEvent, nullptr)) CloseHandle(handle);
    return true;
}
void start() {
    stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!stopEvent) {
        Wh_Log(L"Runtime event creation failed.");
        return;
    }
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(&worker), &module)) {
        Wh_Log(L"Pinch runtime could not retain the mod module.");
        stop();
        return;
    }
    reloadSettings();
    thread = CreateThread(nullptr, 0, worker, module, 0, nullptr);
    if (!thread) {
        FreeLibrary(module);
        Wh_Log(L"Pinch runtime could not start; native gestures remain available.");
        stop();
    }
}
}  // namespace controller

// Native popup menus. UxTheme ordinals are private and must match the offline
// profile on Windows builds for which their entry points have been verified.
namespace legacy_dialogs {
enum class Host { Other, Explorer, Control, Management, Registry };
Host currentHost = Host::Other;
std::atomic<bool> enabled{false};
std::atomic<bool> dark{false};
constexpr wchar_t kOriginalTitle[] = L"BetterVisual.RestoreLegacyTitle";
constexpr COLORREF kBackground = RGB(32, 32, 32);
constexpr COLORREF kField = RGB(44, 44, 44);
constexpr COLORREF kText = RGB(240, 240, 240);
constexpr COLORREF kDisabledText = RGB(156, 156, 156);
constexpr DWORD kImmersiveTitle = 20;
decltype(&DefDlgProcW) originalDialog = nullptr;
decltype(&DefWindowProcW) originalWindow = nullptr;
using OpenTheme = HANDLE(WINAPI*)(HWND, LPCWSTR);
using OpenThemeDpi = HANDLE(WINAPI*)(HWND, LPCWSTR, UINT);
OpenTheme originalOpenTheme = nullptr;
OpenThemeDpi originalOpenThemeDpi = nullptr;
HMODULE themeLibrary = nullptr;

bool inDirectory(const wchar_t* image, const wchar_t* fileName, const wchar_t* directory) {
    const std::size_t length = std::wcslen(directory);
    return _wcsnicmp(image, directory, length) == 0 && image[length] == L'\\' &&
           _wcsicmp(image + length + 1, fileName) == 0;
}

bool allowControlApplet(const wchar_t* commandLine, const wchar_t* system) {
    int count = 0;
    LPWSTR* args = CommandLineToArgvW(commandLine, &count);
    if (!args) return false;
    bool accepted = false;
    if (count >= 3 && (_wcsicmp(args[1], L"shell32.dll,Control_RunDLL") == 0 ||
                      inDirectory(args[1], L"shell32.dll,Control_RunDLL", system))) {
        std::wstring applet(args[2]);
        if (const auto comma = applet.find(L','); comma != std::wstring::npos) applet.erase(comma);
        constexpr const wchar_t* names[]{
            L"appwiz.cpl", L"desk.cpl", L"hdwwiz.cpl", L"inetcpl.cpl", L"intl.cpl", L"main.cpl",
            L"mmsys.cpl", L"ncpa.cpl", L"powercfg.cpl", L"sysdm.cpl", L"timedate.cpl"};
        for (const auto* name : names) {
            if (_wcsicmp(applet.c_str(), name) == 0 || inDirectory(applet.c_str(), name, system)) {
                accepted = true;
                break;
            }
        }
    }
    LocalFree(args);
    return accepted;
}

Host findHost(const wchar_t* image, const wchar_t* commandLine) {
    wchar_t windows[MAX_PATH]{};
    wchar_t system[MAX_PATH]{};
    const UINT windowsLength = GetWindowsDirectoryW(windows, ARRAYSIZE(windows));
    const UINT systemLength = GetSystemDirectoryW(system, ARRAYSIZE(system));
    if (!windowsLength || windowsLength >= ARRAYSIZE(windows) || !systemLength || systemLength >= ARRAYSIZE(system))
        return Host::Other;
    if (inDirectory(image, L"explorer.exe", windows)) return Host::Explorer;
    if (inDirectory(image, L"regedit.exe", windows)) return Host::Registry;
    if (inDirectory(image, L"mmc.exe", system)) return Host::Management;
    if (inDirectory(image, L"rundll32.exe", system))
        return allowControlApplet(commandLine, system) ? Host::Control : Host::Other;
    constexpr const wchar_t* controllers[]{
        L"control.exe", L"OptionalFeatures.exe", L"SystemPropertiesAdvanced.exe", L"SystemPropertiesComputerName.exe",
        L"SystemPropertiesHardware.exe", L"SystemPropertiesPerformance.exe", L"SystemPropertiesProtection.exe",
        L"SystemPropertiesRemote.exe"};
    for (const auto* name : controllers) if (inDirectory(image, name, system)) return Host::Control;
    return Host::Other;
}

bool supportedBuild() {
    using Version = LONG(WINAPI*)(OSVERSIONINFOW*);
    const auto version = reinterpret_cast<Version>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
    OSVERSIONINFOW info{};
    info.dwOSVersionInfoSize = sizeof(info);
    if (!version || version(&info) != 0 || info.dwBuildNumber < 26100) return false;
    if (info.dwBuildNumber != 26100 && info.dwBuildNumber != 26200) return info.dwBuildNumber >= 26300;
    DWORD revision = 0;
    DWORD size = sizeof(revision);
    return RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"UBR",
                        RRF_RT_REG_DWORD, nullptr, &revision, &size) == ERROR_SUCCESS && revision >= 6899;
}

bool sameClass(HWND window, LPCWSTR expected) {
    wchar_t name[64]{};
    return window && GetClassNameW(window, name, ARRAYSIZE(name)) > 0 && _wcsicmp(name, expected) == 0;
}

bool supportedDialog(HWND window) {
    if (!sameClass(window, L"#32770")) return false;
    const HWND root = GetAncestor(window, GA_ROOT);
    if (root != window && !sameClass(root, L"#32770")) return false;
    // Property sheets contain a standard tab control. Preserve native
    // DirectUI-based tasks, including Windows' themed file operations.
    if (FindWindowExW(root, nullptr, L"SysTabControl32", nullptr)) return true;
    return !FindWindowExW(root, nullptr, L"DirectUIHWND", nullptr) &&
           !FindWindowExW(root, nullptr, L"DUIViewWndClassName", nullptr);
}

bool supportedRoot(HWND window) {
    return supportedDialog(window) ||
        (currentHost == Host::Registry && sameClass(window, L"RegEdit_RegEdit")) ||
        (currentHost == Host::Management && sameClass(window, L"MMCMainFrame"));
}

bool supportedControl(HWND window) {
    const HWND parent = GetParent(window);
    return (parent && supportedDialog(parent)) || supportedRoot(GetAncestor(window, GA_ROOT));
}

void setTitle(HWND window, bool useDark) {
    if (GetAncestor(window, GA_ROOT) != window) return;
    const BOOL on = TRUE;
    const BOOL off = FALSE;
    if (useDark) {
        if (GetPropW(window, kOriginalTitle)) return;
        BOOL wasDark = FALSE;
        if (FAILED(DwmGetWindowAttribute(window, kImmersiveTitle, &wasDark, sizeof(wasDark))) || wasDark) return;
        if (SetPropW(window, kOriginalTitle, reinterpret_cast<HANDLE>(1))) {
            if (FAILED(DwmSetWindowAttribute(window, kImmersiveTitle, &on, sizeof(on))))
                RemovePropW(window, kOriginalTitle);
        }
    } else if (RemovePropW(window, kOriginalTitle)) {
        DwmSetWindowAttribute(window, kImmersiveTitle, &off, sizeof(off));
    }
}

BOOL CALLBACK notifyControl(HWND window, LPARAM) {
    PostMessageW(window, WM_THEMECHANGED, 0, 0);
    return TRUE;
}

BOOL CALLBACK refreshWindow(HWND window, LPARAM flags) {
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    if (pid != GetCurrentProcessId() || !supportedRoot(window)) return TRUE;
    setTitle(window, flags != 0);
    notifyControl(window, 0);
    EnumChildWindows(window, notifyControl, 0);
    RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
    return TRUE;
}

bool systemDark() {
    HIGHCONTRASTW contrast{};
    contrast.cbSize = sizeof(contrast);
    if (!SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0) ||
        (contrast.dwFlags & HCF_HIGHCONTRASTON))
        return false;
    DWORD light = 1, size = sizeof(light);
    return RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                        L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &size) == ERROR_SUCCESS &&
           size == sizeof(light) && light == 0;
}

void refreshTheme() {
    const bool useDark = enabled.load(std::memory_order_relaxed) && systemDark();
    const bool previous = dark.exchange(useDark, std::memory_order_relaxed);
    if (previous == useDark) return;
    if (themeLibrary) EnumWindows(refreshWindow, static_cast<LPARAM>(useDark));
}

void setEnabled(bool setting) {
    enabled.store(setting, std::memory_order_relaxed);
    refreshTheme();
}

LRESULT colors(UINT message, HDC dc, HWND control) {
    if (!dc) return 0;
    if (message == WM_CTLCOLORSTATIC && sameClass(control, L"Static")) {
        const auto style = GetWindowLongPtrW(control, GWL_STYLE) & SS_TYPEMASK;
        if (style == SS_ICON || style == SS_BITMAP || style == SS_ENHMETAFILE ||
            style == SS_OWNERDRAW || style >= SS_ETCHEDHORZ) return 0;
    }
    const bool field = message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX || sameClass(control, L"Edit");
    const COLORREF background = field ? kField : kBackground;
    SetTextColor(dc, control && !IsWindowEnabled(control) ? kDisabledText : kText);
    SetBkColor(dc, background);
    SetDCBrushColor(dc, background);
    return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));
}

LRESULT WINAPI dialog(HWND window, UINT message, WPARAM w, LPARAM l) {
    if (dark.load(std::memory_order_relaxed)) {
        if (message == WM_CTLCOLORDLG || message == WM_CTLCOLORSTATIC || message == WM_CTLCOLOREDIT ||
            message == WM_CTLCOLORBTN || message == WM_CTLCOLORLISTBOX) {
            if (supportedDialog(window))
                if (const auto result = colors(message, reinterpret_cast<HDC>(w), reinterpret_cast<HWND>(l))) return result;
        } else if ((message == WM_INITDIALOG || (message == WM_SHOWWINDOW && w)) && supportedDialog(window)) {
            setTitle(window, true);
        }
    }
    [[clang::musttail]] return originalDialog(window, message, w, l);
}

LRESULT WINAPI window(HWND hwnd, UINT message, WPARAM w, LPARAM l) {
    if ((message == WM_SETTINGCHANGE || message == WM_SYSCOLORCHANGE || message == WM_THEMECHANGED) &&
        GetAncestor(hwnd, GA_ROOT) == hwnd) refreshTheme();
    if (dark.load(std::memory_order_relaxed) && (message == WM_SHOWWINDOW || message == WM_NCACTIVATE) &&
        supportedRoot(hwnd)) setTitle(hwnd, true);
    [[clang::musttail]] return originalWindow(hwnd, message, w, l);
}

LPCWSTR darkClass(LPCWSTR requested) {
    if (!requested) return nullptr;
#define BETTER_VISUAL_THEME(name) if (_wcsicmp(requested, L##name) == 0) return L"DarkMode_DarkTheme::" L##name
    BETTER_VISUAL_THEME("Button");
    BETTER_VISUAL_THEME("ComboBox");
    BETTER_VISUAL_THEME("Edit");
    BETTER_VISUAL_THEME("Header");
    BETTER_VISUAL_THEME("ListView");
    BETTER_VISUAL_THEME("ListBox");
    BETTER_VISUAL_THEME("Progress");
    BETTER_VISUAL_THEME("ScrollBar");
    BETTER_VISUAL_THEME("Status");
    BETTER_VISUAL_THEME("Tab");
    BETTER_VISUAL_THEME("Toolbar");
    BETTER_VISUAL_THEME("Trackbar");
    BETTER_VISUAL_THEME("TreeView");
    BETTER_VISUAL_THEME("UpDown");
#undef BETTER_VISUAL_THEME
    return nullptr;
}

HANDLE WINAPI open(HWND hwnd, LPCWSTR classes) {
    if (dark.load(std::memory_order_relaxed)) {
        const auto name = darkClass(classes);
        if (name && supportedControl(hwnd))
            if (const auto theme = originalOpenTheme(hwnd, name)) return theme;
    }
    [[clang::musttail]] return originalOpenTheme(hwnd, classes);
}

HANDLE WINAPI openDpi(HWND hwnd, LPCWSTR classes, UINT dpi) {
    if (dark.load(std::memory_order_relaxed)) {
        const auto name = darkClass(classes);
        if (name && supportedControl(hwnd))
            if (const auto theme = originalOpenThemeDpi(hwnd, name, dpi)) return theme;
    }
    [[clang::musttail]] return originalOpenThemeDpi(hwnd, classes, dpi);
}

bool initialize(Host host) {
    if (host == Host::Other || !supportedBuild()) return false;
    const HMODULE library = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!library) return false;
    const auto theme = GetProcAddress(library, "OpenThemeData");
    const auto perDpi = GetProcAddress(library, "OpenThemeDataForDpi");
    if (!theme) { FreeLibrary(library); return false; }
    currentHost = host;
    themeLibrary = library;
    const bool dialogInstalled = Wh_SetFunctionHook(reinterpret_cast<void*>(DefDlgProcW), reinterpret_cast<void*>(dialog),
                                                   reinterpret_cast<void**>(&originalDialog));
    const bool windowInstalled = Wh_SetFunctionHook(reinterpret_cast<void*>(DefWindowProcW), reinterpret_cast<void*>(window),
                                                   reinterpret_cast<void**>(&originalWindow));
    const bool themesInstalled = Wh_SetFunctionHook(reinterpret_cast<void*>(theme), reinterpret_cast<void*>(open),
                                                   reinterpret_cast<void**>(&originalOpenTheme));
    const bool dpiInstalled = perDpi && Wh_SetFunctionHook(reinterpret_cast<void*>(perDpi), reinterpret_cast<void*>(openDpi),
                                                         reinterpret_cast<void**>(&originalOpenThemeDpi));
    if (dialogInstalled || windowInstalled || themesInstalled || dpiInstalled) return true;
    FreeLibrary(std::exchange(themeLibrary, nullptr));
    return false;
}

void stop() {
    setEnabled(false);
    if (const HMODULE library = std::exchange(themeLibrary, nullptr)) FreeLibrary(library);
}
} // namespace legacy_dialogs

bool runZoomController = false;
bool dormantThemeHost = false;

bool darkThemeSelected() {
    return Wh_GetIntSetting(L"darkLegacyWindows") != 0;
}
void updateLegacyTheme() {
    legacy_dialogs::setEnabled(darkThemeSelected());
}
void stopLegacyTheme() {
    legacy_dialogs::stop();
}

BOOL Wh_ModInit() {
    DWORD session = 0;
    if (!ProcessIdToSessionId(GetCurrentProcessId(), &session) || !session) return FALSE;
    wchar_t path[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, path, ARRAYSIZE(path));
    if (!length || length >= ARRAYSIZE(path)) return FALSE;
    const wchar_t* const separator = wcsrchr(path, L'\\');
    const wchar_t* const leaf = separator ? separator + 1 : path;
    if (_wcsicmp(leaf, L"magnify.exe") == 0) {
        if (magnifier::initialize()) return TRUE;
        return magnifier::symbolUnsupported && !magnifier::hooksAttempted;
    }
    const auto host = legacy_dialogs::findHost(path, GetCommandLineW());
    if (host == legacy_dialogs::Host::Other) return FALSE;
    if (host != legacy_dialogs::Host::Explorer && !darkThemeSelected()) {
        dormantThemeHost = true;
        return TRUE;
    }
    const bool themeAvailable = legacy_dialogs::initialize(host);
    Wh_Log(L"Legacy dialog styling available=%d.", themeAvailable);
    if (host != legacy_dialogs::Host::Explorer) return themeAvailable;

    // Secondary Explorer processes can theme their own windows while the
    // desktop shell alone owns the zoom controller and gesture hooks.
    DWORD shellOwner = 0;
    if (const HWND shellWindow = GetShellWindow()) GetWindowThreadProcessId(shellWindow, &shellOwner);
    if (shellOwner && shellOwner != GetCurrentProcessId()) return themeAvailable;
    if (!shellOwner) {
        int count = 0;
        const auto args = CommandLineToArgvW(GetCommandLineW(), &count);
        if (!args) return themeAvailable;
        const bool startupShell = count == 1 || (count == 2 && _wcsicmp(args[1], L"/nouaccheck") == 0);
        LocalFree(args);
        if (!startupShell) return themeAvailable;
    }
    if (native_gate::initialize() || (native_gate::symbolUnsupported && !native_gate::hooksAttempted)) {
        runZoomController = true;
        return TRUE;
    }
    native_gate::stop();
    if (!native_gate::hooksAttempted) return themeAvailable;
    stopLegacyTheme();
    return FALSE;
}
void Wh_ModAfterInit() {
    if (magnifier::role) {
        magnifier::refreshBorderSettings();
        magnifier::afterInit();
        return;
    }
    updateLegacyTheme();
    if (runZoomController) {
        native_gate::ready();
        controller::start();
    }
}
BOOL Wh_ModSettingsChanged(BOOL* reload) {
    *reload = FALSE;
    if (magnifier::role) {
        if (!magnifier::available || magnifier::stopping.load()) return TRUE;
        if (const HWND target = magnifier::messageWindow.load()) PostMessageW(target, magnifier::kCancelKeyboardHold, 0, 0);
        magnifier::refreshNativeVisibility();
        magnifier::keepPointerStable.store(Wh_GetIntSetting(L"keepPointerStable") != 0);
        magnifier::refreshBorderSettings();
        return TRUE;
    }
    if (dormantThemeHost) {
        *reload = darkThemeSelected();
        return TRUE;
    }
    updateLegacyTheme();
    if (runZoomController) controller::reloadSettings();
    return TRUE;
}
void Wh_ModBeforeUninit() {
    if (runZoomController) controller::requestStop();
}
void Wh_ModUninit() {
    if (magnifier::role) { magnifier::stop(); return; }
    stopLegacyTheme();
    if (runZoomController && controller::stop()) native_gate::stop();
}

#endif  // PINCH_ZOOM_GATE_TEST
#endif  // PINCH_ZOOM_NATIVE_TEST
#endif  // Native adapter or Windhawk
#endif  // PINCH_ZOOM_CORE_TEST
