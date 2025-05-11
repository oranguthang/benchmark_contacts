FROM golang:1.23

WORKDIR /app

COPY go.mod go.sum ./
RUN go mod download

COPY ./app /app

RUN go build -o server .

EXPOSE 8080

CMD ["./server"]
