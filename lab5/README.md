# Budget Service - кеширование и rate limiting

Вариант №12 - REST API для личного бюджета. Добавлены **Redis-кеш** и **rate limiting**.

## Файлы результата

- [`performance_design.md`](performance_design.md) - анализ hot-paths,
  выбор стратегий кеширования и алгоритмов rate limiting, метрики и SLO.
- [`src/cache/`](src/cache) - компонент `CacheService` (Cache-Aside поверх
  `userver::storages::redis`).
- [`src/ratelimit/`](src/ratelimit) - компонент `RateLimiter` (Fixed window
  counter и Sliding window log).

## Архитектура

| Слой | Технология | Назначение |
|------|------------|------------|
| HTTP-фреймворк | [userver](https://userver.tech/) (C++17) | асинхронный server, JWT-bearer auth |
| Хранилище пользователей | PostgreSQL 16 + `pgcrypto` + GIN trgm | unique login, поиск ILIKE |
| Хранилище доходов/расходов | MongoDB 8.0 | гибкая схема, агрегация по периодам |
| Кеш | Redis 7 | Cache-Aside для list/aggregation эндпоинтов |
| Rate limiting | Redis 7 | Fixed window + Sliding window log |
| Метрики | userver statistics | hit-rate, ratelimit allowed/rejected |


## Endpoints и применённые оптимизации

| Endpoint | Кеш | Rate limit |
|----------|-----|------------|
| `POST /api/v1/auth/register` | - | Fixed window 5/мин на IP |
| `POST /api/v1/auth/login`    | - | Sliding window log 10/мин на IP |
| `GET  /api/v1/users/search?mask=…` | Cache-Aside, TTL 120 с | защита глобальным throttling |
| `GET  /api/v1/users/search?login=…` | точечный по uniq-индексу, дешевле без кеша | - |
| `GET  /api/v1/incomes`       | Cache-Aside, TTL 60 с | - |
| `GET  /api/v1/expenses`      | Cache-Aside, TTL 60 с | - |
| `GET  /api/v1/budget/dynamics` | Cache-Aside, TTL 300 | Fixed window 60/мин на user |
| `POST/PUT/DELETE` доходов/расходов | инвалидация (`INCR cv:budget:{uid}`) | - |

В ответе на закешированный GET добавляется заголовок `X-Cache: HIT|MISS`.
В ответе на rate-limited endpoint всегда есть:

```
X-RateLimit-Limit:     10
X-RateLimit-Remaining: 7
X-RateLimit-Reset:     42
Retry-After:           42
```

## Быстрый запуск

```bash
cd lab5
docker compose up --build
```

## Лимиты и тюнинг

Лимиты rate limiter и базовые TTL настраиваются в
[`configs/static_config.yaml`](configs/static_config.yaml):

```yaml
cache-service:
    default_ttl_ms: 60000

rate-limiter:
    login_limit: 10
    login_window_seconds: 60
    register_limit: 5
    register_window_seconds: 60
    dynamics_limit: 60
    dynamics_window_seconds: 60
```

## Метрики и наблюдаемость

`CacheService` и `RateLimiter` поддерживают атомарные (`std::atomic`)
счётчики, доступные через метод `GetStats()`:

- `cache.{hits,misses,errors,writes}`
- `ratelimit.{allowed,rejected,errors}`

В перспективе их несложно будет опубликовать в формате Prometheus.
