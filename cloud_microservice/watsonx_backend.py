"""Optional IBM watsonx.ai generation backend.

This backend is INACTIVE unless real credentials are supplied through the
environment. It never fabricates a model response: without credentials it
reports itself as unconfigured, and calling generate() raises instead of
returning a fake answer.

Environment variables:
    WATSONX_API_KEY     IBM Cloud API key (required)
    WATSONX_PROJECT_ID  watsonx.ai project id (required)
    WATSONX_URL         watsonx.ai endpoint (default: us-south)
    WATSONX_MODEL_ID    foundation model id (default: ibm/granite-3-8b-instruct)

The ibm-watsonx-ai SDK is intentionally not a hard dependency; install it
manually (`pip install ibm-watsonx-ai`) before enabling this backend.
"""

import os

DEFAULT_URL = "https://us-south.ml.cloud.ibm.com"
DEFAULT_MODEL = "ibm/granite-3-8b-instruct"


class WatsonxBackend:
    def __init__(self) -> None:
        self.api_key = os.environ.get("WATSONX_API_KEY", "")
        self.project_id = os.environ.get("WATSONX_PROJECT_ID", "")
        self.url = os.environ.get("WATSONX_URL", DEFAULT_URL)
        self.model_id = os.environ.get("WATSONX_MODEL_ID", DEFAULT_MODEL)

    def is_configured(self) -> bool:
        """True only when real credentials are present in the environment."""
        return bool(self.api_key and self.project_id)

    def generate(self, question: str, context_docs: list) -> str:
        """Generate an answer grounded in the retrieved context documents.

        Raises RuntimeError when unconfigured or when the SDK is missing,
        instead of returning a fabricated response.
        """
        if not self.is_configured():
            raise RuntimeError(
                "watsonx.ai backend is not configured: set WATSONX_API_KEY and "
                "WATSONX_PROJECT_ID environment variables. No response was generated."
            )
        try:
            from ibm_watsonx_ai import Credentials
            from ibm_watsonx_ai.foundation_models import ModelInference
        except ImportError as exc:
            raise RuntimeError(
                "The ibm-watsonx-ai package is not installed. Install it to use "
                "the watsonx.ai backend."
            ) from exc

        model = ModelInference(
            model_id=self.model_id,
            credentials=Credentials(url=self.url, api_key=self.api_key),
            project_id=self.project_id,
        )
        context = "\n\n".join(doc.text for doc in context_docs)
        prompt = (
            "Answer the question using ONLY the context below. "
            "If the answer is not in the context, say so explicitly.\n\n"
            f"Context:\n{context}\n\nQuestion: {question}\nAnswer:"
        )
        return model.generate_text(prompt=prompt)
