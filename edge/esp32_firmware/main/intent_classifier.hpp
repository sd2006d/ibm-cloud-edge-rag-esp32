#ifndef INTENT_CLASSIFIER_HPP
#define INTENT_CLASSIFIER_HPP

#include <cstdint>
#include <string>

enum class EdgeIntent : uint8_t {
    UNKNOWN = 0,
    EMERGENCY_STOP = 1,
    SET_TEMPERATURE = 2,
    CHECK_STATUS = 3,
    ADJUST_PRESSURE = 4
};

struct ClassificationResult {
    EdgeIntent intent;
    float confidence;
    uint32_t processingTimeUs;
    bool requiresCloudRAG;
};

class EdgeIntentClassifier {
public:
    EdgeIntentClassifier();
    bool initialize();
    ClassificationResult classifyAudioFrame(const int16_t* audioBuffer, size_t bufferLen);
    const char* intentToString(EdgeIntent intent);

private:
    bool isInitialized;
    float confidenceThreshold;
};

#endif // INTENT_CLASSIFIER_HPP
