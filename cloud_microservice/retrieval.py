"""Deterministic TF-IDF + cosine-similarity retrieval over a local document corpus.

Pure standard library: no model calls, no network, no randomness. The same
query against the same corpus always produces the same ranking.
"""

import math
import os
import re
from dataclasses import dataclass

_TOKEN_RE = re.compile(r"[a-z0-9]+")

_STOPWORDS = frozenset(
    """
    a an the and or but if then else when while of at by for with about into
    through during before after above below to from up down in out on off over
    under again further once here there all any both each few more most other
    some such no nor not only own same so than too very can will just don
    should now is are was were be been being has have had do does did doing
    would could ought i you he she it we they them his her its our their this
    that these those as
    """.split()
)


@dataclass
class ScoredDoc:
    doc_id: str
    title: str
    text: str
    score: float


def tokenize(text: str) -> list:
    """Lowercase alphanumeric tokens with a small stopword list removed."""
    return [t for t in _TOKEN_RE.findall(text.lower()) if t not in _STOPWORDS]


class TfidfRetriever:
    def __init__(self, corpus_dir: str):
        self.docs: list = []
        self._idf: dict = {}
        self._doc_vectors: list = []
        self._load(corpus_dir)
        self._build_index()

    def _load(self, corpus_dir: str) -> None:
        for name in sorted(os.listdir(corpus_dir)):
            if not name.endswith(".txt"):
                continue
            with open(os.path.join(corpus_dir, name), encoding="utf-8") as f:
                text = f.read().strip()
            title = text.splitlines()[0].strip() if text else name
            self.docs.append({"doc_id": name, "title": title, "text": text})
        if not self.docs:
            raise ValueError(f"no .txt documents found in {corpus_dir}")

    def _build_index(self) -> None:
        df: dict = {}
        tokenized: list = []
        for doc in self.docs:
            toks = tokenize(doc["text"])
            tokenized.append(toks)
            for t in set(toks):
                df[t] = df.get(t, 0) + 1
        n = len(self.docs)
        self._idf = {t: math.log((n + 1) / (c + 1)) + 1.0 for t, c in df.items()}
        for toks in tokenized:
            tf: dict = {}
            for t in toks:
                tf[t] = tf.get(t, 0) + 1
            vec = {t: (1.0 + math.log(c)) * self._idf[t] for t, c in tf.items()}
            norm = math.sqrt(sum(v * v for v in vec.values())) or 1.0
            self._doc_vectors.append({t: v / norm for t, v in vec.items()})

    def search(self, query: str, k: int = 3) -> list:
        """Return up to k docs with cosine score > 0, ranked deterministically."""
        qtoks = tokenize(query)
        if not qtoks:
            return []
        qtf: dict = {}
        for t in qtoks:
            qtf[t] = qtf.get(t, 0) + 1
        qvec = {}
        for t, c in qtf.items():
            idf = self._idf.get(t)
            if idf is None:
                continue
            qvec[t] = (1.0 + math.log(c)) * idf
        qnorm = math.sqrt(sum(v * v for v in qvec.values()))
        if qnorm == 0.0:
            return []
        qvec = {t: v / qnorm for t, v in qvec.items()}

        scored = []
        for doc, dvec in zip(self.docs, self._doc_vectors):
            score = sum(qvec[t] * dvec.get(t, 0.0) for t in qvec)
            scored.append(ScoredDoc(doc["doc_id"], doc["title"], doc["text"], score))
        # Sort by score desc, doc_id asc: fully deterministic, no ties-by-chance.
        scored.sort(key=lambda s: (-s.score, s.doc_id))
        return [s for s in scored[:k] if s.score > 0.0]
