#include "auth_handler.hpp"

#include <cctype>
#include <optional>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_status.hpp>

#include "../../utils/jwt.hpp"

namespace budget {

namespace {

bool IsValidLogin(const std::string& login) {
    if (login.size() < 4 || login.size() > 32) {
        return false;
    }

    for (unsigned char c : login) {
        if (!(std::isalnum(c) || c == '_')) {
            return false;
        }
    }

    return true;
}

}  // namespace

class AuthCheckerBearer final
    : public userver::server::handlers::auth::AuthCheckerBase {
 public:
    using AuthCheckResult = userver::server::handlers::auth::AuthCheckResult;

    explicit AuthCheckerBearer(std::string jwt_secret)
        : jwt_secret_(std::move(jwt_secret)) {}

    [[nodiscard]] AuthCheckResult CheckAuth(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

    [[nodiscard]] bool SupportsUserAuth() const noexcept override {
        return true;
    }

 private:
    const std::string jwt_secret_;
};

AuthCheckerBearer::AuthCheckResult AuthCheckerBearer::CheckAuth(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    const auto& auth_value = request.GetHeader("Authorization");
    if (auth_value.empty()) {
        return AuthCheckResult{
            AuthCheckResult::Status::kTokenNotFound,
            {},
            "Empty 'Authorization' header",
            userver::server::handlers::HandlerErrorCode::kUnauthorized};
    }

    const auto bearer_sep_pos = auth_value.find(' ');
    if (bearer_sep_pos == std::string::npos ||
        std::string_view{auth_value.data(), bearer_sep_pos} != "Bearer") {
        return AuthCheckResult{
            AuthCheckResult::Status::kTokenNotFound,
            {},
            "'Authorization' header should have 'Bearer <token>' format",
            userver::server::handlers::HandlerErrorCode::kUnauthorized};
    }

    const std::string token{auth_value.data() + bearer_sep_pos + 1};
    const auto user_id = VerifyJwt(token, jwt_secret_);
    if (!user_id) {
        return AuthCheckResult{AuthCheckResult::Status::kForbidden};
    }

    context.SetData(kUserIdKey, *user_id);
    return {};
}

CheckerFactory::CheckerFactory(
    const userver::components::ComponentContext& context)
    : storage_(context.FindComponent<UserStorage>()) {}

userver::server::handlers::auth::AuthCheckerBasePtr
CheckerFactory::MakeAuthChecker(
    const userver::server::handlers::auth::HandlerAuthConfig&) const {
    return std::make_shared<AuthCheckerBearer>(storage_.GetJwtSecret());
}

RegisterHandler::RegisterHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<UserStorage>()) {}

std::string RegisterHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    userver::formats::json::Value body;
    try {
        body = userver::formats::json::FromString(request.RequestBody());
    } catch (const std::exception&) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return R"({"error":"Invalid JSON body"})";
    }

    const auto login = body["login"].As<std::string>("");
    const auto password = body["password"].As<std::string>("");
    const auto first_name = body["first_name"].As<std::string>("");
    const auto last_name = body["last_name"].As<std::string>("");

    if (login.empty() || password.empty() || first_name.empty() ||
        last_name.empty()) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return R"({"error":"Fields login, password, first_name, last_name are required"})";
    }

    if (!IsValidLogin(login)) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return R"({"error":"Login must be 4..32 chars and contain only letters, numbers or underscore"})";
    }

    std::optional<User> user;
    try {
        user = storage_.CreateUser(login, password, first_name, last_name);
    } catch (const std::exception&) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return R"({"error":"Failed to create user with provided data"})";
    }

    if (!user) {
        response.SetStatus(userver::server::http::HttpStatus::kConflict);
        return R"({"error":"User with this login already exists"})";
    }

    response.SetStatus(userver::server::http::HttpStatus::kCreated);
    const auto dto = UserResponse::FromUser(*user);
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{dto}.ExtractValue());
}

LoginHandler::LoginHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<UserStorage>()) {}

std::string LoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    userver::formats::json::Value body;
    try {
        body = userver::formats::json::FromString(request.RequestBody());
    } catch (const std::exception&) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return R"({"error":"Invalid JSON body"})";
    }

    const auto login = body["login"].As<std::string>("");
    const auto password = body["password"].As<std::string>("");

    if (login.empty() || password.empty()) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return R"({"error":"Fields login and password are required"})";
    }

    if (!storage_.ValidateCredentials(login, password)) {
        response.SetStatus(userver::server::http::HttpStatus::kUnauthorized);
        return R"({"error":"Invalid login or password"})";
    }

    const auto user = storage_.FindUserByLogin(login);
    if (!user) {
        response.SetStatus(
            userver::server::http::HttpStatus::kInternalServerError);
        return R"({"error":"Internal server error"})";
    }

    const auto token = CreateJwt(user->id, storage_.GetJwtSecret());

    userver::formats::json::ValueBuilder builder;
    builder["token"] = token;
    return userver::formats::json::ToString(builder.ExtractValue());
}

void AppendAuthHandlers(userver::components::ComponentList& list) {
    list.Append<RegisterHandler>();
    list.Append<LoginHandler>();
}

}  // namespace budget
