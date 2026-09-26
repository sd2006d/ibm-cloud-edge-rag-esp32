// ESP32 firmware entry point for the edge node.
//
// This file targets ESP-IDF, but every ESP-IDF-only include and call is
// guarded by #ifdef ESP_PLATFORM, so it also compiles as plain C++ on a
// Linux host for smoke-testing (see sim/fw_main.cpp).
//
// CURRENT STATUS: the classifier runs here on a SYNTHETIC test buffer.
// Hardware I/O is NOT implemented yet:
//   TODO: capture real audio from an I2S microphone (driver/i2s_std.h)
//   TODO: on EMERGENCY_STOP, assert a GPIO-connected E-stop relay output
//   TODO: publish the classification result to the cloud gateway via MQTT/HTTP
// The README documents exactly what is and is not implemented.

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#else
#include <cstdio>
#endif

#include "../../edge_dsp/classifier.hpp"

#ifdef ESP_PLATFORM
static const char* TAG = "edge_rag";
#define LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#else
#define LOGI(...) std::printf(__VA_ARGS__), std::printf("\n")
#endif

namespace {

// 1024-sample synthetic broadband burst at 8 kHz: a stand-in for microphone
// input until I2S capture is implemented (see TODO at the top of this file).
void fill_synthetic_burst(float* buf, int n) {
    unsigned state = 12345u; // fixed seed: deterministic on every boot
    for (int i = 0; i < n; ++i) {
        state = state * 1664525u + 1013904223u;
        buf[i] = static_cast<float>((state >> 8) * (1.0 / 16777216.0) * 1.8 - 0.9);
    }
}

} // namespace

extern "C" void app_main(void) {
    constexpr int kFrame = 1024;
    constexpr float kSampleRateHz = 8000.0f;

    float frame[kFrame];
    fill_synthetic_burst(frame, kFrame); // TODO: replace with I2S mic capture

    const edge_dsp::Classification c =
        edge_dsp::classify_urgency(frame, kFrame, kSampleRateHz);
    LOGI("urgency=%s conf=%.2f rms=%.3f zcr=%.3f flat=%.3f",
         edge_dsp::urgency_to_string(c.label), c.confidence, c.rms,
         c.zero_crossing_rate, c.spectral_flatness);

    // TODO: on EMERGENCY_STOP, drive the GPIO E-stop relay.
    // TODO: publish this classification to the cloud gateway (MQTT/HTTP).

#ifdef ESP_PLATFORM
    vTaskDelay(pdMS_TO_TICKS(1000));
#endif
}
