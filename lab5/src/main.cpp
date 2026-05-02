#include "cache/cache_service.hpp"
#include "handlers/auth/auth_handler.hpp"
#include "handlers/budget/budget_handler.hpp"
#include "handlers/user/user_handler.hpp"
#include "ratelimit/rate_limiter.hpp"
#include "storage/budget/budget_storage.hpp"
#include "storage/user/user_storage.hpp"

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

int main(int argc, char* argv[]) {
    userver::server::handlers::auth::RegisterAuthCheckerFactory<
        budget::CheckerFactory>();

    auto component_list =
        userver::components::MinimalServerComponentList()
            .Append<userver::server::handlers::Ping>()
            .Append<userver::server::handlers::TestsControl>()
            .Append<userver::components::TestsuiteSupport>()
            .Append<userver::clients::dns::Component>()
            .AppendComponentList(userver::clients::http::ComponentList())
            .Append<userver::components::Postgres>("postgres-db")
            .Append<userver::components::Mongo>("mongo-db")
            .Append<userver::components::Secdist>()
            .Append<userver::components::DefaultSecdistProvider>()
            .Append<userver::components::Redis>("budget-cache-redis")
            .Append<budget::CacheService>()
            .Append<budget::RateLimiter>()
            .Append<budget::UserStorage>()
            .Append<budget::BudgetStorage>();

    budget::AppendAuthHandlers(component_list);
    budget::AppendUserHandlers(component_list);
    budget::AppendBudgetHandlers(component_list);

    return userver::utils::DaemonMain(argc, argv, component_list);
}
