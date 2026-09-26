// See classifier.hpp for the documented decision rules.

#include "classifier.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace edge_dsp {
namespace {

// Tunables (see header for rationale).
constexpr float kQuietRmsFloor = 0.08f;   // below this: too quiet to judge
constexpr float kLoudRms = 0.25f;         // above this: loud enough for an e-stop
constexpr float kNoisyZcr = 0.20f;        // above this: broadband-like zero crossings
constexpr float kFlatThreshold = 0.40f;   // above this: spectrally flat (broadband)
constexpr float kTonalTop5Ratio = 0.40f;  // top-5-bin share above this: tonal hum
constexpr std::size_t kDftSize = 256;     // small DFT window for flatness estimate

float compute_rms(const float* s, std::size_t n) {
    double acc = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        acc += static_cast<double>(s[i]) * s[i];
    }
    return static_cast<float>(std::sqrt(acc / static_cast<double>(n)));
}

float compute_zcr(const float* s, std::size_t n) {
    if (n < 2) return 0.0f;
    std::size_t crossings = 0;
    for (std::size_t i = 1; i < n; ++i) {
        if ((s[i - 1] < 0.0f && s[i] > 0.0f) || (s[i - 1] > 0.0f && s[i] < 0.0f)) {
            ++crossings;
        }
    }
    return static_cast<float>(crossings) / static_cast<float>(n - 1);
}

// Naive DFT magnitudes for bins 1..N/2-1 of the first kDftSize samples.
// O(N^2) with N=256 is ~65k multiply-adds: fine for occasional edge inference.
void dft_magnitudes(const float* s, std::size_t n, std::vector<float>& mags) {
    const std::size_t N = std::min(n, kDftSize);
    mags.assign(N / 2, 0.0f);
    const double two_pi_over_n = 2.0 * 3.14159265358979323846 / static_cast<double>(N);
    for (std::size_t k = 1; k < N / 2; ++k) {
        double re = 0.0, im = 0.0;
        for (std::size_t m = 0; m < N; ++m) {
            const double angle = two_pi_over_n * k * m;
            re += s[m] * std::cos(angle);
            im -= s[m] * std::sin(angle);
        }
        mags[k] = static_cast<float>(std::sqrt(re * re + im * im));
    }
}

float spectral_flatness(const std::vector<float>& mags) {
    // Geometric mean / arithmetic mean of the magnitude spectrum, in (0, 1].
    // White noise -> ~0.5-0.6 here; a pure tone -> near 0.
    double log_sum = 0.0, arith = 0.0;
    std::size_t count = 0;
    for (float m : mags) {
        const double v = static_cast<double>(m) + 1e-12;
        log_sum += std::log(v);
        arith += v;
        ++count;
    }
    if (count == 0 || arith <= 0.0) return 0.0f;
    const double geo = std::exp(log_sum / count);
    return static_cast<float>(geo / (arith / count));
}

float tonal_top5_ratio(const std::vector<float>& mags) {
    // Share of spectral magnitude carried by the 5 strongest bins.
    // A pure tone concentrates energy in a few bins (~0.59 for a 200 Hz hum
    // in the 256-point window); white noise spreads it thin (~0.09).
    // (Peak-bin share alone is unreliable: a non-integer bin leaks energy
    // across neighbours under a rectangular window.)
    std::vector<float> sorted = mags;
    std::sort(sorted.begin(), sorted.end(), std::greater<float>());
    double top5 = 0.0, total = 0.0;
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        total += sorted[i];
        if (i < 5) top5 += sorted[i];
    }
    return total > 0.0 ? static_cast<float>(top5 / total) : 0.0f;
}

float clamp01(float v) { return std::min(1.0f, std::max(0.0f, v)); }

} // namespace

const char* urgency_to_string(Urgency u) {
    return u == Urgency::EMERGENCY_STOP ? "EMERGENCY_STOP" : "UNKNOWN";
}

Classification classify_urgency(const float* samples, std::size_t count,
                                float sample_rate_hz) {
    Classification out;
    if (samples == nullptr || count == 0 || sample_rate_hz <= 0.0f) {
        return out; // UNKNOWN, confidence 0
    }

    out.rms = compute_rms(samples, count);
    out.zero_crossing_rate = compute_zcr(samples, count);

    std::vector<float> mags;
    dft_magnitudes(samples, count, mags);
    out.spectral_flatness = spectral_flatness(mags);

    if (out.rms < kQuietRmsFloor) {
        out.label = Urgency::UNKNOWN;
        out.confidence = 0.10f;
        return out;
    }

    if (tonal_top5_ratio(mags) > kTonalTop5Ratio) {
        // Loud but tonal (e.g. 200 Hz machinery hum): not an emergency burst.
        out.label = Urgency::UNKNOWN;
        out.confidence = 0.20f;
        return out;
    }

    if (out.rms > kLoudRms && out.zero_crossing_rate > kNoisyZcr &&
        out.spectral_flatness > kFlatThreshold) {
        out.label = Urgency::EMERGENCY_STOP;
        out.confidence = clamp01(0.65f + 0.20f * out.spectral_flatness +
                                 0.15f * std::min(1.0f, out.rms));
        return out;
    }

    out.label = Urgency::UNKNOWN;
    out.confidence = 0.30f;
    return out;
}

} // namespace edge_dsp
