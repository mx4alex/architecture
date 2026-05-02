#pragma once

#include <string>

#include <userver/server/http/http_request.hpp>

namespace budget {

std::string GetClientIp(const userver::server::http::HttpRequest& request);

}  // namespace budget
