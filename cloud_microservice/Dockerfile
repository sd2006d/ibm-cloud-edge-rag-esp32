# Build context is the REPO ROOT (see docker-compose.yml), so COPY paths are
# relative to the repository root. Keep them in sync with the compose file.
FROM python:3.11-slim

WORKDIR /srv

COPY cloud_microservice/requirements.txt ./requirements.txt
RUN pip install --no-cache-dir -r requirements.txt

COPY cloud_microservice/ ./cloud_microservice/
COPY corpus/ ./corpus/

EXPOSE 8000

CMD ["uvicorn", "cloud_microservice.app:app", "--host", "0.0.0.0", "--port", "8000"]
