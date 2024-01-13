#pragma once

#include <future>
#include <optional>
#include <string>
#include <unordered_map>

namespace utils::http
{
	using headers = std::unordered_map<std::string, std::string>;

	std::optional<std::string> get_data(const std::string& url, const headers& headers = {});
	std::future<std::optional<std::string>> get_data_async(const std::string& url, const headers& headers = {});
}
