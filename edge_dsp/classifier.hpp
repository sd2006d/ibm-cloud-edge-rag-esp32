// Acoustic urgency classifier for the ESP32 edge node.
//
// Decides, from a short frame of microphone samples, whether the acoustic
// scene looks like an emergency (a loud broadband burst, e.g. a crash or
// rupture) or something benign. The classifier is plain ISO C++ with no
// ESP-IDF dependency so it can be unit-tested on a Linux host; the ESP32
// firmware in firmware/ calls this same code.
//
// Features: RMS energy, zero-crossing rate, and a small DFT used for
// spectral flatness (broadband vs. tonal discrimination).
//
// Decision rules (tuned for 8 kHz audio, ~1024-sample frames):
//   * RMS below QUIET_RMS_FLOOR          -> UNKNOWN (too quiet to judge)
//   * spectrum dominated by one tone      -> UNKNOWN (e.g. mains/machinery hum)
//   * loud + high ZCR + spectrally flat   -> EMERGENCY_STOP
//   * anything else                       -> UNKNOWN
//
// Null or empty input is handled safely and yields UNKNOWN with 0 confidence.

#ifndef EDGE_DSP_CLASSIFIER_HPP
#define EDGE_DSP_CLASSIFIER_HPP

#include <cstddef>

namespace edge_dsp {

enum class Urgency {
    UNKNOWN = 0,
    EMERGENCY_STOP = 1,
};

struct Classification {
    Urgency label = Urgency::UNKNOWN;
    float confidence = 0.0f;        // 0..1
    float rms = 0.0f;               // root-mean-square sample amplitude
    float zero_crossing_rate = 0.0f; // fraction of adjacent sample pairs that change sign
    float spectral_flatness = 0.0f; // 0 (pure tone) .. ~1 (white noise)
};

// Classify one frame. `samples` holds `count` normalized samples in [-1, 1].
// Safe to call with samples == nullptr or count == 0 (returns UNKNOWN).
Classification classify_urgency(const float* samples, std::size_t count,
                                float sample_rate_hz);

const char* urgency_to_string(Urgency u);

} // namespace edge_dsp

#endif // EDGE_DSP_CLASSIFIER_HPP
