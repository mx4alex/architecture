#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/storages/redis/client_fwd.hpp>
#include <userver/storages/redis/command_control.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace budget {

class CacheService final : public userver::components::ComponentBase {
 public:
    static constexpr std::string_view kName = "cache-service";

    struct Stats {
        std::uint64_t hits{0};
        std::uint64_t misses{0};
        std::uint64_t errors{0};
        std::uint64_t writes{0};
    };

    CacheService(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);

    std::optional<std::string> Get(const std::string& key,
                                   std::string_view metric_label = {}) const;

    bool Set(const std::string& key, std::string value,
             std::chrono::milliseconds ttl,
             std::string_view metric_label = {}) const;

    bool Set(const std::string& key, std::string value,
             std::string_view metric_label = {}) const;

    void Delete(const std::string& key) const;

    std::int64_t GetVersion(const std::string& version_key) const;

    std::int64_t BumpVersion(const std::string& version_key) const;

    Stats GetStats() const;

    static userver::yaml_config::Schema GetStaticConfigSchema();

 private:
    userver::storages::redis::ClientPtr redis_client_;
    userver::storages::redis::CommandControl command_control_;
    std::chrono::milliseconds default_ttl_;

    mutable std::atomic<std::uint64_t> hits_{0};
    mutable std::atomic<std::uint64_t> misses_{0};
    mutable std::atomic<std::uint64_t> errors_{0};
    mutable std::atomic<std::uint64_t> writes_{0};
};

}  // namespace budget
