from rag_engine import LocalFAISSIndex

class WatsonxRAGPipeline:
    def __init__(self):
        self.vector_db = LocalFAISSIndex()

    def initialize(self):
        self.vector_db.build_industrial_manual_index()

    async def query_watsonx(self, intent: str, user_query: str) -> dict:
        docs = self.vector_db.similarity_search(user_query, k=2)
        answer = f"[Watsonx Granite LLM]: Executing industrial protocol for intent '{intent}'."
        return {
            "answer": answer,
            "sources": docs,
            "intent": intent
        }
