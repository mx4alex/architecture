#pragma once

#include <cstdint>
#include <string>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace budget {

struct User {
    int64_t id{0};
    std::string login;
    std::string password;
    std::string first_name;
    std::string last_name;
};

struct UserResponse {
    int64_t id{0};
    std::string login;
    std::string first_name;
    std::string last_name;

    static UserResponse FromUser(const User& u) {
        return {u.id, u.login, u.first_name, u.last_name};
    }
};

inline userver::formats::json::Value Serialize(
    const UserResponse& v,
    userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder b;
    b["id"] = v.id;
    b["login"] = v.login;
    b["first_name"] = v.first_name;
    b["last_name"] = v.last_name;
    return b.ExtractValue();
}

}  // namespace budget
