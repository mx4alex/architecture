#pragma once

#include <atomic>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include "../../models/user.hpp"

namespace budget {

class UserStorage final : public userver::components::ComponentBase {
 public:
    static constexpr std::string_view kName = "user-storage";

    UserStorage(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& context);

    std::optional<User> CreateUser(const std::string& login,
                                   const std::string& password,
                                   const std::string& first_name,
                                   const std::string& last_name);

    std::optional<User> FindUserByLogin(const std::string& login) const;

    std::vector<User> SearchUsersByName(const std::string& mask) const;

    bool ValidateCredentials(const std::string& login,
                             const std::string& password) const;

    const std::string& GetJwtSecret() const { return jwt_secret_; }

    static userver::yaml_config::Schema GetStaticConfigSchema();

 private:
    mutable userver::engine::SharedMutex mutex_;

    std::unordered_map<int64_t, User> users_;
    std::unordered_map<std::string, int64_t> login_index_;
    std::string jwt_secret_;

    std::atomic<int64_t> next_user_id_{1};
};

}  // namespace budget
