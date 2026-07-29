#include <stdio.h>
#include "intent_classifier.hpp"

int main() {
    printf("Starting IBM Cloud-to-Edge AI Node on ESP32...\n");
    EdgeIntentClassifier classifier;
    classifier.initialize();

    int16_t dummyAudio[512];
    for(int i = 0; i < 512; i++) {
        dummyAudio[i] = (i % 2 == 0) ? 8000 : -8000;
    }

    auto res = classifier.classifyAudioFrame(dummyAudio, 512);
    printf("Intent: %s, Cloud RAG Required: %s\n", 
           classifier.intentToString(res.intent), res.requiresCloudRAG ? "YES" : "NO");
    return 0;
}
