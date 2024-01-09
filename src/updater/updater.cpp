#include <std_include.hpp>

#include <console.hpp>

#include "file_updater.hpp"
#include "updater.hpp"

#include <utils/properties.hpp>

#define IW4X_VERSION_FILE "iw4x-version.json"
#define IW4X_RAW_FILES_UPDATE_FILE "release.zip"
#define IW4X_RAW_FILES_UPDATE_URL "https://github.com/iw4x/iw4x-rawfiles/releases/latest/download/" IW4X_RAW_FILES_UPDATE_FILE
#define IW4X_RAW_FILES_TAGS "https://api.github.com/repos/iw4x/iw4x-rawfiles/releases/latest"

namespace updater
{
	int update_iw4x()
	{
		const auto iw4_install = utils::properties::load("iw4-install");
		if (!iw4_install)
		{
			console::error("Failed to load properties file");
			return false;
		}

		const auto& base = iw4_install.value();

		file_updater file_updater{ "IW4x", base, IW4X_RAW_FILES_UPDATE_FILE, IW4X_VERSION_FILE, IW4X_RAW_FILES_TAGS, IW4X_RAW_FILES_UPDATE_URL };

		file_updater.add_dir_to_clean("iw4x");
		file_updater.add_dir_to_clean("zone");

		file_updater.add_file_to_skip("iw4sp.exe");

		return file_updater.update_if_necessary();
	}
}
