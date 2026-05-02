#include "budget_storage.hpp"

#include <cstdint>
#include <cmath>
#include <map>

#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/storages/mongo/options.hpp>
#include <userver/utils/datetime/date.hpp>

namespace budget {

namespace {

using Decimal2 = userver::decimal64::Decimal<2>;
using Date = userver::utils::datetime::Date;
using userver::formats::bson::MakeDoc;

constexpr std::string_view kCountersCollection = "counters";
constexpr std::string_view kIncomesCollection = "planned_incomes";
constexpr std::string_view kExpensesCollection = "planned_expenses";
constexpr std::string_view kIncomeSequenceKey = "incomes_id_seq";
constexpr std::string_view kExpenseSequenceKey = "expenses_id_seq";

double DecimalToDouble(Decimal2 v) {
    return static_cast<double>(v.AsUnbiased()) / 100.0;
}

Decimal2 DoubleToDecimal(double v) {
    return Decimal2::FromUnbiased(std::llround(v * 100));
}

double ReadNumericAsDouble(const userver::formats::bson::Value& value) {
    if (value.IsDouble()) return value.As<double>();
    if (value.IsInt32()) return static_cast<double>(value.As<int32_t>());
    if (value.IsInt64()) return static_cast<double>(value.As<int64_t>());
    return value.As<double>();
}

}  // namespace

BudgetStorage::BudgetStorage(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::components::ComponentBase(config, context),
      mongo_pool_(
          context.FindComponent<userver::components::Mongo>("mongo-db")
              .GetPool()) {}

int64_t BudgetStorage::NextId(std::string_view sequence_key) const {
    auto counters = mongo_pool_->GetCollection(std::string{kCountersCollection});
    const auto write_result = counters.FindAndModify(
        MakeDoc("_id", std::string{sequence_key}),
        MakeDoc("$inc", MakeDoc("value", int64_t{1})),
        userver::storages::mongo::options::Upsert{},
        userver::storages::mongo::options::ReturnNew{});

    const auto doc = write_result.FoundDocument();
    if (!doc || !doc->HasMember("value")) {
        return 1;
    }

    return (*doc)["value"].As<int64_t>(1);
}

PlannedIncome BudgetStorage::ParseIncome(
    const userver::formats::bson::Document& doc) const {
    PlannedIncome income;
    income.id = doc["id"].As<int64_t>();
    income.user_id = doc["user_id"].As<int64_t>();
    income.category = doc["category"].As<std::string>();
    income.amount = DoubleToDecimal(ReadNumericAsDouble(doc["amount"]));
    income.date = userver::utils::datetime::DateFromRFC3339String(
        doc["date"].As<std::string>());
    income.description = doc["description"].As<std::string>("");
    return income;
}

PlannedExpense BudgetStorage::ParseExpense(
    const userver::formats::bson::Document& doc) const {
    PlannedExpense expense;
    expense.id = doc["id"].As<int64_t>();
    expense.user_id = doc["user_id"].As<int64_t>();
    expense.category = doc["category"].As<std::string>();
    expense.amount = DoubleToDecimal(ReadNumericAsDouble(doc["amount"]));
    expense.date = userver::utils::datetime::DateFromRFC3339String(
        doc["date"].As<std::string>());
    expense.description = doc["description"].As<std::string>("");
    return expense;
}

PlannedIncome BudgetStorage::CreateIncome(int64_t user_id,
                                          const std::string& category,
                                          Decimal2 amount, Date date,
                                          const std::string& description) {
    PlannedIncome income;
    income.id = NextId(kIncomeSequenceKey);
    income.user_id = user_id;
    income.category = category;
    income.amount = amount;
    income.date = date;
    income.description = description;

    auto incomes = mongo_pool_->GetCollection(std::string{kIncomesCollection});
    incomes.InsertOne(MakeDoc("id", income.id, "user_id", income.user_id,
                              "category", income.category, "amount",
                              DecimalToDouble(income.amount), "date",
                              userver::utils::datetime::ToString(income.date),
                              "description", income.description));
    return income;
}

std::vector<PlannedIncome> BudgetStorage::GetIncomes(int64_t user_id) const {
    auto incomes = mongo_pool_->GetCollection(std::string{kIncomesCollection});

    userver::storages::mongo::options::Sort sort;
    sort.By("date", userver::storages::mongo::options::Sort::kDescending)
        .By("id", userver::storages::mongo::options::Sort::kDescending);

    std::vector<PlannedIncome> result;
    for (const auto& doc : incomes.Find(MakeDoc("user_id", user_id), sort)) {
        result.push_back(ParseIncome(doc));
    }
    return result;
}

std::optional<PlannedIncome> BudgetStorage::UpdateIncome(
    int64_t user_id, int64_t income_id, const std::string& category,
    Decimal2 amount, Date date, const std::string& description) {
    auto incomes = mongo_pool_->GetCollection(std::string{kIncomesCollection});
    const auto write_result = incomes.FindAndModify(
        MakeDoc("id", income_id, "user_id", user_id),
        MakeDoc("$set",
                MakeDoc("category", category, "amount", DecimalToDouble(amount),
                        "date", userver::utils::datetime::ToString(date),
                        "description", description)),
        userver::storages::mongo::options::ReturnNew{});

    const auto doc = write_result.FoundDocument();
    if (!doc) {
        return std::nullopt;
    }

    return ParseIncome(*doc);
}

bool BudgetStorage::DeleteIncome(int64_t user_id, int64_t income_id) {
    auto incomes = mongo_pool_->GetCollection(std::string{kIncomesCollection});
    const auto write_result =
        incomes.DeleteOne(MakeDoc("id", income_id, "user_id", user_id));
    return write_result.DeletedCount() > 0;
}

PlannedExpense BudgetStorage::CreateExpense(int64_t user_id,
                                            const std::string& category,
                                            Decimal2 amount, Date date,
                                            const std::string& description) {
    PlannedExpense expense;
    expense.id = NextId(kExpenseSequenceKey);
    expense.user_id = user_id;
    expense.category = category;
    expense.amount = amount;
    expense.date = date;
    expense.description = description;

    auto expenses = mongo_pool_->GetCollection(std::string{kExpensesCollection});
    expenses.InsertOne(MakeDoc("id", expense.id, "user_id", expense.user_id,
                               "category", expense.category, "amount",
                               DecimalToDouble(expense.amount), "date",
                               userver::utils::datetime::ToString(expense.date),
                               "description", expense.description));
    return expense;
}

std::vector<PlannedExpense> BudgetStorage::GetExpenses(int64_t user_id) const {
    auto expenses = mongo_pool_->GetCollection(std::string{kExpensesCollection});

    userver::storages::mongo::options::Sort sort;
    sort.By("date", userver::storages::mongo::options::Sort::kDescending)
        .By("id", userver::storages::mongo::options::Sort::kDescending);

    std::vector<PlannedExpense> result;
    for (const auto& doc : expenses.Find(MakeDoc("user_id", user_id), sort)) {
        result.push_back(ParseExpense(doc));
    }
    return result;
}

std::optional<PlannedExpense> BudgetStorage::UpdateExpense(
    int64_t user_id, int64_t expense_id, const std::string& category,
    Decimal2 amount, Date date, const std::string& description) {
    auto expenses = mongo_pool_->GetCollection(std::string{kExpensesCollection});
    const auto write_result = expenses.FindAndModify(
        MakeDoc("id", expense_id, "user_id", user_id),
        MakeDoc("$set",
                MakeDoc("category", category, "amount", DecimalToDouble(amount),
                        "date", userver::utils::datetime::ToString(date),
                        "description", description)),
        userver::storages::mongo::options::ReturnNew{});

    const auto doc = write_result.FoundDocument();
    if (!doc) {
        return std::nullopt;
    }

    return ParseExpense(*doc);
}

bool BudgetStorage::DeleteExpense(int64_t user_id, int64_t expense_id) {
    auto expenses = mongo_pool_->GetCollection(std::string{kExpensesCollection});
    const auto write_result =
        expenses.DeleteOne(MakeDoc("id", expense_id, "user_id", user_id));
    return write_result.DeletedCount() > 0;
}

BudgetStorage::BudgetDynamics BudgetStorage::GetBudgetDynamics(
    int64_t user_id, Date from, Date to) const {
    const auto from_str = userver::utils::datetime::ToString(from);
    const auto to_str = userver::utils::datetime::ToString(to);

    BudgetDynamics dynamics;
    dynamics.total_income = Decimal2::FromUnbiased(0);
    dynamics.total_expense = Decimal2::FromUnbiased(0);
    dynamics.balance = Decimal2::FromUnbiased(0);
    dynamics.period_from = from;
    dynamics.period_to = to;

    int64_t total_income_cents = 0;
    int64_t total_expense_cents = 0;
    std::map<std::string, std::pair<int64_t, int64_t>> daily_cents;

    const auto make_period_filter = [&]() {
        return MakeDoc("user_id", user_id, "date",
                       MakeDoc("$gte", from_str, "$lte", to_str));
    };

    auto incomes = mongo_pool_->GetCollection(std::string{kIncomesCollection});
    for (const auto& doc : incomes.Find(make_period_filter())) {
        const auto date = doc["date"].As<std::string>();
        const auto amount_cents = DoubleToDecimal(ReadNumericAsDouble(doc["amount"]))
                                      .AsUnbiased();
        total_income_cents += amount_cents;
        daily_cents[date].first += amount_cents;
    }

    auto expenses = mongo_pool_->GetCollection(std::string{kExpensesCollection});
    for (const auto& doc : expenses.Find(make_period_filter())) {
        const auto date = doc["date"].As<std::string>();
        const auto amount_cents = DoubleToDecimal(ReadNumericAsDouble(doc["amount"]))
                                      .AsUnbiased();
        total_expense_cents += amount_cents;
        daily_cents[date].second += amount_cents;
    }

    dynamics.total_income = Decimal2::FromUnbiased(total_income_cents);
    dynamics.total_expense = Decimal2::FromUnbiased(total_expense_cents);
    dynamics.balance = dynamics.total_income - dynamics.total_expense;

    for (const auto& [date, day_values] : daily_cents) {
        BudgetDynamicsEntry entry;
        entry.date = userver::utils::datetime::DateFromRFC3339String(date);
        entry.income = Decimal2::FromUnbiased(day_values.first);
        entry.expense = Decimal2::FromUnbiased(day_values.second);
        entry.balance = entry.income - entry.expense;
        dynamics.daily.push_back(std::move(entry));
    }

    return dynamics;
}

userver::yaml_config::Schema BudgetStorage::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<
        userver::components::ComponentBase>(R"(
        type: object
        description: MongoDB storage for budget data
        additionalProperties: false
        properties: {}
    )");
}

}  // namespace budget
