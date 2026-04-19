const budgetDb = db.getSiblingDB('budget_db');

budgetDb.planned_incomes.insertMany([
  { id: NumberLong(1), user_id: NumberLong(1), category: 'Зарплата', amount: 150000.0, date: '2026-01-15', description: 'Основная зарплата за январь' },
  { id: NumberLong(2), user_id: NumberLong(1), category: 'Премия', amount: 30000.0, date: '2026-01-31', description: 'Квартальная премия' },
  { id: NumberLong(3), user_id: NumberLong(1), category: 'Зарплата', amount: 150000.0, date: '2026-02-15', description: 'Основная зарплата за февраль' },
  { id: NumberLong(4), user_id: NumberLong(2), category: 'Зарплата', amount: 120000.0, date: '2026-01-15', description: 'Зарплата' },
  { id: NumberLong(5), user_id: NumberLong(2), category: 'Фриланс', amount: 25000.0, date: '2026-01-20', description: 'Дизайн лендинга' },
  { id: NumberLong(6), user_id: NumberLong(2), category: 'Зарплата', amount: 120000.0, date: '2026-02-15', description: 'Зарплата за февраль' },
  { id: NumberLong(7), user_id: NumberLong(3), category: 'Зарплата', amount: 200000.0, date: '2026-01-10', description: 'Зарплата разработчика' },
  { id: NumberLong(8), user_id: NumberLong(3), category: 'Инвестиции', amount: 15000.0, date: '2026-01-25', description: 'Дивиденды' },
  { id: NumberLong(9), user_id: NumberLong(4), category: 'Зарплата', amount: 95000.0, date: '2026-01-15', description: 'Зарплата бухгалтера' },
  { id: NumberLong(10), user_id: NumberLong(5), category: 'Зарплата', amount: 180000.0, date: '2026-01-15', description: 'Зарплата менеджера' },
  { id: NumberLong(11), user_id: NumberLong(5), category: 'Подработка', amount: 20000.0, date: '2026-02-01', description: 'Консультация' },
  { id: NumberLong(12), user_id: NumberLong(6), category: 'Зарплата', amount: 110000.0, date: '2026-01-15', description: 'Зарплата' },
  { id: NumberLong(13), user_id: NumberLong(7), category: 'Арендный доход', amount: 45000.0, date: '2026-01-01', description: 'Сдача квартиры' },
  { id: NumberLong(14), user_id: NumberLong(7), category: 'Зарплата', amount: 160000.0, date: '2026-01-15', description: 'Зарплата' },
  { id: NumberLong(15), user_id: NumberLong(8), category: 'Зарплата', amount: 130000.0, date: '2026-01-15', description: 'Зарплата' },
]);

budgetDb.planned_expenses.insertMany([
  { id: NumberLong(1), user_id: NumberLong(1), category: 'Аренда', amount: 45000.0, date: '2026-01-05', description: 'Аренда квартиры' },
  { id: NumberLong(2), user_id: NumberLong(1), category: 'Продукты', amount: 25000.0, date: '2026-01-10', description: 'Еженедельная закупка' },
  { id: NumberLong(3), user_id: NumberLong(1), category: 'Транспорт', amount: 5000.0, date: '2026-01-08', description: 'Проездной' },
  { id: NumberLong(4), user_id: NumberLong(1), category: 'Коммунальные', amount: 8000.0, date: '2026-01-20', description: 'ЖКХ за январь' },
  { id: NumberLong(5), user_id: NumberLong(1), category: 'Развлечения', amount: 10000.0, date: '2026-01-28', description: 'Кино и рестораны' },
  { id: NumberLong(6), user_id: NumberLong(2), category: 'Аренда', amount: 35000.0, date: '2026-01-05', description: 'Аренда' },
  { id: NumberLong(7), user_id: NumberLong(2), category: 'Продукты', amount: 20000.0, date: '2026-01-12', description: 'Продукты на месяц' },
  { id: NumberLong(8), user_id: NumberLong(2), category: 'Одежда', amount: 15000.0, date: '2026-01-18', description: 'Зимняя куртка' },
  { id: NumberLong(9), user_id: NumberLong(3), category: 'Ипотека', amount: 65000.0, date: '2026-01-10', description: 'Платёж по ипотеке' },
  { id: NumberLong(10), user_id: NumberLong(3), category: 'Продукты', amount: 30000.0, date: '2026-01-15', description: 'Продукты' },
  { id: NumberLong(11), user_id: NumberLong(4), category: 'Аренда', amount: 30000.0, date: '2026-01-05', description: 'Аренда' },
  { id: NumberLong(12), user_id: NumberLong(4), category: 'Здоровье', amount: 12000.0, date: '2026-01-22', description: 'Стоматолог' },
  { id: NumberLong(13), user_id: NumberLong(5), category: 'Образование', amount: 25000.0, date: '2026-01-10', description: 'Курсы повышения квалификации' },
  { id: NumberLong(14), user_id: NumberLong(6), category: 'Продукты', amount: 18000.0, date: '2026-01-10', description: 'Продукты' },
  { id: NumberLong(15), user_id: NumberLong(7), category: 'Транспорт', amount: 10000.0, date: '2026-01-05', description: 'Бензин и обслуживание авто' },
]);

budgetDb.counters.replaceOne(
  { _id: 'incomes_id_seq' },
  { _id: 'incomes_id_seq', value: NumberLong(15) },
  { upsert: true },
);
budgetDb.counters.replaceOne(
  { _id: 'expenses_id_seq' },
  { _id: 'expenses_id_seq', value: NumberLong(15) },
  { upsert: true },
);
