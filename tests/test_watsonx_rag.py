from cloud_microservice.rag_engine import LocalFAISSIndex

def test_vector_search():
    index = LocalFAISSIndex()
    index.build_industrial_manual_index()
    res = index.similarity_search("Emergency")
    assert len(res) > 0
