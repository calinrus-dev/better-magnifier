// ==WindhawkMod==
// @id              kde-pinch-zoom
// @name            KWin-inspired Accessible Pinch Zoom
// @name:es-ES      Zoom accesible inspirado en KWin
// @description     KWin-inspired accessibility: three-finger pinch for native Windows Magnifier
// @description:es-ES Accesibilidad inspirada en KWin: pellizco de tres dedos para la Lupa nativa de Windows
// @version         0.3.1
// @author          calinrus-dev
// @github          https://github.com/calinrus-dev
// @homepage        https://github.com/calinrus-dev/windhawk-kde-pinch-zoom
// @include         explorer.exe
// @include         magnify.exe
// @architecture    amd64
// @compilerOptions -DNOMINMAX -static -lhid -lwtsapi32 -lshell32 -ladvapi32
// @license         GPL-3.0-or-later
// ==/WindhawkMod==

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 calinrus-dev
// Raw HID handling was informed by Three Finger Drag by
// Rodrigo Thomazi (GPL-3.0). See THIRD_PARTY.md.

// ==WindhawkModReadme==
/*
# KDE Pinch Zoom — experimental

Spread three fingers to magnify the desktop; pinch them together to zoom out.
Two-finger app zoom and Windows three/four-finger swipe settings are untouched.
A Precision Touchpad and Windows build 26200 are required. A companion in
Explorer holds native app-switch/task-view swipes until the gesture is classified.
Pinches suppress those swipes; genuine swipes use the original Windows handler.
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
https://github.com/calinrus-dev/windhawk-kde-pinch-zoom
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
  $name: Share zoom with Windows Magnifier shortcuts
- keyboardStep: 0
  $name: Keyboard zoom increment (%)
  $description: 0 keeps the increment configured in Windows Magnifier.
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
    int keyboardStep = 0;
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

public:
    void reset() { phase_ = Phase::Idle; }
    Phase phase() const { return phase_; }
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
        const double travel = std::hypot(center.x - start_.x, center.y - start_.y);
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
        if (mode_ == GateMode::Pending) {
            if (phase == Phase::Pinch) mode_ = GateMode::Suppress;
            else if (phase == Phase::Swipe) mode_ = GateMode::Forward;
        }
        return mode_;
    }
    GateMode finish(Phase phase) {
        update(phase);
        if (mode_ == GateMode::Pending) mode_ = GateMode::Forward;
        return mode_;
    }
    GateMode mode() const { return mode_; }
};
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

}  // namespace pinch

#ifndef PINCH_ZOOM_CORE_TEST

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <shellapi.h>
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

namespace bridge {
constexpr wchar_t kMapping[] = L"Local\\KdePinchZoom.NativeGate.v7";
constexpr wchar_t kWake[] = L"Local\\KdePinchZoom.NativeWake.v7";
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
    volatile LONG maxZoomPercent;
    volatile LONG syncRequest;
    volatile LONG nativeCommands;
    volatile LONG pendingCommand;
    volatile LONG nativeError;
    volatile LONG nativeLevel; // Native target in 1/10000 units, not an animation frame.
    volatile LONG requestedLevel;
    volatile LONG requestedSequence;
    volatile LONG appliedSequence;
    volatile LONG startAtNormal;
    volatile LONG zoomRevision;
    volatile LONG requestRevision;
};
class Mapping {
    HANDLE mapping_ = nullptr;
    HANDLE wake_ = nullptr;
#ifdef PINCH_ZOOM_NATIVE_TEST
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
#ifdef PINCH_ZOOM_NATIVE_TEST
    void useTestRecord(Record* record) { record_ = record; testRecord_ = true; }
#endif
    bool magnifierAvailable() const {
        if (!record_ || !InterlockedCompareExchange(&record_->magnifierReady, 0, 0)) return false;
        const auto pid = static_cast<DWORD>(InterlockedCompareExchange(&record_->magnifierPid, 0, 0));
        if (pid != magnifierPid_) {
            if (magnifierProcess_) CloseHandle(magnifierProcess_);
            magnifierProcess_ = OpenProcess(SYNCHRONIZE, FALSE, pid);
            magnifierPid_ = pid;
        }
        return magnifierProcess_ && WaitForSingleObject(magnifierProcess_, 0) == WAIT_TIMEOUT;
    }
    void configure(const pinch::Config& config) {
        if (!record_) return;
        InterlockedExchange(&record_->controlEnabled, config.enabled && !config.observeOnly && config.keyboardInterop ? 1 : 0);
        InterlockedExchange(&record_->hideUi, config.hideMagnifier ? 1 : 0);
        InterlockedExchange(&record_->keyboardStep, config.keyboardStep);
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
    bool requestLevel(double level, LONG expectedRevision = -1) {
        if (!magnifierAvailable()) return false;
        const LONG revision = InterlockedCompareExchange(&record_->zoomRevision, 0, 0);
        if (expectedRevision >= 0 && revision != expectedRevision) return true; // Rebase on the next input frame.
        InterlockedExchange(&record_->requestRevision, revision);
        InterlockedExchange(&record_->requestedLevel, static_cast<LONG>(std::lround(std::clamp(level, 1.0, 16.0) * 10000)));
        InterlockedIncrement(&record_->requestedSequence);
        wake();
        return true;
    }
    void syncNative() { if (record_) { InterlockedIncrement(&record_->syncRequest); wake(); } }
    bool threeContacts() const { return record_ && InterlockedCompareExchange(&record_->down, 0, 0) != 0; }
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
#ifdef PINCH_ZOOM_NATIVE_TEST
        if (!testRecord_ && record_) UnmapViewOfFile(record_);
#else
        if (record_) UnmapViewOfFile(record_);
#endif
        if (mapping_) CloseHandle(mapping_);
        if (wake_) CloseHandle(wake_);
    }
};
}  // namespace bridge

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
    void useTestBackend(GetTransform readFn, SetTransform writeFn, decltype(&GetCursorPos) cursorFn) {
        get_ = readFn;
        set_ = writeFn;
        getCursor_ = cursorFn;
        initialized_ = true;
        nativeRouting_ = false;
    }
#endif
    bool initialize() {
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
    double current() const { return nativeActive() ? native_.nativeLevel() : read().value_or(pinch::View{}).zoom; }
    double rendered() const { return read().value_or(pinch::View{}).zoom; }
    unsigned revision() const { return revision_; }
    bool resetToNormal() {
        if (nativeActive()) {
            InterlockedExchange(&native_.get()->pendingCommand, 3);
            native_.syncNative();
            return true;
        }
        if (!initialized_ || !set_(1.0f, 0, 0)) return false;
        last_ = {};
        original_ = {};
        target_ = 1;
        owned_ = false;
        pinching_ = false;
        ++revision_;
        return true;
    }
    bool target(double value, bool began = false) {
        if (nativeActive()) { (void)began; return native_.requestLevel(value, keyboardRevision_); }
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

class Host {
    pinch::Config config_;
    std::map<HANDLE, Device> devices_;
    Zoom zoom_;
    HWND window_ = nullptr;
    HANDLE activeDevice_ = nullptr;
    bool paused_ = false;
    bool sessionActive_ = true;
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
    unsigned nativeAttempts_ = 0;
    bool sawNative_ = false;
    bool nativeLaunchRequested_ = true;
    std::uint64_t lastNativeAttempt_ = 0;

    bool contactsDown() const {
        return std::any_of(devices_.begin(), devices_.end(), [](const auto& entry) { return entry.second.activeContacts != 0; });
    }
    void restartGestureTracking() {
        waitForRelease_ = contactsDown();
        resetGestures();
    }
    void render(std::uint64_t now) {
        if (!zoom_.tick(static_cast<double>(now - lastTick_), config_.smoothingMs,
                        config_.mouseFollow, config_.edgeMargin)) restartGestureTracking();
        if (observedRevision_ != zoom_.revision()) {
            observedRevision_ = zoom_.revision();
            const double level = zoom_.current();
            for (auto& [id, dev] : devices_) { (void)id; dev.recognizer.adoptZoom(level); }
        }
        lastTick_ = now;
    }
    void ensureNativeMagnifier(std::uint64_t now) {
        if (!config_.enabled || config_.observeOnly || !config_.keyboardInterop || !sessionActive_) return;
        if (bridge_.magnifierAvailable()) { sawNative_ = true; nativeAttempts_ = 0; return; }
        if (sawNative_) {
            sawNative_ = false;
            nativeLaunchRequested_ = false;
            zoom_.resetToNormal();
            restartGestureTracking();
        }
        if (!nativeLaunchRequested_) return;
        if (nativeProcess_ && WaitForSingleObject(nativeProcess_, 0) == WAIT_TIMEOUT) return;
        if (nativeAttempts_ >= 3 || now - lastNativeAttempt_ < 1500) return;
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
        lastNativeAttempt_ = now;
        ++nativeAttempts_;
        if (bridge_.get()) InterlockedExchange(&bridge_.get()->startAtNormal, 1);
        if (ShellExecuteExW(&launch)) nativeProcess_ = launch.hProcess;
    }

    void resetGestures() {
        bridge_.publish(pinch::Phase::Idle, false, false);
        activeDevice_ = nullptr;
        for (auto& [id, dev] : devices_) {
            (void)id;
            dev.recognizer.reset();
            dev.assembler.reset();
        }
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
        if (paused_ || !sessionActive_ || !config_.enabled) {
            bridge_.publish(pinch::Phase::Idle, false, false);
            return;
        }
        if (waitForRelease_) {
            if (dev.activeContacts < 3) {
                waitForRelease_ = false;
            }
            return;
        }
        render(now); // Adopt keyboard targets before computing this finger delta.
        const auto before = dev.recognizer.phase();
        if (dev.waitingForNative) {
            dev.recognizer.adoptZoom(zoom_.current());
            dev.waitingForNative = !bridge_.magnifierAvailable();
        }
        auto effective = config_;
        if (config_.keyboardInterop) effective.maxZoom = std::min(effective.maxZoom, 16.0);
        const auto decision = dev.recognizer.update(contacts, now, zoom_.current(), effective, elapsedMs);
        const auto phase = decision.phase == pinch::Phase::Rejected ||
                           (decision.phase == pinch::Phase::Idle && dev.activeContacts == 3)
                               ? pinch::Phase::Swipe : decision.phase;
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
            ++statistics_.pinches;
            nativeLaunchRequested_ = true;
            if (!config_.observeOnly) ensureNativeMagnifier(now);
            if (config_.debug) {
                log(L"Three-finger pinch recognized.");
            }
        }
        if (decision.zoom && (!activeDevice_ || activeDevice_ == handle)) {
            activeDevice_ = handle;
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
                host->bridge_.heartbeat();
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
                host->zoom_.resetToNormal();
                host->bridge_.syncNative();
                host->restartGestureTracking();
                host->bridge_.publish(pinch::Phase::Idle, false, false);
                if (w == kPauseHotkey) {
                    host->paused_ = !host->paused_;
                    log(host->paused_ ? L"Paused." : L"Resumed.");
                } else {
                    log(L"Zoom reset.");
                }
                return 0;
            case WM_WTSSESSION_CHANGE:
                if (w == WTS_SESSION_LOCK || w == WTS_CONSOLE_DISCONNECT || w == WTS_REMOTE_DISCONNECT ||
                    w == WTS_SESSION_LOGOFF) {
                    host->sessionActive_ = false;
                    host->zoom_.restore();
                    host->resetGestures();
                    host->waitForRelease_ = true;
                } else if (w == WTS_SESSION_UNLOCK || w == WTS_CONSOLE_CONNECT || w == WTS_REMOTE_CONNECT) {
                    host->sessionActive_ = true;
                }
                return 0;
            case WM_POWERBROADCAST:
                if (w == PBT_APMSUSPEND) {
                    host->sessionActive_ = false;
                    host->zoom_.restore();
                    host->resetGestures();
                    host->waitForRelease_ = true;
                } else if (w == PBT_APMRESUMEAUTOMATIC || w == PBT_APMRESUMESUSPEND) {
                    host->sessionActive_ = true;
                }
                return TRUE;
            case kSettingsMessage:
                host->zoom_.end();
                host->restartGestureTracking();
                host->config_ = *reinterpret_cast<const pinch::Config*>(l);
                host->bridge_.configure(host->config_);
                host->nativeAttempts_ = 0;
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
        window_ = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, kClassName, L"KDE Pinch Zoom",
                                  WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, module, this);
        if (!window_) {
            return false;
        }
        RAWINPUTDEVICE registration{kDigitizer, kTouchpad,
                                     RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, window_};
        UINT registrations = 0;
        if (GetRegisteredRawInputDevices(nullptr, &registrations, sizeof(RAWINPUTDEVICE)) == UINT(-1)) return false;
        if (registrations) {
            std::vector<RAWINPUTDEVICE> existing(registrations);
            if (GetRegisteredRawInputDevices(existing.data(), &registrations, sizeof(RAWINPUTDEVICE)) == UINT(-1)) return false;
            for (const auto& item : existing) {
                if (item.usUsagePage == kDigitizer && item.usUsage == kTouchpad) {
                    log(L"Another component already owns this process's raw touchpad registration; startup cancelled.");
                    return false;
                }
            }
        }
        if (!RegisterRawInputDevices(&registration, 1, sizeof(registration))) {
            log(L"Raw Precision Touchpad input registration failed.");
            return false;
        }
        inputRegistered_ = true;
        // Keep an emergency reset available before allowing any zoom changes.
        if (!RegisterHotKey(window_, kResetHotkey, MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, VK_F10)) {
            log(L"Emergency reset hotkey unavailable; startup cancelled.");
            return false;
        }
        if (!RegisterHotKey(window_, kPauseHotkey, MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, VK_F9)) {
            log(L"Pause hotkey unavailable; reset hotkey remains available.");
        }
        wtsRegistered_ = WTSRegisterSessionNotification(window_, NOTIFY_FOR_THIS_SESSION) != FALSE;
        if (!bridge_.open()) return false;
        bridge_.configure(config_);
        bridge_.publish(pinch::Phase::Pending, false, config_.enabled && !config_.observeOnly);
        lastTick_ = GetTickCount64();
        lastDiagnostic_ = lastTick_;
        if (!SetTimer(window_, kTimer, 16, nullptr)) {
            return false;
        }
        UINT count = 0;
        if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) == 0 && count) {
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
        return mapping.gateAvailable() ? 0 : 3;
    }
    if (reset) {
        runtime::Zoom zoom;
        if (!zoom.initialize() || !zoom.resetToNormal()) return 1;
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
        if (!zoom.initialize() || !zoom.target(1.1) || !zoom.tick(16, 0) || std::abs(zoom.current() - 1.1) > 0.002) {
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
using SetOption = void(*)(void*, int, long, bool);
using GetOption = long(*)(void*, int);
using GetKeeper = HRESULT(*)(void**);
Change originalChange = nullptr;
Run originalRun = nullptr;
SetLevel setLevel = nullptr;
ZoomCommand originalZoomCommand = nullptr;
std::atomic<void*> nativeUi{nullptr};
std::atomic<bool> trackingInitialized{false};
std::atomic<DWORD> rendererThread{0};
SetOption originalOption = nullptr;
GetOption getOption = nullptr;
GetKeeper getKeeper = nullptr;
decltype(&ShowWindow) originalShow = nullptr;
decltype(&SetWindowPos) originalPosition = nullptr;
decltype(&CreateWindowExW) originalCreate = nullptr;
decltype(&SetWindowLongPtrW) originalStyle = nullptr;
bridge::Mapping mapping;
bool role = false;
std::atomic<bool> hiding{true};
std::mutex stylesMutex;
std::map<HWND, LONG_PTR> styles;
HANDLE stopEvent = nullptr;
HANDLE thread = nullptr;
std::atomic<HWND> messageWindow{nullptr};
std::atomic<bool> stopping{false};
std::atomic<unsigned> activeUpdates{0};
constexpr wchar_t kNativeClass[] = L"KdePinchZoomNativeTarget.v7";
std::atomic<DWORD> uiThread{0};
std::atomic<void*> nativeSelf{nullptr}; // Dereferenced only on the native rendering thread.
thread_local bool applyingPinch = false;
const UINT updateMessage = RegisterWindowMessageW(L"KdePinchZoom.NativeTarget.v7");
LRESULT CALLBACK messages(HWND window, UINT message, WPARAM w, LPARAM l);
void publish(float level) {
    if (mapping.get()) InterlockedExchange(&mapping.get()->nativeLevel, static_cast<LONG>(std::lround(level * 10000)));
}
void run(void* self) {
    nativeUi.store(self);
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
            if (nativeSelf) InterlockedExchange(&mapping.get()->magnifierReady, 1);
        }
    }
    // Run owns the message loop. Tail-call it so no mod return address remains
    // on the UI stack when Windhawk later unloads this DLL.
    [[clang::musttail]] return originalRun(self);
}
void change(void* self, float level, POINT* point) {
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
    originalChange(self, level, point); // Preserve all native animations and focus tracking.
    publish(level);
    if (messageWindow) InterlockedExchange(&mapping.get()->magnifierReady, 1);
}
long readOption(void* self, int name) {
    if (name == 20 && mapping.get() && mapping.get()->keyboardStep > 0)
        return mapping.get()->keyboardStep;
    return getOption(self, name);
}
void nativeCommandComplete(void*) {
    if (applyingPinch) return;
    void* keeper = nullptr;
    if (FAILED(getKeeper(&keeper)) || !keeper) return;
    publish(getOption(keeper, 14) / 100.0f);
    InterlockedExchange(&mapping.get()->appliedSequence, mapping.get()->requestedSequence);
    InterlockedIncrement(&mapping.get()->nativeCommands);
    InterlockedIncrement(&mapping.get()->zoomRevision);
    trackingInitialized.store(mapping.get()->nativeLevel > 10000);
}
void uiLevel(void* self, long level) {
    setLevel(self, level);
    nativeCommandComplete(self);
}
void zoomCommand(void* self, int command) {
    originalZoomCommand(self, command);
    nativeCommandComplete(self);
}
void option(void* self, int name, long value, bool save) {
    if (applyingPinch) save = false; // No registry writes for every HID frame.
    const long previousMode = name == 4 ? getOption(self, 4) : 0;
    originalOption(self, name, value, save);
    if (name == 4 && mapping.get() && previousMode != getOption(self, 4)) {
        // Mode changes can destroy the fullscreen renderer. Do not keep using it.
        InterlockedExchange(&mapping.get()->magnifierReady, 0);
        nativeSelf = nullptr;
    }
}
LRESULT CALLBACK messages(HWND window, UINT message, WPARAM w, LPARAM l) {
    if (message == WM_CLOSE) {
        const bool reentrant = activeUpdates.load() != 0;
        DestroyWindow(window);
        return reentrant ? 1 : 0;
    }
    if (message == WM_NCDESTROY) { messageWindow.store(nullptr); return DefWindowProcW(window, message, w, l); }
    if (message == updateMessage) {
        if (stopping.load()) return 0;
        struct ActiveUpdate {
            ActiveUpdate() { activeUpdates.fetch_add(1); }
            ~ActiveUpdate() { activeUpdates.fetch_sub(1); }
        } active;
        auto* state = mapping.get();
        const LONG sequence = InterlockedCompareExchange(&state->requestedSequence, 0, 0);
        const LONG action = InterlockedExchange(&state->pendingCommand, 0);
        void* keeper = nullptr;
        if ((action || sequence != state->appliedSequence) && (state->controlEnabled || action == 3) && state->magnifierReady && nativeUi && SUCCEEDED(getKeeper(&keeper)) && keeper) {
            double level = InterlockedCompareExchange(&state->requestedLevel, 0, 0) / 10000.0;
            if (action) {
                const double current = getOption(keeper, 14) / 100.0;
                const double step = getOption(keeper, 20) / 100.0;
                level = action == 3 ? 1 : current + (action == 1 ? step : -step);
            }
            const double cap = std::clamp(state->maxZoomPercent / 100.0, 1.0, 16.0);
            level = std::clamp(level, 1.0, cap);
            if (!action && state->requestRevision != state->zoomRevision) {
                InterlockedExchange(&state->appliedSequence, sequence);
                return 0; // A keyboard change or priming step invalidated this finger baseline.
            }
            if (rendererThread.load() != GetCurrentThreadId()) {
                InterlockedExchange(&state->nativeError, ERROR_INVALID_THREAD_ID);
                return 0; // Preserve native controls instead of calling an object on the wrong thread.
            }
            const double previous = state->nativeLevel / 10000.0;
            const bool prime = level > 1 && (!trackingInitialized.load() || previous <= 1);
            const bool reset = level <= 1 && (previous > 1 || action == 3);
            double delivered = level;
            if (prime || reset || std::abs(level - previous) >= 0.00005) {
                applyingPinch = true;
                long percent = static_cast<long>(std::lround(level * 100));
                if (prime) percent = std::max(101L, percent);
                originalOption(keeper, 14, percent, false);
                if (prime || reset) {
                    // Exactly one UI update enters/leaves magnification and its
                    // native tracker. All intermediate frames retain float precision.
                    setLevel(nativeUi, percent);
                    delivered = percent / 100.0;
                    trackingInitialized.store(prime);
                } else {
                    originalChange(nativeSelf, static_cast<float>(level), nullptr);
                }
                applyingPinch = false;
            }
            publish(static_cast<float>(delivered));
            if (prime) InterlockedIncrement(&state->zoomRevision);
            InterlockedExchange(&state->nativeError, 0);
            InterlockedExchange(&state->appliedSequence, sequence);
            if (action) { InterlockedIncrement(&state->nativeCommands); InterlockedIncrement(&state->zoomRevision); }
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
        const bool hideNow = InterlockedCompareExchange(&state->hideUi, 0, 0) != 0;
        if (hiding.exchange(hideNow) != hideNow) EnumWindows(visibility, 0);
        const LONG sequence = InterlockedCompareExchange(&state->requestedSequence, 0, 0);
        const LONG command = InterlockedCompareExchange(&state->syncRequest, 0, 0);
        const HWND target = messageWindow.load();
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
bool initialize() {
    if (!mapping.open()) return false;
    hiding.store(Wh_GetIntSetting(L"hideMagnifier") != 0);
    void* targets[7]{};
    WH_FIND_SYMBOL symbol{};
    const HANDLE search = Wh_FindFirstSymbol(GetModuleHandleW(nullptr), nullptr, &symbol);
    if (!search) return false;
    do {
        if (wcsstr(symbol.symbol, L"FullscreenMagnifier::ChangeMagnificationFactor(float,class CPoint")) targets[0] = symbol.address;
        if (wcsstr(symbol.symbol, L"OptionsKeeper::SetOption(") && wcsstr(symbol.symbol, L"long,bool")) targets[1] = symbol.address;
        if (wcsstr(symbol.symbol, L"OptionsKeeper::GetOption(")) targets[2] = symbol.address;
        if (wcsstr(symbol.symbol, L"OptionsKeeper::GetOptionsKeeper(")) targets[3] = symbol.address;
        if (wcsstr(symbol.symbol, L"MagnifierUI::Run(")) targets[4] = symbol.address;
        if (wcsstr(symbol.symbol, L"MagnifierUI::HandleSetZoomLevel(")) targets[5] = symbol.address;
        if (wcsstr(symbol.symbol, L"MagnifierUI::HandleZoomCommand(")) targets[6] = symbol.address;
        if (std::all_of(std::begin(targets), std::end(targets), [](void* target) { return target != nullptr; })) break;
    } while (Wh_FindNextSymbol(search, &symbol));
    Wh_FindCloseSymbol(search);
    for (auto target : targets) if (!target) { Wh_Log(L"Native Magnifier symbols unavailable."); return false; }
    getOption = reinterpret_cast<GetOption>(targets[2]);
    getKeeper = reinterpret_cast<GetKeeper>(targets[3]);
    role = true;
    return Wh_SetFunctionHook(targets[4], reinterpret_cast<void*>(run), reinterpret_cast<void**>(&originalRun)) &&
        Wh_SetFunctionHook(targets[5], reinterpret_cast<void*>(uiLevel), reinterpret_cast<void**>(&setLevel)) &&
        Wh_SetFunctionHook(targets[6], reinterpret_cast<void*>(zoomCommand), reinterpret_cast<void**>(&originalZoomCommand)) &&
        Wh_SetFunctionHook(targets[0], reinterpret_cast<void*>(change), reinterpret_cast<void**>(&originalChange)) &&
        Wh_SetFunctionHook(targets[1], reinterpret_cast<void*>(option), reinterpret_cast<void**>(&originalOption)) &&
        Wh_SetFunctionHook(targets[2], reinterpret_cast<void*>(readOption), reinterpret_cast<void**>(&getOption)) &&
        Wh_SetFunctionHook(reinterpret_cast<void*>(ShowWindow), reinterpret_cast<void*>(show), reinterpret_cast<void**>(&originalShow)) &&
        Wh_SetFunctionHook(reinterpret_cast<void*>(SetWindowPos), reinterpret_cast<void*>(position), reinterpret_cast<void**>(&originalPosition)) &&
        Wh_SetFunctionHook(reinterpret_cast<void*>(CreateWindowExW), reinterpret_cast<void*>(create), reinterpret_cast<void**>(&originalCreate)) &&
        Wh_SetFunctionHook(reinterpret_cast<void*>(SetWindowLongPtrW), reinterpret_cast<void*>(style), reinterpret_cast<void**>(&originalStyle));
}
#endif
void afterInit() {
    InterlockedExchange(&mapping.get()->magnifierPid, static_cast<LONG>(GetCurrentProcessId()));
    InterlockedExchange(&mapping.get()->magnifierReady, 0);
    InterlockedExchange(&mapping.get()->appliedSequence, mapping.get()->requestedSequence);
    InterlockedExchange(&mapping.get()->pendingCommand, 0);
    EnumWindows(visibility, 0);
    stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (stopEvent) thread = CreateThread(nullptr, 0, worker, nullptr, 0, nullptr);
}
void stop() {
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

#ifndef PINCH_ZOOM_NATIVE_TEST
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
std::mutex mutex;
struct State {
    pinch::GestureGate gate{pinch::Phase::Pending};
    int x;
    int y;
    unsigned device;
    Context context;
    bool started = false;
    bool sawContacts = false;
};
std::map<void*, State> states;

HRESULT start(void* self, int x, int y, unsigned device, Context context) {
    // The third argument is the native pointer id (the original handler uses
    // GetPointerInfo on it). Four-finger gestures must bypass the gate entirely.
    UINT32 contacts = 0;
    GetPointerFrameInfo(device, &contacts, nullptr);
    const auto phase = contacts == 3 ? mapping.phase() : pinch::Phase::Swipe;
    State state{pinch::GestureGate(phase), x, y, device, context};
    state.sawContacts = mapping.threeContacts();
    if (auto* record = mapping.get()) InterlockedExchange(&record->reserved,
        static_cast<LONG>(contacts | (static_cast<unsigned>(state.gate.mode()) << 8)));
    state.started = state.gate.mode() == pinch::GateMode::Forward;
    {
        std::lock_guard lock(mutex);
        states.insert_or_assign(self, state);
    }
    return state.started ? originalStart(self, x, y, device, context) : S_OK;
}

HRESULT continueSwipe(void* self, int x, int y, int dx, int dy, unsigned device) {
    State state{};
    bool deferredStart = false;
    bool tracked = false;
    {
        std::lock_guard lock(mutex);
        if (const auto found = states.find(self); found != states.end()) {
            tracked = true;
            auto& current = found->second;
            current.sawContacts = current.sawContacts || mapping.threeContacts();
            if (current.gate.update(mapping.phase()) == pinch::GateMode::Forward) {
                deferredStart = !current.started;
                current.started = true;
            }
            state = current;
            if (auto* record = mapping.get()) {
                const LONG contactBits = InterlockedCompareExchange(&record->reserved, 0, 0) & 0xFF;
                InterlockedExchange(&record->reserved, contactBits | (static_cast<LONG>(current.gate.mode()) << 8));
            }
        }
    }
    if (!tracked) return originalContinue(self, x, y, dx, dy, device);
    if (!state.started) return S_OK;
    if (deferredStart) {
        const HRESULT result = originalStart(self, state.x, state.y, state.device, state.context);
        if (FAILED(result)) return result;
    }
    return originalContinue(self, x, y, dx, dy, device);
}

HRESULT finish(void* self, int x, int y, int dx, int dy, unsigned device) {
    std::optional<State> state;
    {
        std::lock_guard lock(mutex);
        if (const auto found = states.find(self); found != states.end()) {
            state = found->second;
            states.erase(found);
        }
    }
    if (!state) return originalFinish(self, x, y, dx, dy, device);
    const auto finalPhase = state->sawContacts || mapping.threeContacts() ? mapping.phase(true) : pinch::Phase::Swipe;
    if (state->gate.finish(finalPhase) == pinch::GateMode::Suppress) return S_OK;
    if (!state->started) {
        const HRESULT result = originalStart(self, state->x, state->y, state->device, state->context);
        if (FAILED(result)) return result;
    }
    return originalFinish(self, x, y, dx, dy, device);
}

HRESULT cancel(void* self) {
    std::optional<State> state;
    {
        std::lock_guard lock(mutex);
        if (const auto found = states.find(self); found != states.end()) {
            state = found->second;
            states.erase(found);
        }
    }
    return !state || state->started ? originalCancel(self) : S_OK;
}

bool initialize() {
    using GetVersion = LONG(WINAPI*)(OSVERSIONINFOW*);
    const auto getVersion = reinterpret_cast<GetVersion>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
    OSVERSIONINFOW version{};
    version.dwOSVersionInfoSize = sizeof(version);
    if (!getVersion || getVersion(&version) != 0 || version.dwBuildNumber != 26200) {
        Wh_Log(L"Native gate currently supports Windows build 26200 only.");
        return false;
    }
    const HMODULE module = GetModuleHandleW(L"twinui.pcshell.dll");
    if (!module || !mapping.open()) return false;
    const wchar_t* names[] = {
        L"MultitaskingViewGestureHandler::StartSwipe(",
        L"MultitaskingViewGestureHandler::ContinueSwipe(",
        L"MultitaskingViewGestureHandler::FinishSwipe(",
        L"MultitaskingViewGestureHandler::CancelSwipe("};
    void* targets[4]{};
    WH_FIND_SYMBOL symbol{};
    const HANDLE search = Wh_FindFirstSymbol(module, nullptr, &symbol);
    if (!search) return false;
    do {
        for (int i = 0; i < 4; ++i) {
            if (std::wcsstr(symbol.symbol, names[i])) targets[i] = symbol.address;
        }
    } while (Wh_FindNextSymbol(search, &symbol));
    Wh_FindCloseSymbol(search);
    for (auto target : targets) {
        if (!target) {
            Wh_Log(L"Native swipe symbols unavailable; refusing to enable pinch zoom.");
            return false;
        }
    }
    shell = true;
    return Wh_SetFunctionHook(targets[0], reinterpret_cast<void*>(start), reinterpret_cast<void**>(&originalStart)) &&
           Wh_SetFunctionHook(targets[1], reinterpret_cast<void*>(continueSwipe), reinterpret_cast<void**>(&originalContinue)) &&
           Wh_SetFunctionHook(targets[2], reinterpret_cast<void*>(finish), reinterpret_cast<void**>(&originalFinish)) &&
           Wh_SetFunctionHook(targets[3], reinterpret_cast<void*>(cancel), reinterpret_cast<void**>(&originalCancel));
}
void ready() {
    if (auto* record = mapping.get()) {
        InterlockedExchange(&record->shellPid, static_cast<LONG>(GetCurrentProcessId()));
        InterlockedExchange(&record->gateReady, 1);
        Wh_Log(L"Native three-finger swipe gate ready.");
    }
}
void stop() {
    if (auto* record = mapping.get()) InterlockedExchange(&record->gateReady, 0);
}
}  // namespace native_gate

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
    config.keyboardStep = std::clamp(Wh_GetIntSetting(L"keyboardStep"), 0, 1000);
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
        if (!getVersion || getVersion(&version) != 0 || version.dwBuildNumber != 26200) return FALSE;
        return magnifier::initialize();
    }
    return native_gate::initialize();
}
void Wh_ModAfterInit() {
    if (magnifier::role) { magnifier::afterInit(); return; }
    native_gate::ready();
    controller::start();
}
BOOL Wh_ModSettingsChanged(BOOL* reload) {
    *reload = FALSE;
    if (magnifier::role) {
        magnifier::hiding.store(Wh_GetIntSetting(L"hideMagnifier") != 0);
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

#endif  // PINCH_ZOOM_NATIVE_TEST
#endif  // Native adapter or Windhawk
#endif  // PINCH_ZOOM_CORE_TEST
