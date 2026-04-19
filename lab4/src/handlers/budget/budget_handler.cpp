#include "budget_handler.hpp"

#include <cmath>
#include <optional>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/datetime/date.hpp>

#include "../auth/auth_handler.hpp"

namespace budget {

namespace {

using Decimal2 = userver::decimal64::Decimal<2>;
using Date = userver::utils::datetime::Date;

Decimal2 DoubleToDecimal(double v) {
    return Decimal2::FromUnbiased(std::llround(v * 100));
}

double DecimalToDouble(Decimal2 v) {
    return static_cast<double>(v.AsUnbiased()) / 100.0;
}

std::optional<Date> ParseDate(const std::string& date_str) {
    try {
        return userver::utils::datetime::DateFromRFC3339String(date_str);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

}  // namespace

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
        const auto amount_d = body["amount"].As<double>(0.0);
        const auto date_str = body["date"].As<std::string>("");
        const auto description = body["description"].As<std::string>("");

        if (category.empty() || amount_d <= 0.0 || date_str.empty()) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Fields category, amount (>0), date are required"})";
        }

        const auto date = ParseDate(date_str);
        if (!date) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Invalid date format, expected YYYY-MM-DD"})";
        }

        const auto income = storage_.CreateIncome(
            user_id, category, DoubleToDecimal(amount_d), *date, description);
        response.SetStatus(userver::server::http::HttpStatus::kCreated);
        return userver::formats::json::ToString(
            userver::formats::json::ValueBuilder{income}.ExtractValue());
    }

    const auto incomes = storage_.GetIncomes(user_id);
    userver::formats::json::ValueBuilder arr(
        userver::formats::json::Type::kArray);
    for (const auto& income : incomes) {
        arr.PushBack(income);
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
        const auto amount_d = body["amount"].As<double>(0.0);
        const auto date_str = body["date"].As<std::string>("");
        const auto description = body["description"].As<std::string>("");

        if (category.empty() || amount_d <= 0.0 || date_str.empty()) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Fields category, amount (>0), date are required"})";
        }

        const auto date = ParseDate(date_str);
        if (!date) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Invalid date format, expected YYYY-MM-DD"})";
        }

        const auto updated = storage_.UpdateIncome(
            user_id, income_id, category, DoubleToDecimal(amount_d), *date,
            description);
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
        const auto amount_d = body["amount"].As<double>(0.0);
        const auto date_str = body["date"].As<std::string>("");
        const auto description = body["description"].As<std::string>("");

        if (category.empty() || amount_d <= 0.0 || date_str.empty()) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Fields category, amount (>0), date are required"})";
        }

        const auto date = ParseDate(date_str);
        if (!date) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Invalid date format, expected YYYY-MM-DD"})";
        }

        const auto expense = storage_.CreateExpense(
            user_id, category, DoubleToDecimal(amount_d), *date, description);
        response.SetStatus(userver::server::http::HttpStatus::kCreated);
        return userver::formats::json::ToString(
            userver::formats::json::ValueBuilder{expense}.ExtractValue());
    }

    const auto expenses = storage_.GetExpenses(user_id);
    userver::formats::json::ValueBuilder arr(
        userver::formats::json::Type::kArray);
    for (const auto& expense : expenses) {
        arr.PushBack(expense);
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
        const auto amount_d = body["amount"].As<double>(0.0);
        const auto date_str = body["date"].As<std::string>("");
        const auto description = body["description"].As<std::string>("");

        if (category.empty() || amount_d <= 0.0 || date_str.empty()) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Fields category, amount (>0), date are required"})";
        }

        const auto date = ParseDate(date_str);
        if (!date) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Invalid date format, expected YYYY-MM-DD"})";
        }

        const auto updated = storage_.UpdateExpense(
            user_id, expense_id, category, DoubleToDecimal(amount_d), *date,
            description);
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

    const auto from_str = request.GetArg("from");
    const auto to_str = request.GetArg("to");

    if (from_str.empty() || to_str.empty()) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return "{\"error\":\"Query parameters 'from' and 'to' are required (YYYY-MM-DD)\"}";
    }

    const auto from = ParseDate(from_str);
    const auto to = ParseDate(to_str);
    if (!from || !to) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return R"({"error":"Invalid date format, expected YYYY-MM-DD"})";
    }

    const auto dynamics = storage_.GetBudgetDynamics(user_id, *from, *to);

    userver::formats::json::ValueBuilder builder;
    builder["total_income"] = DecimalToDouble(dynamics.total_income);
    builder["total_expense"] = DecimalToDouble(dynamics.total_expense);
    builder["balance"] = DecimalToDouble(dynamics.balance);
    builder["period_from"] =
        userver::utils::datetime::ToString(dynamics.period_from);
    builder["period_to"] = userver::utils::datetime::ToString(dynamics.period_to);

    userver::formats::json::ValueBuilder daily_arr(
        userver::formats::json::Type::kArray);
    for (const auto& entry : dynamics.daily) {
        userver::formats::json::ValueBuilder day;
        day["date"] = userver::utils::datetime::ToString(entry.date);
        day["income"] = DecimalToDouble(entry.income);
        day["expense"] = DecimalToDouble(entry.expense);
        day["balance"] = DecimalToDouble(entry.balance);
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
