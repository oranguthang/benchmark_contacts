# Имя compose файла
COMPOSE_FILE=docker-compose.yml

# Сервисы
PYTHON_SERVICES=python_db python_aiohttp benchmark_python

# Общие команды
down:
	docker-compose -f $(COMPOSE_FILE) down -v

up-python:
	docker-compose -f $(COMPOSE_FILE) up -d python_db python_aiohttp

build-python:
	docker-compose -f $(COMPOSE_FILE) build $(PYTHON_SERVICES)

# Бенчмарк для Python
benchmark-python: down build-python up-python
	@echo "Ожидание запуска сервиса..."
	sleep 5
	docker-compose -f $(COMPOSE_FILE) run --rm benchmark_python

# Полная остановка всех контейнеров
stop:
	docker-compose -f $(COMPOSE_FILE) down -v

# Только запуск без бенчмарка
run-python: down build-python up-python
	@echo "Сервисы Python запущены."

# Логи для Python сервиса
logs-python:
	docker-compose -f $(COMPOSE_FILE) logs -f python_aiohttp

.PHONY: down up-python build-python benchmark-python stop run-python logs-python
