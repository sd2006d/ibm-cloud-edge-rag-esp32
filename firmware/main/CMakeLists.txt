# ESP-IDF component for the edge node.
# app_main.cpp is also compilable on a Linux host (ESP_PLATFORM undefined);
# see sim/fw_main.cpp for the host smoke test.
idf_component_register(
    SRCS "app_main.cpp" "../../edge_dsp/classifier.cpp"
    INCLUDE_DIRS "." "../../edge_dsp"
)
