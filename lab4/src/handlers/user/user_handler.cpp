#include "user_handler.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_status.hpp>

namespace budget {

UserSearchHandler::UserSearchHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<UserStorage>()) {}

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

    userver::formats::json::ValueBuilder result_array(
        userver::formats::json::Type::kArray);

    if (!login.empty()) {
        const auto user = storage_.FindUserByLogin(login);
        if (user) {
            result_array.PushBack(UserResponse::FromUser(*user));
        }
    } else {
        const auto users = storage_.SearchUsersByName(resolved_mask);
        for (const auto& user : users) {
            result_array.PushBack(UserResponse::FromUser(user));
        }
    }

    return userver::formats::json::ToString(result_array.ExtractValue());
}

void AppendUserHandlers(userver::components::ComponentList& list) {
    list.Append<UserSearchHandler>();
}

}  // namespace budget
