"""Tests for the cloud microservice: retrieval, extractive answers, API, and the
optional watsonx.ai backend.

The watsonx backend is tested only in its unconfigured state: it must report
itself as inactive and refuse to generate rather than fabricate a response.
"""

import os

import pytest
from fastapi.testclient import TestClient

from cloud_microservice.answer import NO_ANSWER, extract_answer
from cloud_microservice.app import app, retriever
from cloud_microservice.watsonx_backend import WatsonxBackend

client = TestClient(app)


def test_corpus_loads_ten_documents():
    assert len(retriever.docs) == 10


def test_retrieval_finds_emergency_stop_doc():
    hits = retriever.search("how do I trigger an emergency stop on the conveyor?", k=3)
    assert hits, "expected at least one relevant document"
    assert hits[0].doc_id == "doc01_conveyor_emergency_stop.txt"


def test_retrieval_is_deterministic():
    query = "boiler pressure alarm limits"
    first = [(h.doc_id, h.score) for h in retriever.search(query, k=3)]
    second = [(h.doc_id, h.score) for h in retriever.search(query, k=3)]
    assert first == second
    assert first, "expected the determinism check to run on real hits"


def test_retrieval_unrelated_query_returns_nothing():
    hits = retriever.search("best recipe for chocolate cake frosting", k=3)
    assert hits == []


def test_answer_cites_its_source():
    hits = retriever.search("emergency stop conveyor belt procedure", k=3)
    ans = extract_answer("how do I emergency stop the conveyor belt?", hits)
    assert ans["citations"], "answer must cite the document it quotes"
    assert ans["citations"][0]["doc_id"] == "doc01_conveyor_emergency_stop.txt"
    assert "emergency stop" in ans["answer"].lower()


def test_answer_does_not_fabricate():
    ans = extract_answer("how do I bake a chocolate cake?", [])
    assert ans["answer"] == NO_ANSWER
    assert ans["citations"] == []


def test_api_health_reports_corpus_size():
    r = client.get("/health")
    assert r.status_code == 200
    body = r.json()
    assert body["status"] == "ok"
    assert body["documents"] == 10


def test_api_ask_reports_real_latency():
    r = client.post("/ask", json={"query": "what is the lockout tagout procedure?", "k": 3})
    assert r.status_code == 200
    body = r.json()
    assert body["latency_ms"] > 0, "latency must be a real measured value"
    assert body["latency_ms"] < 1000, "local retrieval should answer in milliseconds"
    assert body["citations"], "answer must carry its source"


def test_watsonx_unconfigured_never_fabricates():
    for var in ("WATSONX_API_KEY", "WATSONX_PROJECT_ID"):
        os.environ.pop(var, None)
    backend = WatsonxBackend()
    assert not backend.is_configured()
    with pytest.raises(RuntimeError):
        backend.generate("anything", [])
    r = client.post("/ask-watsonx", json={"query": "emergency stop", "k": 1})
    assert r.status_code == 503
    assert "not configured" in r.json()["detail"]
