# Docker compose file name
COMPOSE_FILE=docker-compose.yml

# Services
PYTHON_SERVICES=db python_aiohttp benchmark_tests_python benchmark_python
PHP_SERVICES=db php_slim benchmark_tests_php benchmark_php
JAVA_SERVICES=db java_spring benchmark_tests_java benchmark_java
RUST_SERVICES=db rust_axum benchmark_tests_rust benchmark_rust
GO_SERVICES=db go_fiber benchmark_tests_go benchmark_go
NODE_SERVICES=db node_express benchmark_tests_node benchmark_node
C_SERVICES=db c_microhttpd benchmark_tests_c benchmark_c
CPP_SERVICES=db cpp_drogon benchmark_tests_cpp benchmark_cpp

# Common commands
down:
	docker-compose -f $(COMPOSE_FILE) down -v

up-python:
	docker-compose -f $(COMPOSE_FILE) up -d db python_aiohttp

up-php:
	docker-compose -f $(COMPOSE_FILE) up -d db php_slim

up-java:
	docker-compose -f $(COMPOSE_FILE) up -d db java_spring

up-rust:
	docker-compose -f $(COMPOSE_FILE) up -d db rust_axum

up-go:
	docker-compose -f $(COMPOSE_FILE) up -d db go_fiber

up-node:
	docker-compose -f $(COMPOSE_FILE) up -d db node_express

up-c:
	docker-compose -f $(COMPOSE_FILE) up -d db c_microhttpd

up-cpp:
	docker-compose -f $(COMPOSE_FILE) up -d db cpp_drogon

build-python:
	docker-compose -f $(COMPOSE_FILE) build $(PYTHON_SERVICES)

build-php:
	docker-compose -f $(COMPOSE_FILE) build $(PHP_SERVICES)

build-java:
	docker-compose -f $(COMPOSE_FILE) build $(JAVA_SERVICES)

build-rust:
	docker-compose -f $(COMPOSE_FILE) build $(RUST_SERVICES)

build-go:
	docker-compose -f $(COMPOSE_FILE) build $(GO_SERVICES)

build-node:
	docker-compose -f $(COMPOSE_FILE) build $(NODE_SERVICES)

build-c:
	docker-compose -f $(COMPOSE_FILE) build $(C_SERVICES)

build-cpp:
	docker-compose -f $(COMPOSE_FILE) build $(CPP_SERVICES)

# Benchmark for Python
benchmark-python: down build-python up-python
	@echo "Waiting for service to start..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_python
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_python

# Benchmark for PHP
benchmark-php: down build-php up-php
	@echo "Waiting for service to start..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_php
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_php

# Benchmark for Java
benchmark-java: down build-java up-java
	@echo "Waiting for service to start..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_java
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_java

# Benchmark for Rust
benchmark-rust: down build-rust up-rust
	@echo "Waiting for service to start..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_rust
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_rust

# Benchmark for Go
benchmark-go: down build-go up-go
	@echo "Waiting for service to start..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_go
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_go

# Benchmark for Node.js
benchmark-node: down build-node up-node
	@echo "Waiting for service to start..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_python
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_python

# Benchmark for C
benchmark-c: down build-c up-c
	@echo "Waiting for service to start..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_c
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_c

# Benchmark for CPP
benchmark-cpp: down build-cpp up-cpp
	@echo "Waiting for service to start..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_cpp
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_cpp

# Full stop of all containers
stop:
	docker-compose -f $(COMPOSE_FILE) down -v

# Run only without benchmark
run-python: down build-python up-python
	@echo "Python services started."

run-php: down build-php up-php
	@echo "PHP services started."

run-java: down build-java up-java
	@echo "Java services started."

run-rust: down build-rust up-rust
	@echo "Rust services started."

run-go: down build-go up-go
	@echo "Go services started."

run-node: down build-node up-node
	@echo "Node.js services started."

run-c: down build-c up-c
	@echo "C services started."

run-cpp: down build-cpp up-cpp
	@echo "C++ services started."

# Logs for Python service
logs-python:
	docker-compose -f $(COMPOSE_FILE) logs -f python_aiohttp

# Logs for PHP service
logs-php:
	docker-compose -f $(COMPOSE_FILE) logs -f php_slim

# Logs for Java service
logs-java:
	docker-compose -f $(COMPOSE_FILE) logs -f java_spring

# Logs for Rust service
logs-rust:
	docker-compose -f $(COMPOSE_FILE) logs -f rust_axum

# Logs for Go service
logs-go:
	docker-compose -f $(COMPOSE_FILE) logs -f go_fiber

# Logs for Node.js service
logs-node:
	docker-compose -f $(COMPOSE_FILE) logs -f node_express

# Logs for C service
logs-c:
	docker-compose -f $(COMPOSE_FILE) logs -f c_microhttpd

# Logs for C++ service
logs-cpp:
	docker-compose -f $(COMPOSE_FILE) logs -f cpp_drogon

.PHONY: down up-python up-php up-java up-rust up-go up-node up-c up-cpp build-python build-php build-java build-rust build-go build-node build-c build-cpp benchmark-python benchmark-php benchmark-java benchmark-rust benchmark-go benchmark-node benchmark-c benchmark-cpp stop run-python run-php run-java run-rust run-go run-node run-c run-cpp logs-python logs-php logs-java logs-rust logs-go logs-node logs-c logs-cpp
