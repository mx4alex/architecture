#include "user_handler.hpp"

#include <chrono>
#include <string>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_status.hpp>

namespace budget {

namespace {

constexpr std::string_view kUsersSearchVersionKey = "cv:users:search";
constexpr std::chrono::seconds kSearchTtl{120};

}  // namespace

UserSearchHandler::UserSearchHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<UserStorage>()),
      cache_(context.FindComponent<CacheService>()) {}

std::string UserSearchHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    const auto login = request.GetArg("login");
    const auto mask = request.GetArg("mask");
    const auto legacy_name = request.GetArg("name");
    const auto resolved_mask = mask.empty() ? legacy_name : mask;

    if (login.empty() && resolved_mask.empty()) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return R"({"error":"Query parameter 'login' or 'mask' is required"})";
    }

    if (login.empty()) {
        const auto version =
            cache_.GetVersion(std::string{kUsersSearchVersionKey});
        const std::string cache_key = "c:users:search:v" +
                                      std::to_string(version) + ":" +
                                      resolved_mask;

        if (auto cached = cache_.Get(cache_key, "users.search")) {
            response.SetHeader(std::string_view{"X-Cache"},
                               std::string{"HIT"});
            return *cached;
        }

        const auto users = storage_.SearchUsersByName(resolved_mask);
        userver::formats::json::ValueBuilder result_array(
            userver::formats::json::Type::kArray);
        for (const auto& user : users) {
            result_array.PushBack(UserResponse::FromUser(user));
        }
        auto serialized = userver::formats::json::ToString(
            result_array.ExtractValue());

        cache_.Set(cache_key, serialized, kSearchTtl, "users.search");
        response.SetHeader(std::string_view{"X-Cache"}, std::string{"MISS"});
        return serialized;
    }

    userver::formats::json::ValueBuilder result_array(
        userver::formats::json::Type::kArray);

    const auto user = storage_.FindUserByLogin(login);
    if (user) {
        result_array.PushBack(UserResponse::FromUser(*user));
    }
    return userver::formats::json::ToString(result_array.ExtractValue());
}

void AppendUserHandlers(userver::components::ComponentList& list) {
    list.Append<UserSearchHandler>();
}

}  // namespace budget
