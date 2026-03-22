#include "budget_storage.hpp"

#include <algorithm>
#include <map>

namespace budget {

BudgetStorage::BudgetStorage(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::components::ComponentBase(config, context) {}

PlannedIncome BudgetStorage::CreateIncome(int64_t user_id,
                                           const std::string& category,
                                           double amount,
                                           const std::string& date,
                                           const std::string& description) {
    std::unique_lock lock(mutex_);
    PlannedIncome income;
    income.id = next_income_id_++;
    income.user_id = user_id;
    income.category = category;
    income.amount = amount;
    income.date = date;
    income.description = description;
    incomes_.push_back(income);
    return income;
}

std::vector<PlannedIncome> BudgetStorage::GetIncomes(int64_t user_id) const {
    std::shared_lock lock(mutex_);
    std::vector<PlannedIncome> result;
    for (const auto& income : incomes_) {
        if (income.user_id == user_id) {
            result.push_back(income);
        }
    }
    return result;
}

std::optional<PlannedIncome> BudgetStorage::UpdateIncome(
    int64_t user_id, int64_t income_id, const std::string& category,
    double amount, const std::string& date, const std::string& description) {
    std::unique_lock lock(mutex_);
    auto it = std::find_if(
        incomes_.begin(), incomes_.end(), [&](const PlannedIncome& i) {
            return i.id == income_id && i.user_id == user_id;
        });
    if (it == incomes_.end()) return std::nullopt;
    it->category = category;
    it->amount = amount;
    it->date = date;
    it->description = description;
    return *it;
}

bool BudgetStorage::DeleteIncome(int64_t user_id, int64_t income_id) {
    std::unique_lock lock(mutex_);
    auto it = std::find_if(
        incomes_.begin(), incomes_.end(), [&](const PlannedIncome& i) {
            return i.id == income_id && i.user_id == user_id;
        });
    if (it == incomes_.end()) return false;
    incomes_.erase(it);
    return true;
}

PlannedExpense BudgetStorage::CreateExpense(int64_t user_id,
                                             const std::string& category,
                                             double amount,
                                             const std::string& date,
                                             const std::string& description) {
    std::unique_lock lock(mutex_);
    PlannedExpense expense;
    expense.id = next_expense_id_++;
    expense.user_id = user_id;
    expense.category = category;
    expense.amount = amount;
    expense.date = date;
    expense.description = description;
    expenses_.push_back(expense);
    return expense;
}

std::vector<PlannedExpense> BudgetStorage::GetExpenses(int64_t user_id) const {
    std::shared_lock lock(mutex_);
    std::vector<PlannedExpense> result;
    for (const auto& expense : expenses_) {
        if (expense.user_id == user_id) {
            result.push_back(expense);
        }
    }
    return result;
}

std::optional<PlannedExpense> BudgetStorage::UpdateExpense(
    int64_t user_id, int64_t expense_id, const std::string& category,
    double amount, const std::string& date, const std::string& description) {
    std::unique_lock lock(mutex_);
    auto it = std::find_if(
        expenses_.begin(), expenses_.end(), [&](const PlannedExpense& e) {
            return e.id == expense_id && e.user_id == user_id;
        });
    if (it == expenses_.end()) return std::nullopt;
    it->category = category;
    it->amount = amount;
    it->date = date;
    it->description = description;
    return *it;
}

bool BudgetStorage::DeleteExpense(int64_t user_id, int64_t expense_id) {
    std::unique_lock lock(mutex_);
    auto it = std::find_if(
        expenses_.begin(), expenses_.end(), [&](const PlannedExpense& e) {
            return e.id == expense_id && e.user_id == user_id;
        });
    if (it == expenses_.end()) return false;
    expenses_.erase(it);
    return true;
}

BudgetStorage::BudgetDynamics BudgetStorage::GetBudgetDynamics(
    int64_t user_id, const std::string& from, const std::string& to) const {
    std::shared_lock lock(mutex_);

    std::map<std::string, BudgetDynamicsEntry> daily_map;

    double total_income = 0.0;
    double total_expense = 0.0;

    for (const auto& income : incomes_) {
        if (income.user_id == user_id && income.date >= from &&
            income.date <= to) {
            total_income += income.amount;
            daily_map[income.date].date = income.date;
            daily_map[income.date].income += income.amount;
        }
    }

    for (const auto& expense : expenses_) {
        if (expense.user_id == user_id && expense.date >= from &&
            expense.date <= to) {
            total_expense += expense.amount;
            daily_map[expense.date].date = expense.date;
            daily_map[expense.date].expense += expense.amount;
        }
    }

    BudgetDynamics dynamics;
    dynamics.total_income = total_income;
    dynamics.total_expense = total_expense;
    dynamics.balance = total_income - total_expense;
    dynamics.period_from = from;
    dynamics.period_to = to;

    for (auto& [date, entry] : daily_map) {
        entry.balance = entry.income - entry.expense;
        dynamics.daily.push_back(entry);
    }

    return dynamics;
}

}  // namespace budget
