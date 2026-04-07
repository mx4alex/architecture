-- Регистрация
INSERT INTO users (login, password, first_name, last_name)
VALUES ($1, crypt($2, gen_salt('bf')), $3, $4)
ON CONFLICT (login) DO NOTHING
RETURNING id, login, first_name, last_name;

-- Поиск по логину
SELECT id, login, first_name, last_name
FROM users
WHERE login = $1;

-- Поиск по маске имени
SELECT id, login, first_name, last_name
FROM users
WHERE first_name ILIKE '%' || $1 || '%'
   OR last_name  ILIKE '%' || $1 || '%';

-- Аутентификация
SELECT id, login, first_name, last_name
FROM users
WHERE login = $1 AND password = crypt($2, password);

-- Создание дохода
INSERT INTO planned_incomes (user_id, category, amount, date, description)
VALUES ($1, $2, $3, $4, $5)
RETURNING id, user_id, category, amount, date, description;

-- Список доходов
SELECT id, user_id, category, amount, date, description
FROM planned_incomes
WHERE user_id = $1
ORDER BY date DESC;

-- Обновление дохода
UPDATE planned_incomes
SET category = $3, amount = $4, date = $5, description = $6
WHERE id = $1 AND user_id = $2
RETURNING id, user_id, category, amount, date, description;

-- Удаление дохода
DELETE FROM planned_incomes
WHERE id = $1 AND user_id = $2;

-- Создание расхода
INSERT INTO planned_expenses (user_id, category, amount, date, description)
VALUES ($1, $2, $3, $4, $5)
RETURNING id, user_id, category, amount, date, description;

-- Список расходов
SELECT id, user_id, category, amount, date, description
FROM planned_expenses
WHERE user_id = $1
ORDER BY date DESC;

-- Обновление расхода
UPDATE planned_expenses
SET category = $3, amount = $4, date = $5, description = $6
WHERE id = $1 AND user_id = $2
RETURNING id, user_id, category, amount, date, description;

-- Удаление расхода
DELETE FROM planned_expenses
WHERE id = $1 AND user_id = $2;

-- Динамика бюджета

-- Итого доходов
SELECT COALESCE(SUM(amount), 0) AS total_income
FROM planned_incomes
WHERE user_id = $1 AND date BETWEEN $2 AND $3;

-- Итого расходов
SELECT COALESCE(SUM(amount), 0) AS total_expense
FROM planned_expenses
WHERE user_id = $1 AND date BETWEEN $2 AND $3;

-- Ежедневная разбивка 
SELECT
    COALESCE(i.date, e.date) AS date,
    COALESCE(i.daily_income, 0) AS income,
    COALESCE(e.daily_expense, 0) AS expense,
    COALESCE(i.daily_income, 0) - COALESCE(e.daily_expense, 0) AS balance
FROM (
    SELECT date, SUM(amount) AS daily_income
    FROM planned_incomes
    WHERE user_id = $1 AND date BETWEEN $2 AND $3
    GROUP BY date
) i
FULL OUTER JOIN (
    SELECT date, SUM(amount) AS daily_expense
    FROM planned_expenses
    WHERE user_id = $1 AND date BETWEEN $2 AND $3
    GROUP BY date
) e ON i.date = e.date
ORDER BY date;
