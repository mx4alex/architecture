#pragma once

#include <cstdint>
#include <string>

#include <userver/decimal64/decimal64.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/datetime/date.hpp>

namespace budget {

struct PlannedIncome {
    int64_t id{0};
    int64_t user_id{0};
    std::string category;
    userver::decimal64::Decimal<2> amount;
    userver::utils::datetime::Date date;
    std::string description;
};

struct PlannedExpense {
    int64_t id{0};
    int64_t user_id{0};
    std::string category;
    userver::decimal64::Decimal<2> amount;
    userver::utils::datetime::Date date;
    std::string description;
};

inline userver::formats::json::Value Serialize(
    const PlannedIncome& v,
    userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder b;
    b["id"] = v.id;
    b["user_id"] = v.user_id;
    b["category"] = v.category;
    b["amount"] = static_cast<double>(v.amount.AsUnbiased()) / 100.0;
    b["date"] = userver::utils::datetime::ToString(v.date);
    b["description"] = v.description;
    return b.ExtractValue();
}

inline userver::formats::json::Value Serialize(
    const PlannedExpense& v,
    userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder b;
    b["id"] = v.id;
    b["user_id"] = v.user_id;
    b["category"] = v.category;
    b["amount"] = static_cast<double>(v.amount.AsUnbiased()) / 100.0;
    b["date"] = userver::utils::datetime::ToString(v.date);
    b["description"] = v.description;
    return b.ExtractValue();
}

}  // namespace budget
