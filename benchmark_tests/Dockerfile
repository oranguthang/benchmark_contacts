FROM python:3.12-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY test_api.py .

ENV BASE_URL=http://localhost:8080

CMD ["sh", "-c", "pytest -v test_api.py --base-url=$BASE_URL"]
