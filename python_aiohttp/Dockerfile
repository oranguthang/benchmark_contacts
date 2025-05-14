FROM python:3.12-slim

WORKDIR /app

RUN apt-get update && apt-get install -y curl

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY ./app /app

ENV CPU_CORES=8

EXPOSE 8080

CMD ["sh", "-c", "gunicorn -w $(($CPU_CORES * 4)) --worker-connections 2000 -k aiohttp.GunicornWebWorker -b 0.0.0.0:8080 main:init_app"]
