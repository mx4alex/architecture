# Budget Service — PostgreSQL

REST API сервис для управления личным бюджетом с хранением данных в PostgreSQL.
Позволяет планировать доходы и расходы, а также отслеживать динамику бюджета за произвольный период.

Реализован на C++ с использованием фреймворка [Yandex userver](https://userver.tech/) и PostgreSQL.

## Архитектура

- **Фреймворк:** userver (C++17, асинхронный)
- **СУБД:** PostgreSQL 16
- **Аутентификация:** JWT (Bearer Token)
- **Хэширование паролей:** bcrypt 
- **Формат данных:** JSON

### Таблицы

| Таблица | Описание | 
|---------|----------|
| `users` | Пользователи системы |
| `planned_incomes` | Планируемые доходы |
| `planned_expenses` | Планируемые расходы |

### Ключевые ограничения

- `users.login` — UNIQUE (уникальный логин)
- `users.password` — хранится как bcrypt-хэш
- `planned_incomes.amount` — CHECK (amount > 0)
- `planned_expenses.amount` — CHECK (amount > 0)
- `planned_incomes.user_id` — FK users(id) ON DELETE CASCADE
- `planned_expenses.user_id` — FK users(id) ON DELETE CASCADE

### Индексы

| Индекс | Тип | Назначение |
|--------|-----|------------|
| `users_pkey` | B-tree (PK) | Поиск пользователя по id |
| `users_login_key` | B-tree (UNIQUE) | Поиск/проверка по логину |
| `idx_users_first_name_trgm` | GIN (pg_trgm) | ILIKE поиск по имени |
| `idx_users_last_name_trgm` | GIN (pg_trgm) | ILIKE поиск по фамилии |
| `idx_planned_incomes_user_id` | B-tree | Выборка доходов пользователя |
| `idx_planned_incomes_user_date` | B-tree | Динамика бюджета за период |
| `idx_planned_expenses_user_id` | B-tree | Выборка расходов пользователя |
| `idx_planned_expenses_user_date` | B-tree | Динамика бюджета за период |

## Запуск

```bash
docker compose up --build
```

Сервис будет доступен на `http://localhost:8080`.
