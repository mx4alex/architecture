#include "budget_storage.hpp"

#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/io/date.hpp>
#include <userver/storages/postgres/io/decimal64.hpp>

namespace budget {

namespace sql {

constexpr auto kInsertIncome =
    "INSERT INTO planned_incomes (user_id, category, amount, date, description) "
    "VALUES ($1, $2, $3, $4, $5) "
    "RETURNING id, user_id, category, amount, date, description";

constexpr auto kSelectIncomes =
    "SELECT id, user_id, category, amount, date, description "
    "FROM planned_incomes "
    "WHERE user_id = $1 "
    "ORDER BY date DESC";

constexpr auto kUpdateIncome =
    "UPDATE planned_incomes "
    "SET category = $3, amount = $4, date = $5, description = $6 "
    "WHERE id = $1 AND user_id = $2 "
    "RETURNING id, user_id, category, amount, date, description";

constexpr auto kDeleteIncome =
    "DELETE FROM planned_incomes WHERE id = $1 AND user_id = $2";

constexpr auto kInsertExpense =
    "INSERT INTO planned_expenses (user_id, category, amount, date, description) "
    "VALUES ($1, $2, $3, $4, $5) "
    "RETURNING id, user_id, category, amount, date, description";

constexpr auto kSelectExpenses =
    "SELECT id, user_id, category, amount, date, description "
    "FROM planned_expenses "
    "WHERE user_id = $1 "
    "ORDER BY date DESC";

constexpr auto kUpdateExpense =
    "UPDATE planned_expenses "
    "SET category = $3, amount = $4, date = $5, description = $6 "
    "WHERE id = $1 AND user_id = $2 "
    "RETURNING id, user_id, category, amount, date, description";

constexpr auto kDeleteExpense =
    "DELETE FROM planned_expenses WHERE id = $1 AND user_id = $2";

constexpr auto kSumIncomes =
    "SELECT COALESCE(SUM(amount), 0) AS total "
    "FROM planned_incomes "
    "WHERE user_id = $1 AND date BETWEEN $2 AND $3";

constexpr auto kSumExpenses =
    "SELECT COALESCE(SUM(amount), 0) AS total "
    "FROM planned_expenses "
    "WHERE user_id = $1 AND date BETWEEN $2 AND $3";

constexpr auto kDailyDynamics =
    "SELECT "
    "  COALESCE(i.date, e.date) AS date, "
    "  COALESCE(i.daily_income, 0) AS income, "
    "  COALESCE(e.daily_expense, 0) AS expense, "
    "  COALESCE(i.daily_income, 0) - COALESCE(e.daily_expense, 0) AS balance "
    "FROM ("
    "  SELECT date, SUM(amount) AS daily_income "
    "  FROM planned_incomes "
    "  WHERE user_id = $1 AND date BETWEEN $2 AND $3 "
    "  GROUP BY date"
    ") i "
    "FULL OUTER JOIN ("
    "  SELECT date, SUM(amount) AS daily_expense "
    "  FROM planned_expenses "
    "  WHERE user_id = $1 AND date BETWEEN $2 AND $3 "
    "  GROUP BY date"
    ") e ON i.date = e.date "
    "ORDER BY COALESCE(i.date, e.date)";

}  // namespace sql

namespace {

using Decimal2 = userver::decimal64::Decimal<2>;
using Date = userver::utils::datetime::Date;

PlannedIncome RowToIncome(const userver::storages::postgres::Row& row) {
    PlannedIncome income;
    income.id = row["id"].As<int64_t>();
    income.user_id = row["user_id"].As<int64_t>();
    income.category = row["category"].As<std::string>();
    income.amount = row["amount"].As<Decimal2>();
    income.date = row["date"].As<Date>();
    income.description = row["description"].As<std::string>();
    return income;
}

PlannedExpense RowToExpense(const userver::storages::postgres::Row& row) {
    PlannedExpense expense;
    expense.id = row["id"].As<int64_t>();
    expense.user_id = row["user_id"].As<int64_t>();
    expense.category = row["category"].As<std::string>();
    expense.amount = row["amount"].As<Decimal2>();
    expense.date = row["date"].As<Date>();
    expense.description = row["description"].As<std::string>();
    return expense;
}

}  // namespace

BudgetStorage::BudgetStorage(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::components::ComponentBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db")
              .GetCluster()) {}

PlannedIncome BudgetStorage::CreateIncome(int64_t user_id,
                                           const std::string& category,
                                           Decimal2 amount, Date date,
                                           const std::string& description) {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        sql::kInsertIncome, user_id, category, amount, date, description);
    return RowToIncome(result[0]);
}

std::vector<PlannedIncome> BudgetStorage::GetIncomes(int64_t user_id) const {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        sql::kSelectIncomes, user_id);

    std::vector<PlannedIncome> incomes;
    for (auto row : result) {
        incomes.push_back(RowToIncome(row));
    }
    return incomes;
}

std::optional<PlannedIncome> BudgetStorage::UpdateIncome(
    int64_t user_id, int64_t income_id, const std::string& category,
    Decimal2 amount, Date date, const std::string& description) {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        sql::kUpdateIncome, income_id, user_id, category, amount, date,
        description);

    if (result.IsEmpty()) return std::nullopt;
    return RowToIncome(result[0]);
}

bool BudgetStorage::DeleteIncome(int64_t user_id, int64_t income_id) {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        sql::kDeleteIncome, income_id, user_id);
    return result.RowsAffected() > 0;
}

PlannedExpense BudgetStorage::CreateExpense(int64_t user_id,
                                             const std::string& category,
                                             Decimal2 amount, Date date,
                                             const std::string& description) {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        sql::kInsertExpense, user_id, category, amount, date, description);
    return RowToExpense(result[0]);
}

std::vector<PlannedExpense> BudgetStorage::GetExpenses(
    int64_t user_id) const {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        sql::kSelectExpenses, user_id);

    std::vector<PlannedExpense> expenses;
    for (auto row : result) {
        expenses.push_back(RowToExpense(row));
    }
    return expenses;
}

std::optional<PlannedExpense> BudgetStorage::UpdateExpense(
    int64_t user_id, int64_t expense_id, const std::string& category,
    Decimal2 amount, Date date, const std::string& description) {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        sql::kUpdateExpense, expense_id, user_id, category, amount, date,
        description);

    if (result.IsEmpty()) return std::nullopt;
    return RowToExpense(result[0]);
}

bool BudgetStorage::DeleteExpense(int64_t user_id, int64_t expense_id) {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        sql::kDeleteExpense, expense_id, user_id);
    return result.RowsAffected() > 0;
}

BudgetStorage::BudgetDynamics BudgetStorage::GetBudgetDynamics(
    int64_t user_id, Date from, Date to) const {
    auto income_totals = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        sql::kSumIncomes, user_id, from, to);

    auto expense_totals = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        sql::kSumExpenses, user_id, from, to);

    auto daily_result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        sql::kDailyDynamics, user_id, from, to);

    BudgetDynamics dynamics;
    dynamics.total_income = income_totals[0]["total"].As<Decimal2>();
    dynamics.total_expense = expense_totals[0]["total"].As<Decimal2>();
    dynamics.balance = dynamics.total_income - dynamics.total_expense;
    dynamics.period_from = from;
    dynamics.period_to = to;

    for (auto row : daily_result) {
        BudgetDynamicsEntry entry;
        entry.date = row["date"].As<Date>();
        entry.income = row["income"].As<Decimal2>();
        entry.expense = row["expense"].As<Decimal2>();
        entry.balance = row["balance"].As<Decimal2>();
        dynamics.daily.push_back(std::move(entry));
    }

    return dynamics;
}

userver::yaml_config::Schema BudgetStorage::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<
        userver::components::ComponentBase>(R"(
        type: object
        description: PostgreSQL storage for budget data
        additionalProperties: false
        properties: {}
    )");
}

}  // namespace budget
