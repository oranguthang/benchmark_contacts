FROM golang:1.23

WORKDIR /app

COPY . .

RUN go mod tidy
RUN go build -o benchmark .

ENV BASE_URL=http://localhost:8080
ENV POST_COUNT=10000
ENV GET_COUNT=10000
ENV POST_WORKERS=1000
ENV GET_WORKERS=1000

CMD ./benchmark --base-url="$BASE_URL" --post-count="$POST_COUNT" --get-count="$GET_COUNT" --post-workers="$POST_WORKERS" --get-workers="$GET_WORKERS"
