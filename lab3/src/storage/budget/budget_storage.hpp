#pragma once

#include <optional>
#include <string>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/decimal64/decimal64.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/utils/datetime/date.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include "../../models/budget.hpp"

namespace budget {

class BudgetStorage final : public userver::components::ComponentBase {
 public:
    static constexpr std::string_view kName = "budget-storage";

    BudgetStorage(const userver::components::ComponentConfig& config,
                  const userver::components::ComponentContext& context);

    PlannedIncome CreateIncome(int64_t user_id, const std::string& category,
                               userver::decimal64::Decimal<2> amount,
                               userver::utils::datetime::Date date,
                               const std::string& description);
    std::vector<PlannedIncome> GetIncomes(int64_t user_id) const;
    std::optional<PlannedIncome> UpdateIncome(
        int64_t user_id, int64_t income_id, const std::string& category,
        userver::decimal64::Decimal<2> amount,
        userver::utils::datetime::Date date, const std::string& description);
    bool DeleteIncome(int64_t user_id, int64_t income_id);

    PlannedExpense CreateExpense(int64_t user_id, const std::string& category,
                                 userver::decimal64::Decimal<2> amount,
                                 userver::utils::datetime::Date date,
                                 const std::string& description);
    std::vector<PlannedExpense> GetExpenses(int64_t user_id) const;
    std::optional<PlannedExpense> UpdateExpense(
        int64_t user_id, int64_t expense_id, const std::string& category,
        userver::decimal64::Decimal<2> amount,
        userver::utils::datetime::Date date, const std::string& description);
    bool DeleteExpense(int64_t user_id, int64_t expense_id);

    struct BudgetDynamicsEntry {
        userver::utils::datetime::Date date;
        userver::decimal64::Decimal<2> income;
        userver::decimal64::Decimal<2> expense;
        userver::decimal64::Decimal<2> balance;
    };

    struct BudgetDynamics {
        userver::decimal64::Decimal<2> total_income;
        userver::decimal64::Decimal<2> total_expense;
        userver::decimal64::Decimal<2> balance;
        userver::utils::datetime::Date period_from;
        userver::utils::datetime::Date period_to;
        std::vector<BudgetDynamicsEntry> daily;
    };

    BudgetDynamics GetBudgetDynamics(int64_t user_id,
                                     userver::utils::datetime::Date from,
                                     userver::utils::datetime::Date to) const;

    static userver::yaml_config::Schema GetStaticConfigSchema();

 private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace budget
