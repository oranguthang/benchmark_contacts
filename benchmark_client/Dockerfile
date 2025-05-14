FROM golang:1.23

WORKDIR /app

COPY . .

RUN go mod tidy
RUN go build -o benchmark .

ENV BASE_URL=http://localhost:8080
ENV POST_COUNT=100000
ENV GET_COUNT=100000
ENV POST_WORKERS=5000
ENV GET_WORKERS=5000

ENV CPU_CORES=8

CMD ./benchmark --base-url="$BASE_URL" --post-count="$POST_COUNT" --get-count="$GET_COUNT" --post-workers="$POST_WORKERS" --get-workers="$GET_WORKERS"
