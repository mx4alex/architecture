#pragma once

#include <atomic>
#include <optional>
#include <string>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/engine/shared_mutex.hpp>

#include "../../models/budget.hpp"

namespace budget {

class BudgetStorage final : public userver::components::ComponentBase {
 public:
    static constexpr std::string_view kName = "budget-storage";

    BudgetStorage(const userver::components::ComponentConfig& config,
                  const userver::components::ComponentContext& context);

    PlannedIncome CreateIncome(int64_t user_id, const std::string& category,
                               double amount, const std::string& date,
                               const std::string& description);
    std::vector<PlannedIncome> GetIncomes(int64_t user_id) const;
    std::optional<PlannedIncome> UpdateIncome(int64_t user_id,
                                              int64_t income_id,
                                              const std::string& category,
                                              double amount,
                                              const std::string& date,
                                              const std::string& description);
    bool DeleteIncome(int64_t user_id, int64_t income_id);

    PlannedExpense CreateExpense(int64_t user_id, const std::string& category,
                                 double amount, const std::string& date,
                                 const std::string& description);
    std::vector<PlannedExpense> GetExpenses(int64_t user_id) const;
    std::optional<PlannedExpense> UpdateExpense(int64_t user_id,
                                                int64_t expense_id,
                                                const std::string& category,
                                                double amount,
                                                const std::string& date,
                                                const std::string& description);
    bool DeleteExpense(int64_t user_id, int64_t expense_id);

    struct BudgetDynamicsEntry {
        std::string date;
        double income{0.0};
        double expense{0.0};
        double balance{0.0};
    };

    struct BudgetDynamics {
        double total_income{0.0};
        double total_expense{0.0};
        double balance{0.0};
        std::string period_from;
        std::string period_to;
        std::vector<BudgetDynamicsEntry> daily;
    };

    BudgetDynamics GetBudgetDynamics(int64_t user_id, const std::string& from,
                                     const std::string& to) const;

 private:
    mutable userver::engine::SharedMutex mutex_;

    std::vector<PlannedIncome> incomes_;
    std::vector<PlannedExpense> expenses_;

    std::atomic<int64_t> next_income_id_{1};
    std::atomic<int64_t> next_expense_id_{1};
};

}  // namespace budget
