#include "user_storage.hpp"

#include <userver/storages/postgres/component.hpp>

namespace budget {

namespace sql {

constexpr auto kInsertUser =
    "INSERT INTO users (login, password, first_name, last_name) "
    "VALUES ($1, crypt($2, gen_salt('bf')), $3, $4) "
    "ON CONFLICT (login) DO NOTHING "
    "RETURNING id, login, password, first_name, last_name";

constexpr auto kSelectUserByLogin =
    "SELECT id, login, password, first_name, last_name "
    "FROM users WHERE login = $1";

constexpr auto kSearchUsersByName =
    "SELECT id, login, password, first_name, last_name "
    "FROM users "
    "WHERE first_name ILIKE '%' || $1 || '%' "
    "   OR last_name  ILIKE '%' || $1 || '%'";

constexpr auto kValidateCredentials =
    "SELECT id FROM users "
    "WHERE login = $1 AND password = crypt($2, password)";

}  // namespace sql

namespace {

User RowToUser(const userver::storages::postgres::Row& row) {
    User user;
    user.id = row["id"].As<int64_t>();
    user.login = row["login"].As<std::string>();
    user.password = row["password"].As<std::string>();
    user.first_name = row["first_name"].As<std::string>();
    user.last_name = row["last_name"].As<std::string>();
    return user;
}

}  // namespace

UserStorage::UserStorage(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context)
    : userver::components::ComponentBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db")
              .GetCluster()),
      jwt_secret_(
          config["jwt_secret"].As<std::string>("budget-service-secret-key")) {}

std::optional<User> UserStorage::CreateUser(const std::string& login,
                                            const std::string& password,
                                            const std::string& first_name,
                                            const std::string& last_name) {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        sql::kInsertUser, login, password, first_name, last_name);

    if (result.IsEmpty()) return std::nullopt;
    return RowToUser(result[0]);
}

std::optional<User> UserStorage::FindUserByLogin(
    const std::string& login) const {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        sql::kSelectUserByLogin, login);

    if (result.IsEmpty()) return std::nullopt;
    return RowToUser(result[0]);
}

std::vector<User> UserStorage::SearchUsersByName(
    const std::string& mask) const {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        sql::kSearchUsersByName, mask);

    std::vector<User> users;
    for (auto row : result) {
        users.push_back(RowToUser(row));
    }
    return users;
}

bool UserStorage::ValidateCredentials(const std::string& login,
                                      const std::string& password) const {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        sql::kValidateCredentials, login, password);
    return !result.IsEmpty();
}

userver::yaml_config::Schema UserStorage::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<
        userver::components::ComponentBase>(R"(
        type: object
        description: PostgreSQL storage for user data
        additionalProperties: false
        properties:
            jwt_secret:
                type: string
                description: Secret key for JWT signing and verification
                defaultDescription: budget-service-secret-key
    )");
}

}  // namespace budget
