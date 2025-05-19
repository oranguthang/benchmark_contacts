# Имя compose файла
COMPOSE_FILE=docker-compose.yml

# Сервисы
PYTHON_SERVICES=db python_aiohttp benchmark_tests_python benchmark_python
PHP_SERVICES=db php_slim benchmark_tests_php benchmark_php
JAVA_SERVICES=db java_spring benchmark_tests_java benchmark_java
RUST_SERVICES=db rust_axum benchmark_tests_rust benchmark_rust
GO_SERVICES=db go_fiber benchmark_tests_go benchmark_go
NODE_SERVICES=db node_express benchmark_tests_node benchmark_node
C_SERVICES=db c_microhttpd benchmark_tests_c benchmark_c
CPP_SERVICES=db cpp_crow benchmark_tests_cpp benchmark_cpp

# Общие команды
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
	docker-compose -f $(COMPOSE_FILE) up -d db cpp_crow

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

# Бенчмарк для Python
benchmark-python: down build-python up-python
	@echo "Ожидание запуска сервиса..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_python
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_python

# Бенчмарк для PHP
benchmark-php: down build-php up-php
	@echo "Ожидание запуска сервиса..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_php
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_php

# Бенчмарк для Java
benchmark-java: down build-java up-java
	@echo "Ожидание запуска сервиса..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_java
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_java

# Бенчмарк для Rust
benchmark-rust: down build-rust up-rust
	@echo "Ожидание запуска сервиса..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_rust
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_rust

# Бенчмарк для Go
benchmark-go: down build-go up-go
	@echo "Ожидание запуска сервиса..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_go
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_go

# Бенчмарк для Node.js
benchmark-node: down build-node up-node
	@echo "Ожидание запуска сервиса..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_python
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_python

# Бенчмарк для C
benchmark-c: down build-c up-c
	@echo "Ожидание запуска сервиса..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_c
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_c

# Бенчмарк для CPP
benchmark-cpp: down build-cpp up-cpp
	@echo "Ожидание запуска сервиса..."
	sleep 1
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_tests_cpp
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_cpp

# Полная остановка всех контейнеров
stop:
	docker-compose -f $(COMPOSE_FILE) down -v

# Только запуск без бенчмарка
run-python: down build-python up-python
	@echo "Сервисы Python запущены."

run-php: down build-php up-php
	@echo "Сервисы PHP запущены."

run-java: down build-java up-java
	@echo "Сервисы Java запущены."

run-rust: down build-rust up-rust
	@echo "Сервисы Rust запущены."

run-go: down build-go up-go
	@echo "Сервисы Go запущены."

run-node: down build-node up-node
	@echo "Сервисы Node.js запущены."

run-c: down build-c up-c
	@echo "Сервисы C запущены."

run-cpp: down build-cpp up-cpp
	@echo "Сервисы C++ запущены."

# Логи для Python сервиса
logs-python:
	docker-compose -f $(COMPOSE_FILE) logs -f python_aiohttp

# Логи для PHP сервиса
logs-php:
	docker-compose -f $(COMPOSE_FILE) logs -f php_slim

# Логи для Java сервиса
logs-java:
	docker-compose -f $(COMPOSE_FILE) logs -f java_spring

# Логи для Rust сервиса
logs-rust:
	docker-compose -f $(COMPOSE_FILE) logs -f rust_axum

# Логи для Go сервиса
logs-go:
	docker-compose -f $(COMPOSE_FILE) logs -f go_fiber

# Логи для Node.js сервиса
logs-node:
	docker-compose -f $(COMPOSE_FILE) logs -f node_express

# Логи для C сервиса
logs-c:
	docker-compose -f $(COMPOSE_FILE) logs -f c_microhttpd

# Логи для C++ сервиса
logs-cpp:
	docker-compose -f $(COMPOSE_FILE) logs -f cpp_crow

.PHONY: down up-python up-php up-java up-rust up-go up-node up-c up-cpp build-python build-php build-java build-rust build-go build-node build-c build-cpp benchmark-python benchmark-php benchmark-java benchmark-rust benchmark-go benchmark-node benchmark-c benchmark-cpp stop run-python run-php run-java run-rust run-go run-node run-c run-cpp logs-python logs-php logs-java logs-rust logs-go logs-node logs-c logs-cpp
