FROM golang:1.22-alpine

WORKDIR /app

COPY . .

RUN go mod init benchmark_client && go get github.com/google/uuid && go build -o benchmark .

CMD ["./benchmark"]
