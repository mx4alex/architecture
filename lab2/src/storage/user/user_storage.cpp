#include "user_storage.hpp"

#include <algorithm>

namespace budget {

namespace {

std::string ToLower(std::string_view s) {
    std::string result(s);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool ContainsIgnoreCase(const std::string& haystack,
                        const std::string& needle) {
    return ToLower(haystack).find(ToLower(needle)) != std::string::npos;
}

}  // namespace

UserStorage::UserStorage(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context)
    : userver::components::ComponentBase(config, context),
      jwt_secret_(
          config["jwt_secret"].As<std::string>("budget-service-secret-key")) {}

std::optional<User> UserStorage::CreateUser(const std::string& login,
                                            const std::string& password,
                                            const std::string& first_name,
                                            const std::string& last_name) {
    std::unique_lock lock(mutex_);
    if (login_index_.count(login)) {
        return std::nullopt;
    }

    User user;
    user.id = next_user_id_++;
    user.login = login;
    user.password = password;
    user.first_name = first_name;
    user.last_name = last_name;

    login_index_[login] = user.id;
    users_[user.id] = user;
    return user;
}

std::optional<User> UserStorage::FindUserByLogin(
    const std::string& login) const {
    std::shared_lock lock(mutex_);
    auto it = login_index_.find(login);
    if (it == login_index_.end()) return std::nullopt;
    auto user_it = users_.find(it->second);
    if (user_it == users_.end()) return std::nullopt;
    return user_it->second;
}

std::vector<User> UserStorage::SearchUsersByName(
    const std::string& mask) const {
    std::shared_lock lock(mutex_);
    std::vector<User> result;
    for (const auto& [id, user] : users_) {
        if (ContainsIgnoreCase(user.first_name, mask) ||
            ContainsIgnoreCase(user.last_name, mask)) {
            result.push_back(user);
        }
    }
    return result;
}

bool UserStorage::ValidateCredentials(const std::string& login,
                                      const std::string& password) const {
    std::shared_lock lock(mutex_);
    auto it = login_index_.find(login);
    if (it == login_index_.end()) return false;
    auto user_it = users_.find(it->second);
    if (user_it == users_.end()) return false;
    return user_it->second.password == password;
}

userver::yaml_config::Schema UserStorage::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<
        userver::components::ComponentBase>(R"(
        type: object
        description: In-memory storage for user data
        additionalProperties: false
        properties:
            jwt_secret:
                type: string
                description: Secret key for JWT signing and verification
                defaultDescription: budget-service-secret-key
    )");
}

}  // namespace budget
