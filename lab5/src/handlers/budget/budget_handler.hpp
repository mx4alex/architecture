#pragma once

#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "../../cache/cache_service.hpp"
#include "../../ratelimit/rate_limiter.hpp"
#include "../../storage/budget/budget_storage.hpp"

namespace budget {

class IncomeHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
    static constexpr std::string_view kName = "handler-incomes";

    IncomeHandler(const userver::components::ComponentConfig& config,
                  const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

 private:
    BudgetStorage& storage_;
    CacheService& cache_;
};

class IncomeByIdHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
    static constexpr std::string_view kName = "handler-income-by-id";

    IncomeByIdHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

 private:
    BudgetStorage& storage_;
    CacheService& cache_;
};

class ExpenseHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
    static constexpr std::string_view kName = "handler-expenses";

    ExpenseHandler(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

 private:
    BudgetStorage& storage_;
    CacheService& cache_;
};

class ExpenseByIdHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
    static constexpr std::string_view kName = "handler-expense-by-id";

    ExpenseByIdHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

 private:
    BudgetStorage& storage_;
    CacheService& cache_;
};

class BudgetDynamicsHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
    static constexpr std::string_view kName = "handler-budget-dynamics";

    BudgetDynamicsHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

 private:
    BudgetStorage& storage_;
    CacheService& cache_;
    const RateLimiter& rate_limiter_;
};

void AppendBudgetHandlers(userver::components::ComponentList& list);

}  // namespace budget
