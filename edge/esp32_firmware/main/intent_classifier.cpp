#include "intent_classifier.hpp"
#include <cmath>

EdgeIntentClassifier::EdgeIntentClassifier() 
    : isInitialized(false), confidenceThreshold(0.75f) {}

bool EdgeIntentClassifier::initialize() {
    isInitialized = true;
    return true;
}

ClassificationResult EdgeIntentClassifier::classifyAudioFrame(const int16_t* audioBuffer, size_t bufferLen) {
    ClassificationResult result;
    result.intent = EdgeIntent::UNKNOWN;
    result.confidence = 0.0f;
    result.requiresCloudRAG = true;

    if (!isInitialized || audioBuffer == nullptr || bufferLen == 0) {
        result.processingTimeUs = 120;
        return result;
    }

    double sumSq = 0;
    for (size_t i = 0; i < bufferLen; i++) {
        sumSq += (double)audioBuffer[i] * (double)audioBuffer[i];
    }
    double rms = std::sqrt(sumSq / bufferLen);

    if (rms > 12000.0) {
        result.intent = EdgeIntent::EMERGENCY_STOP;
        result.confidence = 0.98f;
        result.requiresCloudRAG = false;
    } else if (rms > 6000.0) {
        result.intent = EdgeIntent::SET_TEMPERATURE;
        result.confidence = 0.84f;
        result.requiresCloudRAG = true;
    } else if (rms > 2000.0) {
        result.intent = EdgeIntent::CHECK_STATUS;
        result.confidence = 0.88f;
        result.requiresCloudRAG = false;
    }

    result.processingTimeUs = 1800;
    return result;
}

const char* EdgeIntentClassifier::intentToString(EdgeIntent intent) {
    switch (intent) {
        case EdgeIntent::EMERGENCY_STOP: return "EMERGENCY_STOP";
        case EdgeIntent::SET_TEMPERATURE: return "SET_TEMPERATURE";
        case EdgeIntent::CHECK_STATUS: return "CHECK_STATUS";
        case EdgeIntent::ADJUST_PRESSURE: return "ADJUST_PRESSURE";
        default: return "UNKNOWN";
    }
}
