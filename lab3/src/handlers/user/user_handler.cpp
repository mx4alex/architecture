#include "user_handler.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_status.hpp>

#include "../auth/auth_handler.hpp"

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
    const auto name = request.GetArg("name");

    if (login.empty() && name.empty()) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return R"({"error":"Query parameter 'login' or 'name' is required"})";
    }

    userver::formats::json::ValueBuilder result_array(
        userver::formats::json::Type::kArray);

    if (!login.empty()) {
        auto user = storage_.FindUserByLogin(login);
        if (user) {
            result_array.PushBack(UserResponse::FromUser(*user));
        }
    } else {
        auto users = storage_.SearchUsersByName(name);
        for (const auto& u : users) {
            result_array.PushBack(UserResponse::FromUser(u));
        }
    }

    return userver::formats::json::ToString(result_array.ExtractValue());
}

void AppendUserHandlers(userver::components::ComponentList& list) {
    list.Append<UserSearchHandler>();
}

}  // namespace budget
