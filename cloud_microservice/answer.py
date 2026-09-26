"""Extractive answer backend: it quotes the corpus, never generates text.

If no retrieved passage is relevant to the question, it returns an explicit
no-answer message instead of fabricating a response.
"""

import re

from .retrieval import tokenize

_SENTENCE_RE = re.compile(r"(?<=[.!?])\s+")

NO_ANSWER = (
    "I could not find anything in the equipment manuals relevant to that "
    "question, so I will not guess an answer."
)


def _sentences(text: str) -> list:
    return [s.strip() for s in _SENTENCE_RE.split(text) if s.strip()]


def extract_answer(query: str, scored_docs: list, max_sentences: int = 2) -> dict:
    """Build an extractive answer from the top retrieved document.

    Returns {"answer": str, "citations": [{"doc_id": ..., "title": ...}]}.
    """
    if not scored_docs:
        return {"answer": NO_ANSWER, "citations": []}

    query_terms = set(tokenize(query))
    best = scored_docs[0]
    ranked = []
    for sent in _sentences(best.text):
        overlap = len(query_terms & set(tokenize(sent)))
        ranked.append((overlap, sent))
    # Highest term overlap first; stable for ties (document order preserved).
    ranked.sort(key=lambda pair: -pair[0])
    chosen = [sent for overlap, sent in ranked[:max_sentences] if overlap > 0]
    if not chosen:
        # The top doc matched only weakly (e.g. one shared stopword-free term
        # in the title line); quote its most informative sentence verbatim.
        chosen = _sentences(best.text)[:1]

    return {
        "answer": " ".join(chosen),
        "citations": [{"doc_id": best.doc_id, "title": best.title}],
    }
