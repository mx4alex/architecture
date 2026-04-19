# Проектирование документной модели

## 1. Подход: Polyglot Persistence

| Сущность | СУБД | Почему именно так |
|----------|------|-------------------|
| `users` | PostgreSQL | Жёсткая схема, уникальный логин, bcrypt-хэш через `pgcrypto`, поиск по триграммам, редкие изменения структуры |
| `planned_incomes`, `planned_expenses` | MongoDB | Документно-ориентированные: гибкая мета-информация (теги, категории, флаги повторяемости), большой объём чтений/записей на пользователя, естественные агрегации по датам и категориям |
| `counters` | MongoDB | Атомарная генерация id через `findAndModify + $inc` без отдельного SQL sequence |

Связь между PostgreSQL и MongoDB поддерживается на уровне приложения: `user_id` в Mongo-документах ссылается на `users.id`
из PostgreSQL. FK-ограничения здесь невозможны, поэтому консистентность обеспечивается логикой сервиса.

## 2. Коллекции MongoDB

### `planned_incomes`

Пример документа:

```json
{
  "id": 1,
  "user_id": 1,
  "category": "Зарплата",
  "amount": 150000.0,
  "date": "2026-01-15",
  "description": "Основная зарплата за январь",
  "tags": ["main"],
  "meta": { "recurring": true },
  "created_at": ISODate("2026-01-15T00:00:00Z")
}
```

Категории в данных: `Зарплата`, `Премия`, `Фриланс`, `Инвестиции`, `Подработка`, `Арендный доход`.

### `planned_expenses`

Пример документа:

```json
{
  "id": 1,
  "user_id": 1,
  "category": "Аренда",
  "amount": 45000.0,
  "date": "2026-01-05",
  "description": "Аренда квартиры",
  "tags": ["fixed"],
  "meta": { "recurring": true }
}
```

Категории в данных: `Аренда`, `Продукты`, `Транспорт`, `Коммунальные`, `Развлечения`, `Одежда`, `Ипотека`, `Здоровье`, `Образование`.

### `counters`

Техническая коллекция для генерации автоинкрементных идентификаторов. Каждый документ — отдельный счётчик:

```json
{ "_id": "incomes_id_seq",  "value": 15 }
{ "_id": "expenses_id_seq", "value": 15 }
```

Операция `findAndModify` с `$inc` и `returnNew: true` гарантирует атомарность и уникальность id даже при конкурентных запросах.

## 3. Embedded vs References

### Связь `users (PostgreSQL)` - `planned_incomes / planned_expenses (MongoDB)`

**Выбран подход: references** через поле `user_id`.

Обоснование:

- сущности живут в разных СУБД - embedded физически невозможен;
- у одного пользователя могут быть тысячи записей бюджета, а embedding раздул бы документ users в Postgres;
- доходы/расходы читаются и обновляются независимо от профиля пользователя;

### Embedded-поля внутри документов бюджета

В схемах коллекций предусмотрены вложенные поля:

- `tags: string[]` — массив меток для произвольной классификации (`food`, `recurring`, `vacation`). Удобно для `$in`, `$push`, `$addToSet`, `$pull`.
- `meta: object` — произвольный объект с расширяемыми атрибутами (`recurring`, `source`, `currency_rate`).

Обоснование выбора embedded:

- читаются и обновляются атомарно вместе с родительским документом;
- размер ограничен и предсказуем;
- исключают необходимость дополнительных `$lookup`-запросов;
- схема расширяема без миграций.

## 4. Используемые типы данных MongoDB

| Тип | Где применяется |
|-----|----------------|
| `long` (Int64) | `id`, `user_id` |
| `string` | `category`, `date` (ISO 8601), `description` |
| `double` | `amount` — денежные суммы |
| `object` | `meta` — вложенные документы |
| `array` | `tags` — массивы строк |
| `bool` | `meta.recurring` |
| `date` | `created_at` — временные метки |

## 5. Индексы

### MongoDB

- `planned_incomes.user_id + planned_incomes.date` (compound) — выборка доходов пользователя и аналитика за период;
- `planned_expenses.user_id + planned_expenses.date` (compound) — выборка расходов пользователя и аналитика за период.

### PostgreSQL (для коллекции `users`)

- `users_pkey` (PK) — поиск по id;
- `users_login_key` (UNIQUE) — поиск/проверка по логину;
- `idx_users_first_name_trgm`, `idx_users_last_name_trgm` (GIN + `pg_trgm`) — поиск по маске имени/фамилии через ILIKE.

## 6. Валидация (`$jsonSchema`)

Для коллекций `planned_incomes` и `planned_expenses` подключена строгая валидация (`validationLevel: strict`, `validationAction: error`):

- обязательные поля (`required`): `id`, `user_id`, `category`, `amount`, `date`;
- типы: `bsonType` для каждого поля;
- ограничения: `minimum: 0.01` для `amount`, `pattern: ^\d{4}-\d{2}-\d{2}$` для `date`, `minLength`/`maxLength` для `category`.

В файле [`validation.js`](db/mongo/validation.js) также есть попытка вставки невалидного документа, и она ожидаемо отклоняется MongoDB.
