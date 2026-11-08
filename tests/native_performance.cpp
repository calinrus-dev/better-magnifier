// Opt-in benchmark: protocol handoff latency, not physical input-to-photon latency.
#define PINCH_ZOOM_STANDALONE
#define wmain diagnostic_main
#include "../mods/kde-pinch-zoom.wh.cpp"
#undef wmain
#include <iostream>

int wmain(int argc, wchar_t** argv) {
    if (argc != 2 || wcscmp(argv[1], L"--live") != 0) return 2;
    bridge::Mapping mapping;
    if (!mapping.open() || !mapping.magnifierAvailable()) return 3;
    const double initial = mapping.nativeLevel();
    LARGE_INTEGER frequency{}, begin{}, end{};
    QueryPerformanceFrequency(&frequency);
    std::vector<double> latency;
    int timeouts = 0;
    for (int i = 0; i < 300; ++i) {
        const auto frameStart = GetTickCount64();
        // Oscillate gently between 110 and 150 percent, at approximately 60 Hz.
        const double level = 1.3 + 0.2 * std::sin(i * 0.08);
        QueryPerformanceCounter(&begin);
        if (!mapping.requestLevel(level)) { ++timeouts; break; }
        const LONG sequence = mapping.get()->requestedSequence;
        while (mapping.get()->appliedSequence < sequence && GetTickCount64() - frameStart < 100) Sleep(1);
        QueryPerformanceCounter(&end);
        if (mapping.get()->appliedSequence < sequence) ++timeouts;
        else latency.push_back((end.QuadPart - begin.QuadPart) * 1000.0 / frequency.QuadPart);
        const auto elapsed = GetTickCount64() - frameStart;
        if (elapsed < 16) Sleep(static_cast<DWORD>(16 - elapsed));
    }
    mapping.requestLevel(initial);
    std::sort(latency.begin(), latency.end());
    const auto percentile = [&](double ratio) { return latency.empty() ? -1 : latency[static_cast<std::size_t>((latency.size() - 1) * ratio)]; };
    std::cout << "{\"updates\":" << latency.size() << ",\"timeouts\":" << timeouts
              << ",\"handoffMedianMs\":" << percentile(0.5) << ",\"handoffP95Ms\":" << percentile(0.95)
              << ",\"handoffMaxMs\":" << percentile(1) << "}\n";
    return timeouts ? 1 : 0;
}
