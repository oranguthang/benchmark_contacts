# 📊 Контактный сервис — бенчмарк на разных языках

Этот проект создан для сравнения производительности микросервисов, реализованных на разных языках программирования, при одинаковом API и взаимодействии с PostgreSQL.

---

## 📖 Описание задачи

Каждый сервис реализует два эндпоинта:
- `POST /contacts` — создание контакта
- `GET /contacts` — получение списка контактов с фильтрами и пагинацией

Бенчмарк-клиент:
- выполняет **1 000 000 POST-запросов** для генерации данных
- выполняет **1 000 000 GET-запросов** по определённой схеме фильтрации
- замеряет общее и среднее время выполнения каждого типа запросов

---

## 📑 API-спецификация

### `POST /contacts`

**Запрос:**
```json
{
  "external_id": 12345,
  "phone_number": "+1234567890"
}
```

**Ответ:**
```json
{
  "id": "uuid",
  "external_id": 12345,
  "phone_number": "+1234567890",
  "date_created": "ISO datetime",
  "date_updated": "ISO datetime"
}
```

### `GET /contacts`

Query параметры:

- external_id — фильтр по external_id (опционально)
- phone_number — фильтр по номеру телефона (опционально)
- limit — количество записей (по умолчанию 10000, максимум 10000)
- offset — смещение выборки (по умолчанию 0)

**Ответ:**
```json
[
  {
    "id": "uuid",
    "external_id": 12345,
    "phone_number": "+1234567890",
    "date_created": "ISO datetime",
    "date_updated": "ISO datetime"
  }
]
```
## 📈 Бенчмарк-клиент

### Шаги:

1. Отправляет 1 000 000 POST-запросов:

   - `external_id` — случайное целое число в диапазоне от 0 до 1 000 000
   - `phone_number` — случайный валидный номер в формате +79999999999
   - все созданные `external_id` и `phone_number` сохраняются для последующих GET-запросов.

2. Выполняет 1 000 000 GET-запросов:

   - 300 000 запросов с фильтром `phone_number` (существующий в БД)
   - 300 000 запросов с фильтром `external_id` (существующий в БД)
   - 400 000 запросов с фильтром `external_id` в диапазоне `0 <= external_id <= 100000` (может как существовать, так и нет)

### Ограничение ответа:

- `limit` = 10000
- `offset` = 0

### Измеряем:

- Общее время всех POST-запросов
- Общее время всех GET-запросов
- Среднее время одного POST-запроса
- Среднее время одного GET-запроса

## 📦 Структура проекта

```
/
├── docker-compose.yml
├── python_aiohttp/
│   ├── Dockerfile
│   ├── requirements.txt
│   └── app/
│       ├── main.py
│       ├── models.py
│       ├── db.py
│       └── routes.py
├── benchmark_client/
│   └── benchmark.py (будет позже)
└── README.md
```

## 🚀 Как запустить

Выполнить Makefile, который сделает следующее:

- docker-compose down -v
- билд контейнеров
- поднимет сервисы БД и веб
- подождёт 5 сек (на всякий случай, чтобы БД успела подняться)
- запустит бенчмарк

Команда:

```bash
make benchmark-python
make benchmark-php
make benchmark-java
make benchmark-rust
make benchmark-go
make benchmark-node
```

### 📦 Другие удобные таргеты:

- make run-python — просто поднять сервисы без бенчмарка
- make logs-python — посмотреть логи Python сервиса
- make stop — остановить все контейнеры и удалить volume'ы

## 📌 Планируемые реализации:

- [x] Python (aiohttp)
- [x] Java (Spring Boot)
- [x] Golang
- [x] Rust
- [x] PHP
- [x] Node.js
