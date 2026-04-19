const budgetDb = db.getSiblingDB('budget_db');

// Добавление доходов
budgetDb.planned_incomes.insertOne({
  id: NumberLong(2001),
  user_id: NumberLong(1),
  category: 'Зарплата',
  amount: 99000,
  date: '2026-02-15',
  description: 'Зарплата за февраль',
  tags: ['main'],
  meta: { recurring: true },
  created_at: new Date(),
});

// Добавление расходов
budgetDb.planned_expenses.insertOne({
  id: NumberLong(3001),
  user_id: NumberLong(1),
  category: 'Продукты',
  amount: 18000,
  date: '2026-02-10',
  description: 'Еженедельная закупка',
  tags: ['food'],
  meta: { recurring: false },
  created_at: new Date(),
});

// Доходы пользователя с фильтрами 
budgetDb.planned_incomes.find({
  $and: [
    { user_id: { $eq: NumberLong(1) } },
    { amount: { $gt: 10000, $lt: 200000 } },
    { category: { $in: ['Зарплата', 'Премия', 'Фриланс'] } },
  ],
});

// Расходы пользователя за период
budgetDb.planned_expenses.find(
  {
    user_id: NumberLong(1),
    date: { $gte: '2026-01-01', $lte: '2026-01-31' },
  },
  { _id: 0, id: 1, category: 1, amount: 1, date: 1 },
).sort({ date: 1 });

// Доходы вне категории «Зарплата»
budgetDb.planned_incomes.find({
  $or: [
    { category: { $ne: 'Зарплата' } },
    { amount: { $gt: 100000 } },
  ],
});

// Обновление суммы и описания дохода
budgetDb.planned_incomes.updateOne(
  { id: NumberLong(1), user_id: NumberLong(1) },
  { $set: { amount: 155000, description: 'Зарплата с повышением' } },
);

// Операции с массивами тегов
budgetDb.planned_expenses.updateOne(
  { id: NumberLong(2), user_id: NumberLong(1) },
  { $push: { tags: 'weekly' } },
);
budgetDb.planned_expenses.updateOne(
  { id: NumberLong(2), user_id: NumberLong(1) },
  { $addToSet: { tags: 'food' } },
);
budgetDb.planned_expenses.updateOne(
  { id: NumberLong(2), user_id: NumberLong(1) },
  { $pull: { tags: 'weekly' } },
);

// Массовое обновление meta для всех доходов категории «Зарплата»
budgetDb.planned_incomes.updateMany(
  { category: 'Зарплата' },
  { $set: { 'meta.recurring': true } },
);

// Удаление одного дохода
budgetDb.planned_incomes.deleteOne({
  id: NumberLong(2001),
  user_id: NumberLong(1),
});

// Удаление одного расхода
budgetDb.planned_expenses.deleteOne({
  id: NumberLong(3001),
  user_id: NumberLong(1),
});

// Каскадное удаление всех записей пользователя
budgetDb.planned_incomes.deleteMany({ user_id: NumberLong(99) });
budgetDb.planned_expenses.deleteMany({ user_id: NumberLong(99) });

// Агрегация 1: топ-категорий расходов за период
budgetDb.planned_expenses.aggregate([
  { $match: { user_id: NumberLong(1), date: { $gte: '2026-01-01', $lte: '2026-02-28' } } },
  {
    $group: {
      _id: '$category',
      total: { $sum: '$amount' },
      count: { $sum: 1 },
      avg_amount: { $avg: '$amount' },
    },
  },
  { $project: { _id: 0, category: '$_id', total: 1, count: 1, avg_amount: 1 } },
  { $sort: { total: -1 } },
  { $limit: 5 },
]);

// Агрегация 2: динамика бюджета
budgetDb.planned_incomes.aggregate([
  { $match: { user_id: NumberLong(1), date: { $gte: '2026-01-01', $lte: '2026-02-28' } } },
  { $project: { date: 1, amount: 1, type: { $literal: 'income' } } },
  {
    $unionWith: {
      coll: 'planned_expenses',
      pipeline: [
        { $match: { user_id: NumberLong(1), date: { $gte: '2026-01-01', $lte: '2026-02-28' } } },
        { $project: { date: 1, amount: 1, type: { $literal: 'expense' } } },
      ],
    },
  },
  {
    $group: {
      _id: '$date',
      total_income: { $sum: { $cond: [{ $eq: ['$type', 'income'] }, '$amount', 0] } },
      total_expense: { $sum: { $cond: [{ $eq: ['$type', 'expense'] }, '$amount', 0] } },
    },
  },
  { $addFields: { balance: { $subtract: ['$total_income', '$total_expense'] } } },
  { $project: { _id: 0, date: '$_id', total_income: 1, total_expense: 1, balance: 1 } },
  { $sort: { date: 1 } },
]);
