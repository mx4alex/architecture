#pragma once

#include <string>

#include <userver/components/component_list.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>

#include "../../storage/user/user_storage.hpp"

namespace budget {

inline const std::string kUserIdKey = "user_id";

class CheckerFactory final
    : public userver::server::handlers::auth::AuthCheckerFactoryBase {
 public:
    static constexpr std::string_view kAuthType = "bearer";

    explicit CheckerFactory(
        const userver::components::ComponentContext& context);

    userver::server::handlers::auth::AuthCheckerBasePtr MakeAuthChecker(
        const userver::server::handlers::auth::HandlerAuthConfig&
            auth_config) const override;

 private:
    const UserStorage& storage_;
};

class RegisterHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
    static constexpr std::string_view kName = "handler-register";

    RegisterHandler(const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

 private:
    UserStorage& storage_;
};

class LoginHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
    static constexpr std::string_view kName = "handler-login";

    LoginHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

 private:
    UserStorage& storage_;
};

void AppendAuthHandlers(userver::components::ComponentList& list);

}  // namespace budget
