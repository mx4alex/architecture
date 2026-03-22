#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace budget {

std::string CreateJwt(int64_t user_id, const std::string& secret);
std::optional<int64_t> VerifyJwt(const std::string& token,
                                 const std::string& secret);

}  // namespace budget
