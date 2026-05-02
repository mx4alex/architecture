#include "rate_limiter.hpp"

#include <atomic>
#include <chrono>
#include <string>

#include <userver/logging/log.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/redis/exception.hpp>
#include <userver/storages/redis/transaction.hpp>
#include <userver/utils/datetime.hpp>

namespace budget {

namespace {

constexpr std::chrono::milliseconds kRedisTimeoutSingle{200};
constexpr std::chrono::milliseconds kRedisTimeoutAll{500};
constexpr std::size_t kRedisMaxRetries{1};

std::int64_t NowEpochMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               userver::utils::datetime::Now().time_since_epoch())
        .count();
}

std::int64_t UniqueMember() {
    static std::atomic<std::int64_t> counter{0};
    return counter.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace

RateLimiter::RateLimiter(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context)
    : userver::components::ComponentBase(config, context),
      redis_client_(context
                        .FindComponent<userver::components::Redis>(
                            config["redis_database_name"].As<std::string>())
                        .GetClient(config["redis_client_name"].As<std::string>())),
      command_control_(kRedisTimeoutSingle, kRedisTimeoutAll, kRedisMaxRetries) {
    login_.limit = config["login_limit"].As<std::int64_t>(10);
    login_.window =
        std::chrono::seconds{config["login_window_seconds"].As<std::int64_t>(60)};

    register_.limit = config["register_limit"].As<std::int64_t>(5);
    register_.window = std::chrono::seconds{
        config["register_window_seconds"].As<std::int64_t>(60)};

    dynamics_.limit = config["dynamics_limit"].As<std::int64_t>(60);
    dynamics_.window = std::chrono::seconds{
        config["dynamics_window_seconds"].As<std::int64_t>(60)};
}

RateLimitDecision RateLimiter::CheckFixedWindow(
    const std::string& key, const RateLimitConfig& cfg,
    std::string_view metric_label) const {
    RateLimitDecision decision;
    decision.limit = cfg.limit;
    decision.reset_seconds = cfg.window.count();

    try {
        auto txn = redis_client_->Multi();
        auto incr_request = txn->Incr(key);
        auto expire_request = txn->Expire(
            key, cfg.window,
            userver::storages::redis::ExpireOptions{
                userver::storages::redis::ExpireOptions::Exist::kSetIfNotExist});
        txn->Exec(command_control_).Get();

        const auto count = incr_request.Get();
        (void)expire_request.Get();

        decision.remaining = std::max<std::int64_t>(0, cfg.limit - count);
        decision.allowed = count <= cfg.limit;

        if (decision.allowed) {
            allowed_.fetch_add(1, std::memory_order_relaxed);
        } else {
            rejected_.fetch_add(1, std::memory_order_relaxed);
            LOG_WARNING() << "ratelimit rejected endpoint=" << metric_label
                          << " key=" << key << " count=" << count
                          << " limit=" << cfg.limit;
        }
    } catch (const userver::storages::redis::Exception& ex) {
        errors_.fetch_add(1, std::memory_order_relaxed);
        LOG_WARNING() << "ratelimit redis error endpoint=" << metric_label
                      << " key=" << key << " err=" << ex.what()
                      << " (fail-open)";
        decision.allowed = true;
        decision.remaining = cfg.limit;
    }

    return decision;
}

RateLimitDecision RateLimiter::CheckSlidingWindow(
    const std::string& key, const RateLimitConfig& cfg,
    std::string_view metric_label) const {
    RateLimitDecision decision;
    decision.limit = cfg.limit;
    decision.reset_seconds = cfg.window.count();

    const auto now_ms = NowEpochMillis();
    const auto window_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(cfg.window).count();
    const auto cutoff_ms = now_ms - window_ms;
    const std::string member =
        std::to_string(now_ms) + "-" + std::to_string(UniqueMember());

    try {
        auto txn = redis_client_->Multi();
        auto rem_request = txn->Zremrangebyscore(key, 0.0,
                                                 static_cast<double>(cutoff_ms));
        auto add_request = txn->Zadd(key, static_cast<double>(now_ms), member);
        auto card_request = txn->Zcard(key);
        auto expire_request = txn->Expire(key, cfg.window);
        txn->Exec(command_control_).Get();

        (void)rem_request.Get();
        (void)add_request.Get();
        const auto count = static_cast<std::int64_t>(card_request.Get());
        (void)expire_request.Get();

        decision.remaining = std::max<std::int64_t>(0, cfg.limit - count);
        decision.allowed = count <= cfg.limit;

        if (decision.allowed) {
            allowed_.fetch_add(1, std::memory_order_relaxed);
        } else {
            rejected_.fetch_add(1, std::memory_order_relaxed);
            LOG_WARNING() << "ratelimit rejected (sliding) endpoint="
                          << metric_label << " key=" << key
                          << " count=" << count << " limit=" << cfg.limit;
        }
    } catch (const userver::storages::redis::Exception& ex) {
        errors_.fetch_add(1, std::memory_order_relaxed);
        LOG_WARNING() << "ratelimit redis error endpoint=" << metric_label
                      << " key=" << key << " err=" << ex.what()
                      << " (fail-open)";
        decision.allowed = true;
        decision.remaining = cfg.limit;
    }

    return decision;
}

RateLimiter::Stats RateLimiter::GetStats() const {
    return Stats{allowed_.load(std::memory_order_relaxed),
                 rejected_.load(std::memory_order_relaxed),
                 errors_.load(std::memory_order_relaxed)};
}

userver::yaml_config::Schema RateLimiter::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(R"(
        type: object
        description: Redis-backed rate limiter (Fixed window + Sliding window log)
        additionalProperties: false
        properties:
            redis_database_name:
                type: string
                description: Имя компонента components::Redis для FindComponent.
            redis_client_name:
                type: string
                description: Имя клиента (db) внутри Redis-компонента.
            login_limit:
                type: integer
                description: Лимит запросов на /api/v1/auth/login на IP за окно.
                defaultDescription: '10'
            login_window_seconds:
                type: integer
                description: Длина окна (sec) для login.
                defaultDescription: '60'
            register_limit:
                type: integer
                description: Лимит запросов на /api/v1/auth/register на IP.
                defaultDescription: '5'
            register_window_seconds:
                type: integer
                description: Длина окна (sec) для register.
                defaultDescription: '60'
            dynamics_limit:
                type: integer
                description: Лимит запросов на /api/v1/budget/dynamics на user.
                defaultDescription: '60'
            dynamics_window_seconds:
                type: integer
                description: Длина окна (sec) для dynamics.
                defaultDescription: '60'
    )");
}

}  // namespace budget
