CREATE EXTENSION IF NOT EXISTS pgcrypto;
CREATE EXTENSION IF NOT EXISTS pg_trgm;

-- Пользователи
CREATE TABLE users (
    id         BIGSERIAL       PRIMARY KEY,
    login      VARCHAR(255)    NOT NULL UNIQUE,
    password   VARCHAR(255)    NOT NULL,
    first_name VARCHAR(255)    NOT NULL,
    last_name  VARCHAR(255)    NOT NULL,
    created_at TIMESTAMPTZ     NOT NULL DEFAULT NOW()
);

-- Планируемые доходы
CREATE TABLE planned_incomes (
    id          BIGSERIAL       PRIMARY KEY,
    user_id     BIGINT          NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    category    VARCHAR(255)    NOT NULL,
    amount      NUMERIC(15, 2)  NOT NULL CHECK (amount > 0),
    date        DATE            NOT NULL,
    description TEXT            NOT NULL DEFAULT '',
    created_at  TIMESTAMPTZ     NOT NULL DEFAULT NOW()
);

-- Планируемые расходы
CREATE TABLE planned_expenses (
    id          BIGSERIAL       PRIMARY KEY,
    user_id     BIGINT          NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    category    VARCHAR(255)    NOT NULL,
    amount      NUMERIC(15, 2)  NOT NULL CHECK (amount > 0),
    date        DATE            NOT NULL,
    description TEXT            NOT NULL DEFAULT '',
    created_at  TIMESTAMPTZ     NOT NULL DEFAULT NOW()
);

-- GIN-индексы для ILIKE поиска по имени/фамилии
CREATE INDEX idx_users_first_name_trgm ON users USING GIN (first_name gin_trgm_ops);
CREATE INDEX idx_users_last_name_trgm  ON users USING GIN (last_name gin_trgm_ops);

-- Индексы FK для выборки записей пользователя
CREATE INDEX idx_planned_incomes_user_id  ON planned_incomes  (user_id);
CREATE INDEX idx_planned_expenses_user_id ON planned_expenses (user_id);

-- Составные индексы для запроса динамики бюджета за период
CREATE INDEX idx_planned_incomes_user_date  ON planned_incomes  (user_id, date);
CREATE INDEX idx_planned_expenses_user_date ON planned_expenses (user_id, date);
