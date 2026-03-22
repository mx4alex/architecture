# Вариант 12: Budget Service

REST API сервис для управления личным бюджетом.
Позволяет планировать доходы и расходы, а также отслеживать динамику бюджета за произвольный период.

Реализован на C++ с использованием асинхронного фреймворка [Yandex userver](https://userver.tech/).

## Архитектура

- **Фреймворк:** userver v2.8 (C++17)
- **Хранилище:** In-memory
- **Аутентификация:** JWT
- **Формат данных:** JSON

## API Endpoints

| Метод | URL | Описание | Аутентификация |
|-------|-----|----------|----------------|
| POST | `/api/v1/auth/register` | Регистрация пользователя | Нет |
| POST | `/api/v1/auth/login` | Аутентификация, получение токена | Нет |
| GET | `/api/v1/users/search` | Поиск по логину или имени/фамилии | Bearer Token |
| POST | `/api/v1/incomes` | Создать планируемый доход | Bearer Token |
| GET | `/api/v1/incomes` | Перечень планируемых доходов | Bearer Token |
| PUT | `/api/v1/incomes/{id}` | Обновить планируемый доход | Bearer Token |
| DELETE | `/api/v1/incomes/{id}` | Удалить планируемый доход | Bearer Token |
| POST | `/api/v1/expenses` | Создать планируемый расход | Bearer Token |
| GET | `/api/v1/expenses` | Перечень планируемых расходов | Bearer Token |
| PUT | `/api/v1/expenses/{id}` | Обновить планируемый расход | Bearer Token |
| DELETE | `/api/v1/expenses/{id}` | Удалить планируемый расход | Bearer Token |
| GET | `/api/v1/budget/dynamics` | Динамика бюджета за период | Bearer Token |
| GET | `/ping` | Health check | Нет |

## Запуск

```bash
docker compose up --build
```

Сервис будет доступен на `http://localhost:8080`.

## Примеры использования API

### Регистрация

```bash
curl -X POST http://localhost:8080/api/v1/auth/register \
  -H "Content-Type: application/json" \
  -d '{"login": "ivan", "password": "pass123", "first_name": "Иван", "last_name": "Петров"}'
```

Ответ (201 Created):
```json
{"id": 1, "login": "ivan", "first_name": "Иван", "last_name": "Петров"}
```

### Аутентификация

```bash
curl -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login": "ivan", "password": "pass123"}'
```

Ответ (200 OK):
```json
{"token": "eyJhbGciOiJIUzI1NiIs..."}
```

### Создание дохода

```bash
curl -X POST http://localhost:8080/api/v1/incomes \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{"category": "Зарплата", "amount": 150000.0, "date": "2024-01-15", "description": "Основная"}'
```

Ответ (201 Created):
```json
{"id": 1, "user_id": 1, "category": "Зарплата", "amount": 150000.0, "date": "2024-01-15", "description": "Основная"}
```

### Обновление дохода

```bash
curl -X PUT http://localhost:8080/api/v1/incomes/1 \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{"category": "Зарплата", "amount": 160000.0, "date": "2024-01-15", "description": "Повышение"}'
```

Ответ (200 OK):
```json
{"id": 1, "user_id": 1, "category": "Зарплата", "amount": 160000.0, "date": "2024-01-15", "description": "Повышение"}
```

### Удаление дохода

```bash
curl -X DELETE http://localhost:8080/api/v1/incomes/1 \
  -H "Authorization: Bearer <token>"
```

Ответ (200 OK):
```json
{"status": "deleted"}
```

### Динамика бюджета за период

```bash
curl "http://localhost:8080/api/v1/budget/dynamics?from=2024-01-13&to=2024-01-15" \
  -H "Authorization: Bearer <token>"
```

Ответ (200 OK):
```json
{
  "total_income": 150000.0,
  "total_expense": 25000.0,
  "balance": 125000.0,
  "period_from": "2024-01-13",
  "period_to": "2024-01-15",
  "daily": [
    {"date": "2024-01-13", "income": 0.0, "expense": 10000.0, "balance": -10000.0},
    {"date": "2024-01-14", "income": 0.0, "expense": 15000.0, "balance": -15000.0},
    {"date": "2024-01-15", "income": 150000.0, "expense": 0.0, "balance": 150000.0}
  ]
}
```

## HTTP коды ответов

| Код | Описание |
|-----|----------|
| 200 | Успешный запрос |
| 201 | Ресурс создан |
| 400 | Некорректные данные запроса |
| 401 | Требуется аутентификация / неверные креды |
| 404 | Ресурс не найден |
| 409 | Конфликт (дублирование логина) |
| 500 | Внутренняя ошибка сервера |

## OpenAPI документация

Полная спецификация API доступна в файле [docs/openapi.yaml](docs/openapi.yaml).
