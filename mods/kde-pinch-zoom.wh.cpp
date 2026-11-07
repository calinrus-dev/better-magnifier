// ==WindhawkMod==
// @id              kde-pinch-zoom
// @name            KDE Pinch Zoom
// @name:es-ES      Zoom con pellizco de KDE
// @description     Continuous three-finger desktop pinch zoom, leaving Windows swipe settings intact
// @description:es-ES Zoom continuo del escritorio con un pellizco de tres dedos, conservando la configuración de deslizamientos de Windows
// @version         0.1.0
// @author          calinrus-dev
// @github          https://github.com/calinrus-dev
// @homepage        https://github.com/calinrus-dev/windhawk-kde-pinch-zoom
// @include         windhawk.exe
// @architecture    amd64
// @compilerOptions -DNOMINMAX -static -lhid -lwtsapi32 -lshell32
// @license         GPL-3.0-or-later
// ==/WindhawkMod==

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 calinrus-dev
// The tool-mod bootstrap follows the Windhawk tool-mod template by Michael
// Maltsev. Raw HID handling was informed by Three Finger Drag by
// Rodrigo Thomazi (GPL-3.0). See THIRD_PARTY.md.

// ==WindhawkModReadme==
/*
# KDE Pinch Zoom — experimental

Spread three fingers to magnify the desktop; pinch them together to zoom out.
Two-finger app zoom and Windows three/four-finger swipe settings are untouched.
A Precision Touchpad is required. Native swipes and pinch recognition run in
parallel: hardware validation is needed to rule out accidental swipe actions.
This version does not selectively suppress Windows gestures.

Ctrl+Alt+Shift+F10 resets the zoom. Ctrl+Alt+Shift+F9 pauses/resumes the mod.
Disabling/unloading restores the previous view if the mod still owns it.
Do not run Windows Magnifier or another desktop zoom tool simultaneously.

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
- sensitivity: 150
  $name: Zoom sensitivity (%)
  $description: 100 follows finger spread directly; higher values zoom faster.
- threshold: 6
  $name: Pinch activation threshold (%)
  $description: Minimum change in finger spread before zoom starts.
- maxZoom: 400
  $name: Maximum zoom (%)
- smoothing: 40
  $name: Smoothing time (milliseconds)
  $description: Set to 0 for direct tracking.
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
    double sensitivity = 1.5;
    double threshold = 0.06;
    double maxZoom = 4.0;
    double smoothingMs = 40;
    double fallbackAspect = 0.60;
    bool enabled = true;
    bool observeOnly = false;
    bool debug = false;
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
    std::uint64_t lastTime_ = 0;

public:
    void reset() { phase_ = Phase::Idle; }
    Phase phase() const { return phase_; }
    Decision update(const std::vector<Contact>& frame, std::uint64_t now,
                    double currentZoom, const Config& config) {
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
            phase_ = radius >= 3 ? Phase::Pending : Phase::Rejected;
            lastTime_ = now;
            return {phase_, std::nullopt, false};
        }
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
            spread >= 0.6 && travel <= std::max(0.8, spread * 0.8)) {
            phase_ = Phase::Pinch;
            began = true;
        }
        if (phase_ != Phase::Pinch) {
            return {phase_, std::nullopt, false};
        }
        const double zoom = std::clamp(zoom_ * std::pow(scale, config.sensitivity),
                                       1.0, config.maxZoom);
        return {phase_, zoom, began};
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
#include <string>

#ifndef PINCH_ZOOM_STANDALONE
#include <windhawk_api.h>
#endif

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
    bool initialize() {
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
    double current() const { return read().value_or(pinch::View{}).zoom; }
    bool target(double value) {
        if (!initialized_) {
            return false;
        }
        if (!owned_) {
            const auto original = read();
            if (!original || original->zoom > 1.001) {
                log(L"Another magnifier is active; zoom skipped. Close it and try again.");
                return false;
            }
            original_ = last_ = *original;
            owned_ = true;
        }
        target_ = value;
        return true;
    }
    bool tick(double elapsed, double smoothing) {
        if (!owned_) {
            return true;
        }
        const auto actual = read();
        if (!actual || !same(*actual, last_)) {
            owned_ = false;
            log(L"Magnification changed externally; released control.");
            return false;
        }
        POINT cursor{};
        if (!GetCursorPos(&cursor)) {
            return true;
        }
        const double level = pinch::smooth(last_.zoom, target_, elapsed, smoothing);
        const auto next = pinch::viewAt(level, {static_cast<double>(cursor.x),
                                              static_cast<double>(cursor.y)},
                                      {static_cast<double>(GetSystemMetrics(SM_XVIRTUALSCREEN)),
                                       static_cast<double>(GetSystemMetrics(SM_YVIRTUALSCREEN)),
                                       static_cast<double>(GetSystemMetrics(SM_CXVIRTUALSCREEN)),
                                       static_cast<double>(GetSystemMetrics(SM_CYVIRTUALSCREEN))});
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
        target_ = 1;
    }
    ~Zoom() {
        restore();
        if (initialized_) {
            uninitialize_();
        }
        if (module_) FreeLibrary(module_);
    }
};

struct Statistics {
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
    Statistics statistics_;

    void resetGestures() {
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
    void frame(HANDLE handle, Device& dev, const std::vector<pinch::Contact>& contacts, ULONGLONG now) {
        ++statistics_.frames;
        if (paused_ || !sessionActive_ || !config_.enabled) {
            return;
        }
        if (waitForRelease_) {
            if (std::none_of(contacts.begin(), contacts.end(), [](const pinch::Contact& c) { return c.down; })) {
                waitForRelease_ = false;
            }
            return;
        }
        const auto before = dev.recognizer.phase();
        const auto decision = dev.recognizer.update(contacts, now, zoom_.current(), config_);
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
            if (config_.debug) {
                log(L"Three-finger pinch recognized.");
            }
        }
        if (decision.zoom && (!activeDevice_ || activeDevice_ == handle)) {
            activeDevice_ = handle;
            if (!config_.observeOnly && !zoom_.target(*decision.zoom)) {
                dev.recognizer.reset();
                activeDevice_ = nullptr;
            }
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
            return;
        }
        ++statistics_.reports;
        pinch::Report decoded;
        decoded.count = *contactCount;
        if (dev.hasScan) {
            decoded.scan = value(kDigitizer, dev.scanCollection, kScanTime);
        }
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
            frame(handle, dev, *complete, now);
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
        const auto& hid = raw->data.hid;
        const auto payload = static_cast<std::size_t>(received) - offsetof(RAWINPUT, data.hid.bRawData);
        if (!hid.dwSizeHid || hid.dwCount > payload / hid.dwSizeHid) {
            return;
        }
        if (auto* dev = device(raw->header.hDevice)) {
            for (DWORD i = 0; i < hid.dwCount; ++i) {
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
                if (!host->zoom_.tick(static_cast<double>(now - host->lastTick_), host->config_.smoothingMs)) {
                    host->resetGestures();
                    host->waitForRelease_ = true;
                }
                host->lastTick_ = now;
                return 0;
            }
            case WM_HOTKEY:
                host->zoom_.restore();
                host->resetGestures();
                host->waitForRelease_ = true;
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
                host->zoom_.restore();
                host->resetGestures();
                host->waitForRelease_ = true;
                host->config_ = *reinterpret_cast<const pinch::Config*>(l);
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
        lastTick_ = GetTickCount64();
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
        zoom_.restore();
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
    for (int i = 1; i < argc; ++i) {
        const std::wstring arg = argv[i];
        if (arg == L"--run") {
            config.observeOnly = false;
            seconds = 0;
        } else if (arg == L"--diagnose") {
            config.observeOnly = true;
        } else if (arg == L"--smoke-zoom") {
            smoke = true;
        } else if (arg == L"--seconds" && i + 1 < argc) {
            wchar_t* end = nullptr;
            const unsigned long parsed = std::wcstoul(argv[++i], &end, 10);
            if (!end || *end || parsed < 1 || parsed > 3600) {
                std::fputs("--seconds must be between 1 and 3600\n", stderr);
                return 2;
            }
            seconds = static_cast<unsigned>(parsed);
        } else {
            std::fputs("Usage: pinch-zoom-host [--diagnose | --run] [--seconds 1..3600] [--smoke-zoom]\n", stderr);
            return 2;
        }
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
            std::wprintf(L"Summary: devices=%u reports=%llu frames=%llu pinches=%llu swipes=%llu\n",
                         stats.devices, static_cast<unsigned long long>(stats.reports),
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

#else  // Windhawk tool-mod adapter

namespace tool {

HANDLE stopEvent = nullptr;
HANDLE thread = nullptr;
HANDLE mutex = nullptr;
HANDLE readyEvent = nullptr;
std::atomic<HWND> window{nullptr};
std::atomic<bool> started{false};
bool launcher = false;
bool child = false;

pinch::Config settings() {
    pinch::Config config;
    config.enabled = Wh_GetIntSetting(L"enabled") != 0;
    config.observeOnly = Wh_GetIntSetting(L"observeOnly") != 0;
    config.sensitivity = std::clamp(Wh_GetIntSetting(L"sensitivity"), 25, 500) / 100.0;
    config.threshold = std::clamp(Wh_GetIntSetting(L"threshold"), 2, 30) / 100.0;
    config.maxZoom = std::clamp(Wh_GetIntSetting(L"maxZoom"), 100, 1000) / 100.0;
    config.smoothingMs = std::clamp(Wh_GetIntSetting(L"smoothing"), 0, 250);
    config.fallbackAspect = std::clamp(Wh_GetIntSetting(L"fallbackHeight"), 20, 150) / 100.0;
    config.debug = Wh_GetIntSetting(L"debugLogging") != 0;
    return config;
}

DWORD WINAPI worker(void*) {
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&worker), &module);
    runtime::Host host(settings());
    if (host.initialize(module)) {
        window.store(host.window());
        started.store(true);
    }
    SetEvent(readyEvent);
    if (started.load()) {
        host.loop(stopEvent);
    }
    window.store(nullptr);
    return 0;
}

void WINAPI entryPoint() { ExitThread(0); }

bool initializeChild() {
    mutex = CreateMutexW(nullptr, FALSE, L"Local\\Windhawk_KdePinchZoom_" WH_MOD_ID);
    if (!mutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        return false;
    }
    // Avoid the child also becoming a normal Windhawk UI/daemon. This is the
    // documented tool-mod entry point interception, not a system gesture hook.
    auto* image = reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(image + dos->e_lfanew);
    if (!Wh_SetFunctionHook(image + nt->OptionalHeader.AddressOfEntryPoint,
                           reinterpret_cast<void*>(&entryPoint), nullptr)) {
        return false;
    }
    stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    readyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!stopEvent || !readyEvent) {
        return false;
    }
    thread = CreateThread(nullptr, 0, worker, nullptr, 0, nullptr);
    return thread && WaitForSingleObject(readyEvent, 10000) == WAIT_OBJECT_0 && started.load();
}

void spawn() {
    if (!launcher) return;
    wchar_t path[MAX_PATH];
    const DWORD size = GetModuleFileNameW(nullptr, path, ARRAYSIZE(path));
    if (!size || size >= ARRAYSIZE(path)) return;
    std::wstring command = L"\"" + std::wstring(path) + L"\" -tool-mod \"" WH_MOD_ID L"\"";
    // Windhawk's own CreateProcessW hook must not redirect this dedicated
    // process. This is the same launch mechanism as the upstream tool template.
    using CreateInternal = BOOL(WINAPI*)(HANDLE, LPCWSTR, LPWSTR, LPSECURITY_ATTRIBUTES,
        LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW,
        LPPROCESS_INFORMATION, PHANDLE);
    auto kernel = GetModuleHandleW(L"kernelbase.dll");
    const auto create = reinterpret_cast<CreateInternal>(GetProcAddress(kernel ? kernel : GetModuleHandleW(L"kernel32.dll"),
                                                                       "CreateProcessInternalW"));
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_FORCEOFFFEEDBACK | STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process{};
    if (!create || !create(nullptr, path, command.data(), nullptr, nullptr, FALSE,
                            NORMAL_PRIORITY_CLASS, nullptr, nullptr, &startup, &process, nullptr)) {
        Wh_Log(L"Could not start pinch-zoom tool process (%lu)", GetLastError());
        return;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
}

}  // namespace tool

BOOL Wh_ModInit() {
    DWORD session = 0;
    if (!ProcessIdToSessionId(GetCurrentProcessId(), &session) || !session) return FALSE;
    int count = 0;
    wchar_t** args = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!args) return FALSE;
    bool otherTool = false;
    bool service = false;
    for (int i = 1; i < count; ++i) {
        const std::wstring arg(args[i]);
        if (arg == L"-service" || arg == L"-service-start" || arg == L"-service-stop") service = true;
        if (arg == L"-tool-mod" && i + 1 < count) {
            tool::child = std::wstring(args[i + 1]) == WH_MOD_ID;
            otherTool = !tool::child;
        }
    }
    LocalFree(args);
    if (service || otherTool) return FALSE;
    if (tool::child) {
        if (!tool::initializeChild()) ExitProcess(1);
    } else {
        tool::launcher = true;
    }
    return TRUE;
}
void Wh_ModAfterInit() { tool::spawn(); }
BOOL Wh_ModSettingsChanged(BOOL* reload) {
    *reload = FALSE;
    if (const HWND window = tool::window.load()) {
        const auto config = tool::settings();
        // The config lives until synchronous delivery returns. The worker owns
        // every runtime object, avoiding concurrent mutation of recognizers.
        SendMessageW(window, runtime::kSettingsMessage, 0, reinterpret_cast<LPARAM>(&config));
    }
    return TRUE;
}
void Wh_ModUninit() {
    if (!tool::child) return;
    if (tool::stopEvent) SetEvent(tool::stopEvent);
    if (tool::thread) {
        WaitForSingleObject(tool::thread, INFINITE);
        CloseHandle(tool::thread);
    }
    if (tool::readyEvent) CloseHandle(tool::readyEvent);
    if (tool::stopEvent) CloseHandle(tool::stopEvent);
    if (tool::mutex) CloseHandle(tool::mutex);
    ExitProcess(0);
}

#endif  // PINCH_ZOOM_STANDALONE
#endif  // PINCH_ZOOM_CORE_TEST
