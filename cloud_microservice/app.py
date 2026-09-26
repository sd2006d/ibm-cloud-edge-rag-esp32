"""FastAPI gateway for the cloud side of the edge-RAG pipeline.

Endpoints:
    GET  /health      service status, corpus size, watsonx availability
    POST /retrieve    TF-IDF retrieval with real end-to-end latency (perf_counter)
    POST /ask         extractive local answer with citations and latency
    POST /ask-watsonx optional watsonx.ai generation; 503 unless real
                      credentials are configured (never a fabricated answer)
"""

import os
import time

from fastapi import FastAPI
from fastapi.responses import JSONResponse
from pydantic import BaseModel, Field

from .answer import extract_answer
from .retrieval import TfidfRetriever
from .watsonx_backend import WatsonxBackend


def _corpus_dir() -> str:
    return os.environ.get(
        "CORPUS_DIR",
        os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "corpus"),
    )


retriever = TfidfRetriever(_corpus_dir())
watsonx = WatsonxBackend()

app = FastAPI(title="Edge RAG Gateway", version="1.0.0")


class Query(BaseModel):
    query: str = Field(min_length=1, max_length=500)
    k: int = Field(default=3, ge=1, le=10)


@app.get("/health")
def health() -> dict:
    return {
        "status": "ok",
        "documents": len(retriever.docs),
        "watsonx_configured": watsonx.is_configured(),
    }


@app.post("/retrieve")
def retrieve(q: Query) -> dict:
    start = time.perf_counter()
    hits = retriever.search(q.query, k=q.k)
    latency_ms = (time.perf_counter() - start) * 1000.0
    return {
        "query": q.query,
        "latency_ms": round(latency_ms, 3),
        "results": [
            {"doc_id": h.doc_id, "title": h.title, "score": round(h.score, 4)}
            for h in hits
        ],
    }


@app.post("/ask")
def ask(q: Query) -> dict:
    start = time.perf_counter()
    hits = retriever.search(q.query, k=q.k)
    answer = extract_answer(q.query, hits)
    latency_ms = (time.perf_counter() - start) * 1000.0
    return {
        "query": q.query,
        "backend": "extractive-local",
        "latency_ms": round(latency_ms, 3),
        "answer": answer["answer"],
        "citations": answer["citations"],
    }


@app.post("/ask-watsonx")
def ask_watsonx(q: Query):
    if not watsonx.is_configured():
        return JSONResponse(
            status_code=503,
            content={
                "detail": (
                    "watsonx.ai backend is not configured. Set WATSONX_API_KEY "
                    "and WATSONX_PROJECT_ID to enable it; no answer was generated."
                )
            },
        )
    start = time.perf_counter()
    hits = retriever.search(q.query, k=q.k)
    text = watsonx.generate(q.query, hits)
    latency_ms = (time.perf_counter() - start) * 1000.0
    return {
        "query": q.query,
        "backend": f"watsonx:{watsonx.model_id}",
        "latency_ms": round(latency_ms, 3),
        "answer": text,
        "citations": [{"doc_id": h.doc_id, "title": h.title} for h in hits],
    }
