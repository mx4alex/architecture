#include "jwt.hpp"

#include <jwt-cpp/jwt.h>

namespace budget {

namespace {
constexpr std::string_view kJwtIssuer = "budget-service";
constexpr auto kJwtTtl = std::chrono::hours{24};
}  // namespace

std::string CreateJwt(int64_t user_id, const std::string& secret) {
    auto now = std::chrono::system_clock::now();
    return jwt::create()
        .set_issuer(std::string{kJwtIssuer})
        .set_type("JWT")
        .set_issued_at(now)
        .set_expires_at(now + kJwtTtl)
        .set_payload_claim("user_id", jwt::claim(std::to_string(user_id)))
        .sign(jwt::algorithm::hs256{secret});
}

std::optional<int64_t> VerifyJwt(const std::string& token,
                                 const std::string& secret) {
    try {
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{secret})
            .with_issuer(std::string{kJwtIssuer});

        auto decoded = jwt::decode(token);
        verifier.verify(decoded);

        auto claim = decoded.get_payload_claim("user_id");
        return std::stoll(claim.as_string());
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

}  // namespace budget
