#include "client_ip.hpp"

namespace budget {

std::string GetClientIp(const userver::server::http::HttpRequest& request) {
    const auto& xff = request.GetHeader("X-Forwarded-For");
    if (!xff.empty()) {
        const auto comma = xff.find(',');
        const auto first =
            comma == std::string::npos ? xff : xff.substr(0, comma);

        const auto begin = first.find_first_not_of(" \t");
        const auto end = first.find_last_not_of(" \t");
        if (begin != std::string::npos && end != std::string::npos) {
            return first.substr(begin, end - begin + 1);
        }
    }
    return request.GetRemoteAddress().PrimaryAddressString();
}

}  // namespace budget
