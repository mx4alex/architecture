const budgetDb = db.getSiblingDB('budget_db');

budgetDb.createCollection('planned_incomes', {
  validator: {
    $jsonSchema: {
      bsonType: 'object',
      required: ['id', 'user_id', 'category', 'amount', 'date'],
      properties: {
        id: { bsonType: 'long', minimum: NumberLong(1) },
        user_id: { bsonType: 'long', minimum: NumberLong(1) },
        category: { bsonType: 'string', minLength: 2, maxLength: 64 },
        amount: {
          bsonType: ['double', 'int', 'long', 'decimal'],
          minimum: 0.01,
        },
        date: {
          bsonType: 'string',
          pattern: '^\\d{4}-\\d{2}-\\d{2}$',
        },
        description: { bsonType: 'string' },
        tags: {
          bsonType: 'array',
          items: { bsonType: 'string' },
        },
        meta: { bsonType: 'object' },
        created_at: { bsonType: 'date' },
      },
    },
  },
  validationLevel: 'strict',
  validationAction: 'error',
});

budgetDb.createCollection('planned_expenses', {
  validator: {
    $jsonSchema: {
      bsonType: 'object',
      required: ['id', 'user_id', 'category', 'amount', 'date'],
      properties: {
        id: { bsonType: 'long', minimum: NumberLong(1) },
        user_id: { bsonType: 'long', minimum: NumberLong(1) },
        category: { bsonType: 'string', minLength: 2, maxLength: 64 },
        amount: {
          bsonType: ['double', 'int', 'long', 'decimal'],
          minimum: 0.01,
        },
        date: {
          bsonType: 'string',
          pattern: '^\\d{4}-\\d{2}-\\d{2}$',
        },
        description: { bsonType: 'string' },
        tags: {
          bsonType: 'array',
          items: { bsonType: 'string' },
        },
        meta: { bsonType: 'object' },
        created_at: { bsonType: 'date' },
      },
    },
  },
  validationLevel: 'strict',
  validationAction: 'error',
});

budgetDb.createCollection('counters');

budgetDb.planned_incomes.createIndex({ user_id: 1, date: 1 }, { name: 'idx_income_user_date' });
budgetDb.planned_expenses.createIndex({ user_id: 1, date: 1 }, { name: 'idx_expense_user_date' });

print('Validating schema rules with invalid insert attempt...');
try {
  budgetDb.planned_incomes.insertOne({
    id: NumberLong(99999),
    user_id: NumberLong(1),
    category: 'X',
    amount: -100,
    date: 'not-a-date',
  });
  print('Validation test failed: invalid document unexpectedly inserted');
} catch (error) {
  print(`Validation test passed: ${error.codeName || error.message}`);
}
