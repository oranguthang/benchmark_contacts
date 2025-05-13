FROM golang:1.23

WORKDIR /app

COPY ./app/go.mod /app
COPY ./app/go.sum /app

RUN go mod tidy

COPY ./app /app

RUN go build -o server .

EXPOSE 8080

ENV CPU_CORES=8

CMD ["./server"]
