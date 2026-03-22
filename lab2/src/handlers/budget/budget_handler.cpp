#include "budget_handler.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_status.hpp>

#include "../auth/auth_handler.hpp"

namespace budget {

IncomeHandler::IncomeHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<BudgetStorage>()) {}

std::string IncomeHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    const auto user_id = context.GetData<int64_t>(kUserIdKey);

    if (request.GetMethod() == userver::server::http::HttpMethod::kPost) {
        userver::formats::json::Value body;
        try {
            body = userver::formats::json::FromString(request.RequestBody());
        } catch (const std::exception&) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Invalid JSON body"})";
        }

        const auto category = body["category"].As<std::string>("");
        const auto amount = body["amount"].As<double>(0.0);
        const auto date = body["date"].As<std::string>("");
        const auto description =
            body["description"].As<std::string>("");

        if (category.empty() || amount <= 0.0 || date.empty()) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Fields category, amount (>0), date are required"})";
        }

        auto income = storage_.CreateIncome(user_id, category, amount,
                                            date, description);
        response.SetStatus(userver::server::http::HttpStatus::kCreated);
        return userver::formats::json::ToString(
            userver::formats::json::ValueBuilder{income}.ExtractValue());
    }

    auto incomes = storage_.GetIncomes(user_id);
    userver::formats::json::ValueBuilder arr(
        userver::formats::json::Type::kArray);
    for (const auto& inc : incomes) {
        arr.PushBack(inc);
    }
    return userver::formats::json::ToString(arr.ExtractValue());
}

IncomeByIdHandler::IncomeByIdHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<BudgetStorage>()) {}

std::string IncomeByIdHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    const auto user_id = context.GetData<int64_t>(kUserIdKey);

    int64_t income_id = 0;
    try {
        income_id = std::stoll(request.GetPathArg("id"));
    } catch (const std::exception&) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return R"({"error":"Invalid income id"})";
    }

    if (request.GetMethod() == userver::server::http::HttpMethod::kPut) {
        userver::formats::json::Value body;
        try {
            body = userver::formats::json::FromString(request.RequestBody());
        } catch (const std::exception&) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Invalid JSON body"})";
        }

        const auto category = body["category"].As<std::string>("");
        const auto amount = body["amount"].As<double>(0.0);
        const auto date = body["date"].As<std::string>("");
        const auto description = body["description"].As<std::string>("");

        if (category.empty() || amount <= 0.0 || date.empty()) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Fields category, amount (>0), date are required"})";
        }

        auto updated = storage_.UpdateIncome(user_id, income_id, category,
                                             amount, date, description);
        if (!updated) {
            response.SetStatus(userver::server::http::HttpStatus::kNotFound);
            return R"({"error":"Income not found"})";
        }
        return userver::formats::json::ToString(
            userver::formats::json::ValueBuilder{*updated}.ExtractValue());
    }

    if (!storage_.DeleteIncome(user_id, income_id)) {
        response.SetStatus(userver::server::http::HttpStatus::kNotFound);
        return R"({"error":"Income not found"})";
    }
    return R"({"status":"deleted"})";
}

ExpenseHandler::ExpenseHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<BudgetStorage>()) {}

std::string ExpenseHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    const auto user_id = context.GetData<int64_t>(kUserIdKey);

    if (request.GetMethod() == userver::server::http::HttpMethod::kPost) {
        userver::formats::json::Value body;
        try {
            body = userver::formats::json::FromString(request.RequestBody());
        } catch (const std::exception&) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Invalid JSON body"})";
        }

        const auto category = body["category"].As<std::string>("");
        const auto amount = body["amount"].As<double>(0.0);
        const auto date = body["date"].As<std::string>("");
        const auto description =
            body["description"].As<std::string>("");

        if (category.empty() || amount <= 0.0 || date.empty()) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Fields category, amount (>0), date are required"})";
        }

        auto expense = storage_.CreateExpense(user_id, category, amount,
                                              date, description);
        response.SetStatus(userver::server::http::HttpStatus::kCreated);
        return userver::formats::json::ToString(
            userver::formats::json::ValueBuilder{expense}.ExtractValue());
    }

    auto expenses = storage_.GetExpenses(user_id);
    userver::formats::json::ValueBuilder arr(
        userver::formats::json::Type::kArray);
    for (const auto& exp : expenses) {
        arr.PushBack(exp);
    }
    return userver::formats::json::ToString(arr.ExtractValue());
}

ExpenseByIdHandler::ExpenseByIdHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<BudgetStorage>()) {}

std::string ExpenseByIdHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    const auto user_id = context.GetData<int64_t>(kUserIdKey);

    int64_t expense_id = 0;
    try {
        expense_id = std::stoll(request.GetPathArg("id"));
    } catch (const std::exception&) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return R"({"error":"Invalid expense id"})";
    }

    if (request.GetMethod() == userver::server::http::HttpMethod::kPut) {
        userver::formats::json::Value body;
        try {
            body = userver::formats::json::FromString(request.RequestBody());
        } catch (const std::exception&) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Invalid JSON body"})";
        }

        const auto category = body["category"].As<std::string>("");
        const auto amount = body["amount"].As<double>(0.0);
        const auto date = body["date"].As<std::string>("");
        const auto description = body["description"].As<std::string>("");

        if (category.empty() || amount <= 0.0 || date.empty()) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Fields category, amount (>0), date are required"})";
        }

        auto updated = storage_.UpdateExpense(user_id, expense_id, category,
                                              amount, date, description);
        if (!updated) {
            response.SetStatus(userver::server::http::HttpStatus::kNotFound);
            return R"({"error":"Expense not found"})";
        }
        return userver::formats::json::ToString(
            userver::formats::json::ValueBuilder{*updated}.ExtractValue());
    }

    if (!storage_.DeleteExpense(user_id, expense_id)) {
        response.SetStatus(userver::server::http::HttpStatus::kNotFound);
        return R"({"error":"Expense not found"})";
    }
    return R"({"status":"deleted"})";
}

BudgetDynamicsHandler::BudgetDynamicsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<BudgetStorage>()) {}

std::string BudgetDynamicsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    const auto user_id = context.GetData<int64_t>(kUserIdKey);

    const auto from = request.GetArg("from");
    const auto to = request.GetArg("to");

    if (from.empty() || to.empty()) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return R"json({"error":"Query parameters 'from' and 'to' are required (YYYY-MM-DD)"})json";
    }

    auto dynamics = storage_.GetBudgetDynamics(user_id, from, to);

    userver::formats::json::ValueBuilder builder;
    builder["total_income"] = dynamics.total_income;
    builder["total_expense"] = dynamics.total_expense;
    builder["balance"] = dynamics.balance;
    builder["period_from"] = dynamics.period_from;
    builder["period_to"] = dynamics.period_to;

    userver::formats::json::ValueBuilder daily_arr(
        userver::formats::json::Type::kArray);
    for (const auto& entry : dynamics.daily) {
        userver::formats::json::ValueBuilder day;
        day["date"] = entry.date;
        day["income"] = entry.income;
        day["expense"] = entry.expense;
        day["balance"] = entry.balance;
        daily_arr.PushBack(day.ExtractValue());
    }
    builder["daily"] = daily_arr.ExtractValue();

    return userver::formats::json::ToString(builder.ExtractValue());
}

void AppendBudgetHandlers(userver::components::ComponentList& list) {
    list.Append<IncomeHandler>();
    list.Append<IncomeByIdHandler>();
    list.Append<ExpenseHandler>();
    list.Append<ExpenseByIdHandler>();
    list.Append<BudgetDynamicsHandler>();
}

}  // namespace budget
