#pragma once

#include <filesystem>
#include <optional>

namespace utils::properties
{
	std::optional<std::string> load(const std::string& name);
	void store(const std::string& name, const std::string& value);
}
