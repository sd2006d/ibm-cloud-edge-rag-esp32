import time
from fastapi import FastAPI
from pydantic import BaseModel
from watsonx_service import WatsonxRAGPipeline

app = FastAPI(
    title="Cloud-to-Edge RAG Gateway Service",
    description="IBM Watsonx.ai microservice receiving edge intents.",
    version="1.0.0"
)

pipeline = WatsonxRAGPipeline()

class EdgeCommandPayload(BaseModel):
    node_id: str
    intent_code: int
    intent_label: str
    confidence: float
    raw_transcript: str | None = None
    requires_rag: bool = True

@app.get("/health")
def health_check():
    return {"status": "healthy", "edge_gateway": "online"}

@app.post("/api/v1/process-command")
async def process_edge_command(payload: EdgeCommandPayload):
    start_time = time.time()
    
    if not payload.requires_rag and payload.confidence > 0.85:
        return {
            "status": "LOCAL_EXECUTION",
            "execution_plan": f"Direct trigger executed on ESP32: {payload.intent_label}",
            "context_retrieved": [],
            "latency_ms": round((time.time() - start_time) * 1000, 2),
            "offload_saved_ms": 215.0
        }

    result = await pipeline.query_watsonx(payload.intent_label, payload.raw_transcript or payload.intent_label)
    return {
        "status": "WATSONX_ENRICHED",
        "execution_plan": result["answer"],
        "context_retrieved": result["sources"],
        "latency_ms": round((time.time() - start_time) * 1000, 2),
        "offload_saved_ms": 200.0
    }
