class LocalFAISSIndex:
    def __init__(self):
        self.documents = []

    def build_industrial_manual_index(self):
        self.documents = [
            "Industrial Safety Manual Section 4.2: Emergency shutdown requires immediate circuit breaker isolate.",
            "Temperature Regulation Doc 12: Maintain pressure threshold below 45 PSI when adjusting setpoint."
        ]

    def similarity_search(self, query: str, k: int = 2) -> list[str]:
        return self.documents[:k]
