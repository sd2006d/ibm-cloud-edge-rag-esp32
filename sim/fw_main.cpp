// Host smoke test for the real firmware entry point.
//
// Compiles firmware/main/app_main.cpp WITHOUT ESP-IDF (ESP_PLATFORM
// undefined) and calls app_main() once, proving the firmware file builds and
// runs outside the ESP32 toolchain. Microphone/GPIO/MQTT hardware paths are
// TODOs in app_main.cpp and are not exercised here.

extern "C" void app_main(void);

int main() {
    app_main();
    return 0;
}
