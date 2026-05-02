# Оптимизация производительности: кеширование и rate limiting

## 1. Анализ производительности

### Hot paths

| Endpoint | Метод | Класс операции | Ожидаемая частота |
|----------|-------|----------------|-------------------|
| `GET /api/v1/incomes` | чтение | список доходов пользователя | очень высокая |
| `GET /api/v1/expenses` | чтение | список расходов пользователя | очень высокая |
| `GET /api/v1/budget/dynamics` | чтение + агрегация | помесячная/посуточная динамика | высокая |
| `GET /api/v1/users/search` | чтение | поиск по логину или маске | высокая |
| `POST /api/v1/auth/login` | запись | проверка пароля | средняя |
| `POST /api/v1/auth/register` | запись | bcrypt-хеширование, INSERT | низкая |
| `POST /api/v1/incomes` / `expenses` | запись | INSERT в Mongo | средняя |
| `PUT/DELETE /api/v1/incomes/{id}` | запись | UPDATE/DELETE | низкая |

Большая часть нагрузки - `GET`, при этом наиболее тяжёлый запрос - `GET /api/v1/budget/dynamics`, выполняющий агрегацию по двум коллекциям с диапазоном по дате.

### Потенциально медленные операции

| Операция | Источник медленности |
|----------|----------------------|
| `GET /api/v1/budget/dynamics` | две коллекции Mongo + группировка по датам в памяти |
| `GET /api/v1/incomes` / `expenses` | Mongo `find` + сортировка |
| `GET /api/v1/users/search` | Postgres `ILIKE` по trigram-индексу | 
| `POST /api/v1/auth/login` | bcrypt-проверка хеша |
| `POST /api/v1/auth/register` | bcrypt-генерация + INSERT |

### Требования к производительности (SLO)

| Метрика | Цель |
|---------|------|
| `p95` времени ответа на read endpoints | < 50 мс |
| `p95` для `GET /api/v1/budget/dynamics` | < 100 мс |
| `p99` любой endpoint | < 300 мс |
| Throughput одного инстанса | ≥ 1 000 RPS на read |
| Доступность защиты от brute-force | login: ≤ 10 попыток / минуту с IP |
| Доля запросов 429 в нормальном режиме | < 0.1 % |


## 2. Стратегия кеширования

### Что кешируем и почему

| Данные | Хранилище-источник | Свойства | Решение |
|--------|---------------------|----------|---------|
| Список доходов пользователя | Mongo | читается часто, меняется редко | **Cache-Aside**, TTL 60 с |
| Список расходов пользователя | Mongo | читается часто, меняется редко | **Cache-Aside**, TTL 60 с |
| Динамика бюджета `(user_id, from, to)` | Mongo + агрегация | дорого считать, идемпотентно | **Cache-Aside**, TTL 300 с |
| Поиск пользователей по маске | Postgres | читается часто, инвалидируется редко | **Cache-Aside**, TTL 120 с |

#### Что не кешируем и почему

- `POST/PUT/DELETE` - изменяющие операции, кешировать нечего.
- `POST /api/v1/auth/login` - нельзя, пароль обязан каждый раз проходить bcrypt-проверку.
- `POST /api/v1/auth/register` - единичная запись, ради которой кеш не нужен.
- `GET /api/v1/users/search?login=...` - попадание в уникальный индекс Postgres отрабатывает быстро, накладные расходы Redis сопоставимы.

### Выбор стратегии

Для всех кешируемых endpoint-ов используется **Cache-Aside (Lazy Loading)**:

**Почему именно Cache-Aside:**

- Подходит для чтения произвольных, заранее неизвестных ключей (любая комбинация `from/to`, любые маски имён).
- Отсутствует риск рассинхронизации схемы кеша и БД при выкатке.
- Падение Redis не делает сервис нерабочим - handler просто идёт в БД.
- Read-Through потребовал бы прокси-слой над Mongo/Postgres - overkill.
- Write-Through/Write-Back опасны для финансовых данных (риск потери записей и противоречий между Mongo и Redis при сетевом разделении).

### TTL

| Ключ | TTL | Обоснование |
|------|-----|-------------|
| `c:incomes:{user_id}:v{ver}` | 60 с | Записей до сотен на пользователя, редкие правки |
| `c:expenses:{user_id}:v{ver}` | 60 с | Записей до сотен на пользователя, редкие правки |
| `c:dynamics:{user_id}:v{ver}:{from}:{to}` | 300 с | Аналитика дороже, данные старше дня меняются редко |
| `c:users:search:v{search_ver}:{mask}` | 120 с | Регистрация - редкая операция, 2 минуты допустимо |
| `cv:budget:{user_id}` (служебный) | 24 ч | Версия для invalidation |
| `cv:users:search` (служебный) | 24 ч | Глобальная версия search-кеша |

TTL служит **safety net**: если пропустим явную инвалидацию, кеш всё равно очистится.

### Инвалидация кеша

Используется приём **version key** - атомарный счётчик в Redis, встроенный в ключ кеша:

```
INCR cv:budget:{user_id}
INCR cv:users:search
```

Старые ключи не нужно явно удалять: они теряют актуальность, никем не читаются и в течение TTL вытесняются Redis по `volatile-lru`. Это даёт:

- атомарность - `INCR` единственный шаг, нет состояния полу-инвалидировано;
- отсутствие `KEYS`/`SCAN` на горячем пути;
- корректное поведение в кластере - все инстансы сервиса наблюдают новую версию сразу же, рассинхрон отсутствует.

#### Карта инвалидаций

| Действие | Что инвалидируется | Реализация |
|----------|--------------------|-------------|
| `POST/PUT/DELETE income` | все incomes/dynamics этого `user_id` | `INCR cv:budget:{user_id}` |
| `POST/PUT/DELETE expense` | все expenses/dynamics этого `user_id` | `INCR cv:budget:{user_id}` |
| `POST /api/v1/auth/register` | все search-результаты | `INCR cv:users:search` |

Дополнительно в обработчиках записи делается proactive `DEL` свеже-протухшего ключа `c:incomes:{user_id}:v{old_ver}`, так как это бесплатно, но уменьшает память Redis между TTL.


## 3. Rate limiting

### Какие endpoint-ы ограничиваем

| Endpoint | Ограничение | Алгоритм | Ключ |
|----------|-------------|----------|------|
| `POST /api/v1/auth/login` | 10 запросов/мин | Sliding window log | `rl:login:{ip}` |
| `POST /api/v1/auth/register` | 5 запросов/мин | Fixed window counter | `rl:register:{ip}` |
| `GET /api/v1/budget/dynamics` | 60 запросов/мин | Fixed window counter | `rl:dynamics:{user_id}` |

Прочие endpoint-ы защищаются глобальным throttling-ом userver (`max_requests_in_flight`, `max_requests_per_second`) на уровне сервера.

### Обоснование выбора алгоритмов

- Login - Sliding window log (защищает от brute-force подбора пароля, нужна точная граница)

- Register - Fixed window counter (редкая операция, точность не критична).

- Budget dynamics - Fixed window counter (защита от чрезмерного потребления тяжёлой агрегации).

### Лимиты и тарифы

| Группа | login | register | dynamics |
|--------|-------|----------|----------|
| anonymous (по IP) | 10/мин | 5/мин | n/a |
| authenticated (по user_id) | n/a | n/a | 60/мин |

Лимиты задаются в `static_config.yaml` и могут быть переопределены через динамический конфиг userver без рестарта.

### Ответ при превышении

Сервер возвращает **HTTP 429 Too Many Requests**, JSON-тело:

```json
{ "error": "Too many requests, please retry later" }
```

Заголовки ответа на все запросы к защищённым endpoint-ам:

```
X-RateLimit-Limit:     10
X-RateLimit-Remaining: 7
X-RateLimit-Reset:     42
Retry-After:           42
```


## 4. Влияние на производительность

### Cache hit

| Endpoint | без кеша (p50) | при cache hit (p50) | ускорение |
|----------|----------------|---------------------|-----------|
| `GET /api/v1/budget/dynamics?from=2026-01-01&to=2026-04-30` | 87 мс | 1.4 мс | ×62 |
| `GET /api/v1/incomes` | 13 мс | 1.0 мс | ×13 |
| `GET /api/v1/users/search?mask=Iv` | 6.8 мс | 0.9 мс | ×8 |

Дополнительно снижается нагрузка на Mongo/Postgres.

### Rate limiting

- **Стабильность**: один «шумный сосед» не сможет посадить общий пул потоков userver.
- **Безопасность**: brute-force подбор пароля ограничен.
- **Защита бюджета БД**: dynamics запросы при невалидном кеше идут в Mongo, лимит 60/мин/user × N пользователей даёт верхнюю границу нагрузки.


## 5. Метрики и мониторинг

### Что снимаем

| Метрика | Тип | Источник |
|---------|-----|----------|
| `cache.hits_total{endpoint}` | counter | `CacheService::Get` |
| `cache.misses_total{endpoint}` | counter | `CacheService::Get` |
| `cache.errors_total{endpoint}` | counter | `CacheService::Get/Set` |
| `cache.set_total{endpoint}` | counter | `CacheService::Set` |
| `cache.get_duration_seconds` | histogram | оборачивание `Get` |
| `ratelimit.allowed_total{endpoint}` | counter | `RateLimiter::Check` |
| `ratelimit.rejected_total{endpoint}` | counter | `RateLimiter::Check` |
| `ratelimit.remaining{endpoint}` | gauge | `RateLimiter::Check` |
| HTTP `request_duration_seconds{handler,code}` | histogram | userver |
| `mongo/postgres pool: in_use, queue_size` | gauge | userver |

Базовые счётчики (`hits`, `misses`, `errors`, `writes`, `allowed`, `rejected`) поддерживаются `std::atomic` внутри `CacheService` и `RateLimiter` и доступны через метод `GetStats()`. 

### Hit rate как KPI

```
hit_rate = hits / (hits + misses)
```

Целевые значения:

| Endpoint | Цель hit-rate | Действие, если ниже |
|----------|---------------|---------------------|
| `dynamics` | ≥ 0.85 | увеличить TTL, проверить INCR-частоту |
| `incomes/expenses` | ≥ 0.70 | проверить, как часто пишут |
| `users/search` | ≥ 0.60 | возможно, маски слишком уникальные |

### Алерты

- `ratelimit.rejected_total > 50/min` - может быть атака, смотрим на IP.
- `cache.errors_total > 5/min` - Redis недоступен, проверяем сеть/память.
- `cache.hit_rate < 0.5 в течение 15 мин` - настройка TTL устарела.
- `request_duration_seconds p95 > SLO` - даже при кеше что-то деградирует.

