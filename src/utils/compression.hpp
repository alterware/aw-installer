#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#define CHUNK 16384u

namespace utils::compression
{
	namespace zlib
	{
		std::string compress(const std::string& data);
		std::string decompress(const std::string& data);
	}

	namespace zip
	{
		class archive
		{
		public:
			void add(const std::string& filename, const std::string& data);
			[[nodiscard]] bool write(const std::string& filename, const std::string& comment = {});

			static void decompress(const std::string& filename, const std::filesystem::path& out_dir);

		private:
			std::unordered_map<std::string, std::string> files_;
		};
	}
};
