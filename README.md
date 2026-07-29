# Cloud-to-Edge AI Integration (IBM Watsonx.ai + ESP32)

[![IBM Watsonx](https://img.shields.io/badge/IBM-Watsonx.ai-blue.svg)](https://www.ibm.com/watsonx)
[![ESP32 Edge](https://img.shields.io/badge/Hardware-ESP32%20S3-green.svg)](https://www.espressif.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Tests Passing](https://img.shields.io/badge/Tests-8%2F8%20Passed-brightgreen.svg)]()

> **IBM TechXchange Hackathon - Top 5 Finalist Project (Aug 2025)**

## Architecture Overview

This repository implements a **hybrid Cloud-to-Edge AI pipeline** designed for secure, low-bandwidth voice commanding in noisy industrial IoT environments.

## Repository Structure

├── edge/
│   └── esp32_firmware/
│       ├── CMakeLists.txt
│       └── main/
│           ├── main.cpp
│           ├── intent_classifier.hpp
│           └── intent_classifier.cpp
├── cloud_microservice/
│   ├── app.py
│   ├── watsonx_service.py
│   ├── rag_engine.py
│   └── requirements.txt
├── tests/
│   ├── test_intent_classifier.py
│   └── test_watsonx_rag.py
├── docker-compose.yml
└── README.md
