# Cloud-to-Edge RAG Pipeline (IBM watsonx.ai + ESP32)

A hybrid cloud-to-edge AI pipeline for low-bandwidth voice commanding in noisy
industrial IoT environments: an ESP32-class edge node classifies acoustic
urgency locally, while a cloud microservice answers natural-language questions
grounded in a corpus of industrial equipment manuals.

Built for the IBM TechXchange hackathon (Aug 2025).

## How it works

```
microphone -> [ edge_dsp classifier ] -> urgency label
                                              |
user question -> [ FastAPI gateway ] -> [ TF-IDF retrieval ] -> extractive answer
                                              |                        + citations
                                              +-> (optional) watsonx.ai generation
```

**Cloud (`cloud_microservice/`)** — a FastAPI gateway over a 10-document
industrial-equipment manual corpus (`corpus/`):
- Deterministic TF-IDF + cosine-similarity retrieval (pure standard library,
  no model calls, no randomness).
- A local extractive answer backend that quotes the manuals and cites its
  source. When nothing relevant is retrieved it says so instead of guessing.
- Every `/retrieve` and `/ask` response includes `latency_ms` measured with
  `time.perf_counter()` around the real request handling.
- An optional watsonx.ai backend (`watsonx_backend.py`) that activates **only**
  with real credentials (`WATSONX_API_KEY`, `WATSONX_PROJECT_ID`). Without
  them it reports itself as unconfigured and `/ask-watsonx` returns 503. It
  never fabricates a model response.

**Edge (`edge_dsp/`)** — a C++ acoustic urgency classifier with no ESP-IDF
dependency, so it builds and runs on a normal Linux host for testing. From one
audio frame it computes RMS energy, zero-crossing rate, and a 256-point DFT
used for spectral flatness (broadband vs. tonal discrimination):
- loud broadband burst -> `EMERGENCY_STOP` (high confidence)
- quiet background -> `UNKNOWN`
- loud pure 200 Hz hum (machinery/mains tone) -> `UNKNOWN`, not an emergency
- null/empty input -> `UNKNOWN` with 0 confidence, handled safely

**Firmware (`firmware/`)** — the ESP-IDF entry point (`extern "C" void
app_main()`). ESP-IDF-only includes are guarded by `#ifdef ESP_PLATFORM`, so
the same file compiles on a host for smoke-testing.

## Repository layout

```
corpus/                  10 industrial-equipment manuals (.txt)
cloud_microservice/
  app.py                 FastAPI gateway (/health, /retrieve, /ask, /ask-watsonx)
  retrieval.py           deterministic TF-IDF + cosine retrieval
  answer.py              extractive, citation-carrying answer backend
  watsonx_backend.py     optional watsonx.ai path (credentials required)
  Dockerfile             build context = repo root (see docker-compose.yml)
edge_dsp/
  classifier.hpp/.cpp    acoustic urgency classifier (no ESP-IDF dependency)
firmware/main/
  app_main.cpp           ESP-IDF entry point, runs classifier on a synthetic buffer
  CMakeLists.txt         ESP-IDF component registration
sim/
  host_test.cpp          classifier unit tests (normal main(), synthetic signals)
  fw_main.cpp            host smoke test that calls the real app_main()
tests/
  test_retrieval_api.py  pytest suite for retrieval, answers, and the API
docker-compose.yml       builds the gateway image from the repo root
```

## Running it

Cloud gateway (Python 3.10+):

```bash
pip install -r cloud_microservice/requirements.txt
uvicorn cloud_microservice.app:app --reload
curl -X POST localhost:8000/ask \
  -H 'Content-Type: application/json' \
  -d '{"query": "how do I emergency stop the conveyor belt?"}'
```

Edge classifier tests (any C++17 compiler):

```bash
g++ -std=c++17 -O2 -Wall -Wextra -o host_test sim/host_test.cpp edge_dsp/classifier.cpp
./host_test
```

Firmware host smoke test:

```bash
g++ -std=c++17 -O2 -I edge_dsp -c firmware/main/app_main.cpp -o app_main.o
g++ -std=c++17 -O2 sim/fw_main.cpp app_main.o edge_dsp/classifier.cpp -o fw_smoke
./fw_smoke
```

Docker (build context is the repository root):

```bash
docker compose build
docker compose up   # gateway on http://localhost:8000
```

## What was verified, and what was not

Verified on a Linux host:
- 9/9 Python tests pass (retrieval correctness, determinism, extractive
  answers with citations, honest no-answer fallback, API latency reporting,
  watsonx backend refusing to fabricate without credentials).
- 4/4 C++ classifier host tests pass: loud broadband burst ->
  `EMERGENCY_STOP` (confidence 0.87); quiet background -> `UNKNOWN`; loud
  200 Hz hum -> `UNKNOWN`; null input handled safely.
- `firmware/main/app_main.cpp` compiles and runs on the host with
  `ESP_PLATFORM` undefined.

Not verified:
- No run on physical ESP32 hardware (no ESP-IDF toolchain here); the
  firmware currently classifies a clearly-labeled synthetic buffer.
- The watsonx.ai path was not exercised with real credentials or the
  `ibm-watsonx-ai` SDK installed.
- `docker compose build` was not run here (no Docker daemon); the compose
  build context and Dockerfile COPY paths were kept consistent by
  construction.

## Not implemented (explicitly)

Hardware integration does not exist yet and is marked TODO in
`firmware/main/app_main.cpp`: I2S microphone capture, GPIO E-stop relay
action, and MQTT/HTTP publishing of classifications. The README claims no
more than what the code does.
