#pragma once

#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "../../storage/user/user_storage.hpp"

namespace budget {

class UserSearchHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
    static constexpr std::string_view kName = "handler-user-search";

    UserSearchHandler(const userver::components::ComponentConfig& config,
                      const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

 private:
    UserStorage& storage_;
};

void AppendUserHandlers(userver::components::ComponentList& list);

}  // namespace budget
