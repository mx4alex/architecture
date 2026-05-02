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

-- GIN-индексы для ILIKE поиска по имени/фамилии
CREATE INDEX idx_users_first_name_trgm ON users USING GIN (first_name gin_trgm_ops);
CREATE INDEX idx_users_last_name_trgm  ON users USING GIN (last_name gin_trgm_ops);
