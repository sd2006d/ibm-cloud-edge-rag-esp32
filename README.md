# Cloud-to-Edge RAG Pipeline (IBM watsonx.ai + ESP32)

Voice commanding for noisy industrial sites. An ESP32-class edge node
classifies acoustic urgency locally (is that a crash or just background
hum?), while a cloud service answers questions grounded in equipment
manuals. Built for the IBM TechXchange hackathon (Aug 2025).

## How it works

```
microphone -> [ edge_dsp classifier ] -> urgency label
                                              |
user question -> [ FastAPI gateway ] -> [ TF-IDF retrieval ] -> extractive answer
                                              |                        + citations
                                              +-> (optional) watsonx.ai generation
```

**Cloud (`cloud_microservice/`)** - FastAPI gateway over 10 equipment
manuals in `corpus/`:

- TF-IDF + cosine retrieval, pure stdlib, deterministic, no model calls.
- Local extractive answers that quote the manual and cite the source. Says
  it doesn't know instead of guessing when nothing relevant comes back.
- Every `/retrieve` and `/ask` response includes `latency_ms`, measured
  with `perf_counter` around the real request handling.
- Optional watsonx.ai backend that only activates with real credentials
  (`WATSONX_API_KEY`, `WATSONX_PROJECT_ID`). Without them `/ask-watsonx`
  returns 503. It never fakes a model response.

**Edge (`edge_dsp/`)** - C++ acoustic urgency classifier with no ESP-IDF
dependency, so it builds on a normal Linux box for testing. From one audio
frame it computes RMS energy, zero-crossing rate, and a 256-point DFT for
spectral flatness (broadband vs. tonal):

- loud broadband burst -> `EMERGENCY_STOP`
- quiet background -> `UNKNOWN`
- loud 200 Hz hum (mains/machinery tone) -> `UNKNOWN`, not an emergency
- null/empty input -> `UNKNOWN`, handled safely

**Firmware (`firmware/`)** - the ESP-IDF entry point (`app_main()`).
ESP-IDF-only includes sit behind `#ifdef ESP_PLATFORM`, so the same file
compiles on a host for smoke-testing.

## Layout

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

## Testing

On a Linux box:

- 9/9 Python tests pass: retrieval correctness, determinism, cited answers,
  no-answer fallback instead of guessing, real latency numbers, watsonx
  backend refusing to fabricate without credentials.
- 4/4 C++ classifier tests pass: loud broadband burst -> `EMERGENCY_STOP`
  (confidence 0.87); quiet background -> `UNKNOWN`; loud 200 Hz hum ->
  `UNKNOWN`; null input handled safely.
- `firmware/main/app_main.cpp` compiles and runs on the host with
  `ESP_PLATFORM` undefined.

Haven't tested:

- No run on physical ESP32 hardware (no ESP-IDF toolchain here); the
  firmware classifies a clearly-labeled synthetic buffer for now.
- The watsonx.ai path was never exercised with real credentials.
- `docker compose build` was never run here (no Docker daemon); the compose
  build context and Dockerfile COPY paths are consistent by construction.

One thing I hit while building: the first tonal-detection metric (peak-bin
share) only reached 0.249 on a 200 Hz hum because of spectral leakage, so
the tonal branch never fired. Switched to a top-5-bin share metric
(hum 0.586 vs noise 0.088) and it works.

## TODO

Hardware integration: I2S mic capture, GPIO e-stop relay action, MQTT/HTTP
publishing of classifications. Marked in `firmware/main/app_main.cpp`.

## License

MIT, see [LICENSE](LICENSE).
