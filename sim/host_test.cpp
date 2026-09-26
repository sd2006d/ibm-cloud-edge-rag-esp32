// Host test harness for edge_dsp/classifier.hpp.
//
// Builds with a normal C++ compiler (no ESP-IDF) and exercises the classifier
// against four SYNTHETIC signals. All signals are deterministic: white noise
// comes from a fixed-seed LCG, so results are reproducible run to run.
//
// Expected behavior:
//   1. loud broadband burst  -> EMERGENCY_STOP, high confidence
//   2. quiet background      -> UNKNOWN
//   3. loud pure 200 Hz hum  -> UNKNOWN (tonal, not an emergency)
//   4. null input            -> UNKNOWN, confidence 0, no crash
//
// Exit code 0 = all assertions passed.

#include <cmath>
#include <cstdio>
#include <vector>

#include "../edge_dsp/classifier.hpp"

namespace {

// Deterministic 32-bit LCG (Numerical Recipes constants).
struct Lcg {
    unsigned state;
    explicit Lcg(unsigned seed) : state(seed) {}
    // Uniform double in [0, 1).
    double next() {
        state = state * 1664525u + 1013904223u;
        return (state >> 8) * (1.0 / 16777216.0);
    }
};

constexpr float kSampleRate = 8000.0f;
constexpr std::size_t kFrame = 1024;

std::vector<float> synthetic_burst() {
    Lcg rng(12345);
    std::vector<float> s(kFrame);
    for (auto& v : s) v = static_cast<float>(rng.next() * 1.8 - 0.9); // [-0.9, 0.9]
    return s;
}

std::vector<float> synthetic_quiet() {
    Lcg rng(999);
    std::vector<float> s(kFrame);
    for (auto& v : s) v = static_cast<float>(rng.next() * 0.06 - 0.03); // [-0.03, 0.03]
    return s;
}

std::vector<float> synthetic_hum_200hz() {
    std::vector<float> s(kFrame);
    for (std::size_t i = 0; i < kFrame; ++i) {
        const double t = i / kSampleRate;
        s[i] = static_cast<float>(0.8 * std::sin(2.0 * 3.141592653589793 * 200.0 * t));
    }
    return s;
}

int failures = 0;

void check(bool cond, const char* name, const edge_dsp::Classification& c) {
    std::printf("[%s] label=%-14s conf=%.2f rms=%.3f zcr=%.3f flat=%.3f  %s\n",
                cond ? "PASS" : "FAIL", edge_dsp::urgency_to_string(c.label),
                c.confidence, c.rms, c.zero_crossing_rate, c.spectral_flatness,
                name);
    if (!cond) ++failures;
}

} // namespace

int main() {
    using edge_dsp::Urgency;

    auto burst = synthetic_burst();
    auto c1 = edge_dsp::classify_urgency(burst.data(), burst.size(), kSampleRate);
    check(c1.label == Urgency::EMERGENCY_STOP && c1.confidence >= 0.75,
          "loud broadband burst -> EMERGENCY_STOP (high confidence)", c1);

    auto quiet = synthetic_quiet();
    auto c2 = edge_dsp::classify_urgency(quiet.data(), quiet.size(), kSampleRate);
    check(c2.label == Urgency::UNKNOWN, "quiet background -> UNKNOWN", c2);

    auto hum = synthetic_hum_200hz();
    auto c3 = edge_dsp::classify_urgency(hum.data(), hum.size(), kSampleRate);
    check(c3.label == Urgency::UNKNOWN, "loud 200 Hz hum -> UNKNOWN", c3);

    auto c4 = edge_dsp::classify_urgency(nullptr, 0, kSampleRate);
    check(c4.label == Urgency::UNKNOWN && c4.confidence == 0.0f,
          "null input handled safely -> UNKNOWN", c4);

    std::printf(failures == 0 ? "\nALL HOST TESTS PASSED\n" : "\n%d TEST(S) FAILED\n",
                failures);
    return failures == 0 ? 0 : 1;
}
