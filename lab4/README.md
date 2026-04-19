# Budget Service

REST API сервис для управления личным бюджетом. Применён паттерн polyglot persistence:
пользователи хранятся в PostgreSQL, планируемые доходы и расходы - в MongoDB.

Реализован на C++ с использованием фреймворка [userver](https://userver.tech/).

## Файлы результата

- [`schema_design.md`](schema_design.md) - описание проектирования документной модели с обоснованием выбора polyglot persistence и embedded/references
- [`data.js`](db/mongo/data.js) - скрипт с тестовыми данными для MongoDB (доходы/расходы/счётчики)
- [`queries.js`](db/mongo/queries.js) - MongoDB запросы для всех CRUD операций и aggregation pipelines
- [`validation.js`](db/mongo/validation.js) - скрипт с валидацией схем, создание коллекций и индексов, проверка невалидной вставки

## Архитектура

- **Фреймворк:** userver (C++17, асинхронный)
- **PostgreSQL 16:** таблица `users` (bcrypt через `pgcrypto`, GIN trgm для ILIKE-поиска)
- **MongoDB 8.0:** коллекции `planned_incomes`, `planned_expenses`, `counters`
- **Аутентификация:** JWT (Bearer Token, HS256)
- **Формат данных:** JSON

### Хранилища

| Сущность | СУБД | Обоснование |
|----------|------|-------------|
| `users` | PostgreSQL | Уникальный логин, bcrypt-хэши, поиск по триграммам |
| `planned_incomes` | MongoDB | Гибкая мета-информация, агрегации по периодам |
| `planned_expenses` | MongoDB | Гибкая мета-информация, агрегации по периодам |
| `counters` | MongoDB | Атомарная генерация id через `findAndModify + $inc` |

### Валидация

Коллекции `planned_incomes` и `planned_expenses` защищены `$jsonSchema`
с уровнем `strict` / `error`:

- обязательные поля (`required`)
- типы данных (`bsonType`)
- ограничения: `minLength` / `maxLength`, `minimum`, `pattern`

### Индексы

| Индекс | СУБД | Тип | Назначение |
|--------|------|-----|------------|
| `users.login` | Postgres | Unique | Поиск и проверка уникальности логина |
| `users.first_name`, `users.last_name` | Postgres | GIN trgm | Поиск по маске имени/фамилии (ILIKE) |
| `planned_incomes.user_id + date` | Mongo | Compound | Выборка доходов и аналитика за период |
| `planned_expenses.user_id + date` | Mongo | Compound | Выборка расходов и аналитика за период |

## Быстрый запуск

```
cd lab4
docker compose up --build
```

Сервис будет доступен на `http://localhost:8080`.
