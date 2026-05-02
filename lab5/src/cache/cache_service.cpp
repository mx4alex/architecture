#include "cache_service.hpp"

#include <userver/logging/log.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/redis/exception.hpp>

namespace budget {

namespace {

constexpr std::chrono::milliseconds kRedisTimeoutSingle{200};
constexpr std::chrono::milliseconds kRedisTimeoutAll{500};
constexpr std::size_t kRedisMaxRetries{2};

}  // namespace

CacheService::CacheService(const userver::components::ComponentConfig& config,
                           const userver::components::ComponentContext& context)
    : userver::components::ComponentBase(config, context),
      redis_client_(context
                        .FindComponent<userver::components::Redis>(
                            config["redis_database_name"].As<std::string>())
                        .GetClient(config["redis_client_name"].As<std::string>())),
      command_control_(kRedisTimeoutSingle, kRedisTimeoutAll, kRedisMaxRetries),
      default_ttl_(std::chrono::milliseconds{
          config["default_ttl_ms"].As<std::int64_t>(60000)}) {}

std::optional<std::string> CacheService::Get(const std::string& key,
                                             std::string_view metric_label) const {
    try {
        auto reply = redis_client_->Get(key, command_control_).Get();
        if (reply.has_value()) {
            hits_.fetch_add(1, std::memory_order_relaxed);
            return reply;
        }
        misses_.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    } catch (const userver::storages::redis::Exception& ex) {
        errors_.fetch_add(1, std::memory_order_relaxed);
        LOG_WARNING() << "cache get error endpoint=" << metric_label
                      << " key=" << key << " err=" << ex.what();
        return std::nullopt;
    }
}

bool CacheService::Set(const std::string& key, std::string value,
                       std::chrono::milliseconds ttl,
                       std::string_view metric_label) const {
    try {
        redis_client_->Set(key, std::move(value), ttl, command_control_).Get();
        writes_.fetch_add(1, std::memory_order_relaxed);
        return true;
    } catch (const userver::storages::redis::Exception& ex) {
        errors_.fetch_add(1, std::memory_order_relaxed);
        LOG_WARNING() << "cache set error endpoint=" << metric_label
                      << " key=" << key << " err=" << ex.what();
        return false;
    }
}

bool CacheService::Set(const std::string& key, std::string value,
                       std::string_view metric_label) const {
    return Set(key, std::move(value), default_ttl_, metric_label);
}

void CacheService::Delete(const std::string& key) const {
    try {
        redis_client_->Del(key, command_control_).Get();
    } catch (const userver::storages::redis::Exception& ex) {
        errors_.fetch_add(1, std::memory_order_relaxed);
        LOG_WARNING() << "cache delete error key=" << key
                      << " err=" << ex.what();
    }
}

std::int64_t CacheService::GetVersion(const std::string& version_key) const {
    try {
        const auto reply = redis_client_->Get(version_key, command_control_).Get();
        if (!reply.has_value()) {
            return 0;
        }
        return std::stoll(*reply);
    } catch (const userver::storages::redis::Exception& ex) {
        errors_.fetch_add(1, std::memory_order_relaxed);
        LOG_WARNING() << "cache version read error key=" << version_key
                      << " err=" << ex.what();
        return 0;
    } catch (const std::exception& ex) {
        LOG_WARNING() << "cache version parse error key=" << version_key
                      << " err=" << ex.what();
        return 0;
    }
}

std::int64_t CacheService::BumpVersion(const std::string& version_key) const {
    try {
        const auto value =
            redis_client_->Incr(version_key, command_control_).Get();
        try {
            redis_client_
                ->Expire(version_key, std::chrono::seconds{24 * 3600},
                         command_control_)
                .Get();
        } catch (const userver::storages::redis::Exception& ex) {
            LOG_INFO() << "cache version expire refresh failed key="
                       << version_key << " err=" << ex.what();
        }
        return value;
    } catch (const userver::storages::redis::Exception& ex) {
        errors_.fetch_add(1, std::memory_order_relaxed);
        LOG_WARNING() << "cache version bump error key=" << version_key
                      << " err=" << ex.what();
        return 0;
    }
}

CacheService::Stats CacheService::GetStats() const {
    return Stats{hits_.load(std::memory_order_relaxed),
                 misses_.load(std::memory_order_relaxed),
                 errors_.load(std::memory_order_relaxed),
                 writes_.load(std::memory_order_relaxed)};
}

userver::yaml_config::Schema CacheService::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(R"(
        type: object
        description: Cache-aside service backed by Redis
        additionalProperties: false
        properties:
            redis_database_name:
                type: string
                description: Имя компонента components::Redis для FindComponent.
            redis_client_name:
                type: string
                description: Имя клиента (db) внутри Redis-компонента.
            default_ttl_ms:
                type: integer
                description: Базовый TTL в миллисекундах для Set без явного TTL.
                defaultDescription: '60000'
    )");
}

}  // namespace budget
