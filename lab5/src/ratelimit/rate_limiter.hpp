#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/storages/redis/client_fwd.hpp>
#include <userver/storages/redis/command_control.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace budget {

struct RateLimitDecision {
    bool allowed{true};
    std::int64_t limit{0};
    std::int64_t remaining{0};
    std::int64_t reset_seconds{0};
};

struct RateLimitConfig {
    std::int64_t limit{0};
    std::chrono::seconds window{60};
};

class RateLimiter final : public userver::components::ComponentBase {
 public:
    static constexpr std::string_view kName = "rate-limiter";

    RateLimiter(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& context);

    RateLimitDecision CheckFixedWindow(const std::string& key,
                                       const RateLimitConfig& cfg,
                                       std::string_view metric_label) const;

    RateLimitDecision CheckSlidingWindow(const std::string& key,
                                         const RateLimitConfig& cfg,
                                         std::string_view metric_label) const;

    const RateLimitConfig& Login() const { return login_; }
    const RateLimitConfig& Register() const { return register_; }
    const RateLimitConfig& Dynamics() const { return dynamics_; }

    struct Stats {
        std::uint64_t allowed{0};
        std::uint64_t rejected{0};
        std::uint64_t errors{0};
    };

    Stats GetStats() const;

    static userver::yaml_config::Schema GetStaticConfigSchema();

 private:
    userver::storages::redis::ClientPtr redis_client_;
    userver::storages::redis::CommandControl command_control_;

    RateLimitConfig login_;
    RateLimitConfig register_;
    RateLimitConfig dynamics_;

    mutable std::atomic<std::uint64_t> allowed_{0};
    mutable std::atomic<std::uint64_t> rejected_{0};
    mutable std::atomic<std::uint64_t> errors_{0};
};

}  // namespace budget
