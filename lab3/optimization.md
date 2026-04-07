# Оптимизация запросов

Анализ и оптимизация SQL-запросов сервиса бюджетирования с использованием `EXPLAIN ANALYZE`.

Тестирование: PostgreSQL 16, **10 012 пользователей**, **100 015 доходов**, **100 015 расходов**.

---

## 1. Поиск пользователя по логину

```sql
SELECT id, login, first_name, last_name FROM users WHERE login = 'user_500';
```

Уникальный индекс `users_login_key` создаётся автоматически через `UNIQUE` constraint —
Seq Scan невозможен, PostgreSQL всегда использует Index Scan:

```
Index Scan using users_login_key on users  (cost=0.29..8.30 rows=1 width=44) (actual time=0.049..0.052 rows=1 loops=1)
  Index Cond: ((login)::text = 'user_500'::text)
Planning Time: 0.511 ms
Execution Time: 0.134 ms
```

---

## 2. Поиск по маске имени/фамилии

```sql
SELECT id, login, first_name, last_name
FROM users
WHERE first_name ILIKE '%Алексе%' OR last_name ILIKE '%Алексе%';
```

### До оптимизации

```
Seq Scan on users  (cost=0.00..332.18 rows=1289 width=44) (actual time=0.010..5.127 rows=1333 loops=1)
  Filter: (((first_name)::text ~~* '%Алексе%'::text) OR ((last_name)::text ~~* '%Алексе%'::text))
  Rows Removed by Filter: 8679
Planning Time: 0.064 ms
Execution Time: 5.166 ms
```

Seq Scan — полный перебор всех 10 012 строк, ILIKE проверяется для каждой.

### После оптимизации

```sql
CREATE INDEX idx_users_first_name_trgm ON users USING GIN (first_name gin_trgm_ops);
CREATE INDEX idx_users_last_name_trgm  ON users USING GIN (last_name gin_trgm_ops);
```

```
Bitmap Heap Scan on users  (cost=84.86..286.85 rows=1289 width=44) (actual time=0.220..1.185 rows=1333 loops=1)
  Recheck Cond: (((first_name)::text ~~* '%Алексе%'::text) OR ((last_name)::text ~~* '%Алексе%'::text))
  Heap Blocks: exact=182
  ->  BitmapOr  (cost=84.86..84.86 rows=1333 width=0) (actual time=0.181..0.182 rows=0 loops=1)
        ->  Bitmap Index Scan on idx_users_first_name_trgm  (cost=0.00..42.11 rows=667 width=0) (actual time=0.104..0.104 rows=667 loops=1)
              Index Cond: ((first_name)::text ~~* '%Алексе%'::text)
        ->  Bitmap Index Scan on idx_users_last_name_trgm  (cost=0.00..42.11 rows=666 width=0) (actual time=0.073..0.073 rows=666 loops=1)
              Index Cond: ((last_name)::text ~~* '%Алексе%'::text)
Planning Time: 0.720 ms
Execution Time: 1.324 ms
```

**Результат: 5.166 мс -> 1.324 мс (~4x).** BitmapOr объединяет результаты двух GIN-индексов, Recheck Cond проверяет только 182 блока вместо полного сканирования.

---

## 3. Получение доходов пользователя

```sql
SELECT id, user_id, category, amount, date, description
FROM planned_incomes WHERE user_id = 513 ORDER BY date DESC;
```

### До оптимизации

```
Sort  (cost=2549.35..2549.38 rows=10 width=62) (actual time=3.455..3.456 rows=10 loops=1)
  Sort Key: date DESC
  Sort Method: quicksort  Memory: 25kB
  ->  Seq Scan on planned_incomes  (cost=0.00..2549.19 rows=10 width=62) (actual time=0.061..3.412 rows=10 loops=1)
        Filter: (user_id = 513)
        Rows Removed by Filter: 100005
Planning Time: 0.203 ms
Execution Time: 3.475 ms
```

Seq Scan по 100 015 строк, из них 100 005 отброшены фильтром.

### После оптимизации

```sql
CREATE INDEX idx_planned_incomes_user_id ON planned_incomes (user_id);
```

```
Sort  (cost=42.03..42.05 rows=10 width=62) (actual time=0.122..0.123 rows=11 loops=1)
  Sort Key: date DESC
  Sort Method: quicksort  Memory: 25kB
  ->  Bitmap Heap Scan on planned_incomes  (cost=4.37..41.86 rows=10 width=62) (actual time=0.037..0.102 rows=11 loops=1)
        Recheck Cond: (user_id = 513)
        Heap Blocks: exact=11
        ->  Bitmap Index Scan on idx_planned_incomes_user_id  (cost=0.00..4.37 rows=10 width=0) (actual time=0.022..0.023 rows=11 loops=1)
              Index Cond: (user_id = 513)
Planning Time: 0.291 ms
Execution Time: 0.148 ms
```

**Результат: 3.475 мс -> 0.148 мс (~23x).** Bitmap Index Scan по индексу выбирает только 11 блоков.

---

## 4. Получение расходов пользователя

```sql
SELECT id, user_id, category, amount, date, description
FROM planned_expenses WHERE user_id = 513 ORDER BY date DESC;
```

### До оптимизации

```
Sort  (cost=2532.35..2532.38 rows=10 width=63) (actual time=3.330..3.331 rows=10 loops=1)
  Sort Key: date DESC
  Sort Method: quicksort  Memory: 25kB
  ->  Seq Scan on planned_expenses  (cost=0.00..2532.19 rows=10 width=63) (actual time=0.027..3.322 rows=10 loops=1)
        Filter: (user_id = 513)
        Rows Removed by Filter: 100005
Planning Time: 0.102 ms
Execution Time: 3.346 ms
```

### После оптимизации

```sql
CREATE INDEX idx_planned_expenses_user_id ON planned_expenses (user_id);
```

```
Sort  (cost=42.01..42.04 rows=10 width=63) (actual time=0.093..0.094 rows=10 loops=1)
  Sort Key: date DESC
  Sort Method: quicksort  Memory: 25kB
  ->  Bitmap Heap Scan on planned_expenses  (cost=4.37..41.85 rows=10 width=63) (actual time=0.043..0.085 rows=10 loops=1)
        Recheck Cond: (user_id = 513)
        Heap Blocks: exact=10
        ->  Bitmap Index Scan on idx_planned_expenses_user_id  (cost=0.00..4.37 rows=10 width=0) (actual time=0.035..0.035 rows=10 loops=1)
              Index Cond: (user_id = 513)
Planning Time: 0.271 ms
Execution Time: 0.114 ms
```

**Результат: 3.346 мс -> 0.114 мс (~29x).**

---

## 5. Динамика бюджета за период

```sql
SELECT COALESCE(SUM(amount), 0) AS total
FROM planned_incomes
WHERE user_id = 513 AND date BETWEEN '2024-01-01' AND '2024-03-31';
```

### До оптимизации (без составного индекса)

```
Aggregate  (cost=3049.27..3049.28 rows=1 width=32) (actual time=3.658..3.658 rows=1 loops=1)
  ->  Seq Scan on planned_incomes  (cost=0.00..3049.26 rows=2 width=6) (actual time=0.829..3.643 rows=2 loops=1)
        Filter: ((date >= '2024-01-01'::date) AND (date <= '2024-03-31'::date) AND (user_id = 513))
        Rows Removed by Filter: 100013
Planning Time: 0.148 ms
Execution Time: 3.677 ms
```

### После оптимизации

```sql
CREATE INDEX idx_planned_incomes_user_date ON planned_incomes (user_id, date);
```

```
Aggregate  (cost=16.09..16.10 rows=1 width=32) (actual time=0.035..0.035 rows=1 loops=1)
  ->  Bitmap Heap Scan on planned_incomes  (cost=4.46..16.08 rows=3 width=6) (actual time=0.028..0.030 rows=3 loops=1)
        Recheck Cond: ((user_id = 513) AND (date >= '2024-01-01'::date) AND (date <= '2024-03-31'::date))
        Heap Blocks: exact=3
        ->  Bitmap Index Scan on idx_planned_incomes_user_date  (cost=0.00..4.46 rows=3 width=0) (actual time=0.024..0.024 rows=3 loops=1)
              Index Cond: ((user_id = 513) AND (date >= '2024-01-01'::date) AND (date <= '2024-03-31'::date))
Planning Time: 0.191 ms
Execution Time: 0.065 ms
```

**Результат: 3.677 мс -> 0.065 мс (~57x).** Составной индекс `(user_id, date)` фильтрует и по пользователю, и по диапазону дат на уровне индекса.

### Ежедневная разбивка с агрегацией

```sql
SELECT date, SUM(amount) AS daily_income
FROM planned_incomes
WHERE user_id = 513 AND date BETWEEN '2024-01-01' AND '2024-03-31'
GROUP BY date ORDER BY date;
```

**До:** Seq Scan + Sort + GroupAggregate:

```
GroupAggregate  (cost=3049.27..3049.31 rows=2 width=36) (actual time=3.427..3.428 rows=2 loops=1)
  Group Key: date
  ->  Sort  (cost=3049.27..3049.28 rows=2 width=10) (actual time=3.423..3.423 rows=2 loops=1)
        Sort Key: date
        Sort Method: quicksort  Memory: 25kB
        ->  Seq Scan on planned_incomes  (cost=0.00..3049.26 rows=2 width=10) (actual time=0.767..3.413 rows=2 loops=1)
              Filter: ((date >= '2024-01-01'::date) AND (date <= '2024-03-31'::date) AND (user_id = 513))
              Rows Removed by Filter: 100013
Planning Time: 0.034 ms
Execution Time: 3.460 ms
```

**После:**

```
GroupAggregate  (cost=16.10..16.16 rows=3 width=36) (actual time=0.018..0.020 rows=3 loops=1)
  Group Key: date
  ->  Sort  (cost=16.10..16.11 rows=3 width=10) (actual time=0.015..0.015 rows=3 loops=1)
        Sort Key: date
        Sort Method: quicksort  Memory: 25kB
        ->  Bitmap Heap Scan on planned_incomes  (cost=4.46..16.08 rows=3 width=10) (actual time=0.009..0.010 rows=3 loops=1)
              Recheck Cond: ((user_id = 513) AND (date >= '2024-01-01'::date) AND (date <= '2024-03-31'::date))
              Heap Blocks: exact=3
              ->  Bitmap Index Scan on idx_planned_incomes_user_date  (cost=0.00..4.46 rows=3 width=0) (actual time=0.006..0.006 rows=3 loops=1)
                    Index Cond: ((user_id = 513) AND (date >= '2024-01-01'::date) AND (date <= '2024-03-31'::date))
Planning Time: 0.046 ms
Execution Time: 0.074 ms
```

**Результат: 3.460 мс -> 0.074 мс (~47x).**

---

## 6. UPDATE по первичному ключу

```sql
UPDATE planned_incomes
SET category = 'Тест', amount = 999.00, date = '2024-06-01', description = 'test'
WHERE id = 500 AND user_id = (SELECT user_id FROM planned_incomes WHERE id = 500);
```

```
Update on planned_incomes  (cost=8.60..16.62 rows=0 width=0) (actual time=0.210..0.210 rows=0 loops=1)
  InitPlan 1 (returns $0)
    ->  Index Scan using planned_incomes_pkey on planned_incomes planned_incomes_1  (cost=0.29..8.31 rows=1 width=8) (actual time=0.005..0.005 rows=1 loops=1)
          Index Cond: (id = 500)
  ->  Index Scan using planned_incomes_pkey on planned_incomes  (cost=0.29..8.31 rows=1 width=576) (actual time=0.033..0.034 rows=1 loops=1)
        Index Cond: (id = 500)
        Filter: (user_id = $0)
Planning Time: 0.069 ms
Execution Time: 0.300 ms
```

Index Scan по PK — O(log n), дополнительная проверка user_id как Filter.

---

## 7. INSERT

```sql
INSERT INTO planned_incomes (user_id, category, amount, date, description)
VALUES (513, 'Премия', 50000.00, '2024-02-28', 'Годовая')
RETURNING id, user_id, category, amount, date, description;
```

```
Insert on planned_incomes  (cost=0.00..0.01 rows=1 width=594) (actual time=0.067..0.067 rows=1 loops=1)
  ->  Result  (cost=0.00..0.01 rows=1 width=594) (actual time=0.053..0.053 rows=1 loops=1)
Planning Time: 0.028 ms
Trigger for constraint planned_incomes_user_id_fkey: time=0.160 calls=1
Execution Time: 0.238 ms
```

---

## Сводная таблица

| Запрос | Без индексов | С индексами | Ускорение | Индекс |
|--------|-------------|-------------|-----------|--------|
| Поиск по имени ILIKE | 5.166 мс (Seq Scan) | 1.324 мс (Bitmap) | ~4x | GIN pg_trgm |
| Доходы пользователя | 3.475 мс (Seq Scan) | 0.148 мс (Bitmap) | ~23x | B-tree (user_id) |
| Расходы пользователя | 3.346 мс (Seq Scan) | 0.114 мс (Bitmap) | ~29x | B-tree (user_id) |
| Сумма доходов за период | 3.677 мс (Seq Scan) | 0.065 мс (Bitmap) | ~57x | B-tree (user_id, date) |
| Разбивка по дням | 3.460 мс (Seq Scan) | 0.074 мс (Bitmap) | ~47x | B-tree (user_id, date) |

---

## Партиционирование

Таблица `planned_incomes_part` — RANGE-партиционирование по `date` (поквартально).

### DDL

```sql
CREATE TABLE planned_incomes_part (
    id          BIGSERIAL,
    user_id     BIGINT          NOT NULL,
    category    VARCHAR(255)    NOT NULL,
    amount      NUMERIC(15, 2) NOT NULL CHECK (amount > 0),
    date        DATE            NOT NULL,
    description TEXT            NOT NULL DEFAULT '',
    created_at  TIMESTAMPTZ     NOT NULL DEFAULT NOW(),
    PRIMARY KEY (id, date)
) PARTITION BY RANGE (date);

CREATE TABLE planned_incomes_part_2024_q1 PARTITION OF planned_incomes_part
    FOR VALUES FROM ('2024-01-01') TO ('2024-04-01');
CREATE TABLE planned_incomes_part_2024_q2 PARTITION OF planned_incomes_part
    FOR VALUES FROM ('2024-04-01') TO ('2024-07-01');
CREATE TABLE planned_incomes_part_2024_q3 PARTITION OF planned_incomes_part
    FOR VALUES FROM ('2024-07-01') TO ('2024-10-01');
CREATE TABLE planned_incomes_part_2024_q4 PARTITION OF planned_incomes_part
    FOR VALUES FROM ('2024-10-01') TO ('2025-01-01');
CREATE TABLE planned_incomes_part_default PARTITION OF planned_incomes_part DEFAULT;

CREATE INDEX idx_part_incomes_user_date ON planned_incomes_part (user_id, date);
```

### EXPLAIN с Partition Pruning (Q1 2024)

```sql
SELECT date, SUM(amount) FROM planned_incomes_part
WHERE user_id = 513 AND date BETWEEN '2024-01-01' AND '2024-03-31'
GROUP BY date ORDER BY date;
```

```
GroupAggregate  (cost=11.89..11.93 rows=2 width=36) (actual time=0.033..0.034 rows=3 loops=1)
  Group Key: planned_incomes_part.date
  ->  Sort  (cost=11.89..11.89 rows=2 width=10) (actual time=0.029..0.029 rows=3 loops=1)
        Sort Key: planned_incomes_part.date
        Sort Method: quicksort  Memory: 25kB
        ->  Bitmap Heap Scan on planned_incomes_part_2024_q1 planned_incomes_part  (cost=4.31..11.88 rows=2 width=10) (actual time=0.021..0.023 rows=3 loops=1)
              Recheck Cond: ((user_id = 513) AND (date >= '2024-01-01'::date) AND (date <= '2024-03-31'::date))
              Heap Blocks: exact=3
              ->  Bitmap Index Scan on planned_incomes_part_2024_q1_user_id_date_idx  (cost=0.00..4.31 rows=2 width=0) (actual time=0.018..0.018 rows=3 loops=1)
                    Index Cond: ((user_id = 513) AND (date >= '2024-01-01'::date) AND (date <= '2024-03-31'::date))
Planning Time: 0.309 ms
Execution Time: 0.074 ms
```

### EXPLAIN с Partition Pruning (Q3 2024)

```sql
SELECT date, SUM(amount) FROM planned_incomes_part
WHERE user_id = 513 AND date BETWEEN '2024-07-01' AND '2024-09-30'
GROUP BY date ORDER BY date;
```

```
GroupAggregate  (cost=15.54..15.60 rows=3 width=36) (actual time=0.014..0.015 rows=2 loops=1)
  Group Key: planned_incomes_part.date
  ->  Sort  (cost=15.54..15.55 rows=3 width=10) (actual time=0.013..0.013 rows=2 loops=1)
        Sort Key: planned_incomes_part.date
        Sort Method: quicksort  Memory: 25kB
        ->  Bitmap Heap Scan on planned_incomes_part_2024_q3 planned_incomes_part  (cost=4.33..15.52 rows=3 width=10) (actual time=0.010..0.011 rows=2 loops=1)
              Recheck Cond: ((user_id = 513) AND (date >= '2024-07-01'::date) AND (date <= '2024-09-30'::date))
              Heap Blocks: exact=2
              ->  Bitmap Index Scan on planned_incomes_part_2024_q3_user_id_date_idx  (cost=0.00..4.32 rows=3 width=0) (actual time=0.009..0.009 rows=2 loops=1)
                    Index Cond: ((user_id = 513) AND (date >= '2024-07-01'::date) AND (date <= '2024-09-30'::date))
Planning Time: 0.085 ms
Execution Time: 0.029 ms
```

PostgreSQL автоматически выполняет Partition Pruning: видно, что сканируется только
`planned_incomes_part_2024_q1` или `planned_incomes_part_2024_q3`, а остальные партиции
полностью исключены из плана.

### Преимущества

- **Partition Pruning** — запрос за конкретный квартал обращается только к одной партиции.
- **Архивация** — старые партиции можно отсоединить (`DETACH PARTITION`) и переместить в архив.
- **Обслуживание** — `VACUUM` / `ANALYZE` работают на мелких партициях, не блокируя остальные.

### Ограничения

- PK включает `date` -> семантика идентификации `(id, date)` вместо `(id)`.
- UPDATE/DELETE по `id` требуют знания `date` для выбора нужной партиции.
