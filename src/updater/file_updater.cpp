#include <std_include.hpp>

#include <console.hpp>

#include "file_updater.hpp"

#include <utils/compression.hpp>
#include <utils/http.hpp>
#include <utils/io.hpp>

namespace updater
{
	namespace
	{
		std::optional<std::string> get_release_tag(const std::string& release_url)
		{
			const auto release_info = utils::http::get_data(release_url);
			if (!release_info.has_value())
			{
				console::warn("Could not reach remote URL \"%s\"", release_url.c_str());
				return {};
			}

			rapidjson::Document release_json{};

			const rapidjson::ParseResult result = release_json.Parse(release_info.value());
			if (!result || !release_json.IsObject())
			{
				console::error("Could not parse remote JSON response from \"%s\"", release_url.c_str());
				return {};
			}

			if (release_json.HasMember("tag_name") && release_json["tag_name"].IsString())
			{
				const auto* tag_name = release_json["tag_name"].GetString();
				return tag_name;
			}

			console::error("Remote JSON response from \"%s\" does not contain the data we expected", release_url.c_str());
			return {};
		}
	}

	file_updater::file_updater(std::string name, std::filesystem::path base, std::filesystem::path out_name,
	                           std::filesystem::path version_file,
	                           std::string remote_tag, std::string remote_download)
		: name_(std::move(name))
		, base_(std::move(base))
		, out_name_(std::move(out_name))
		, version_file_(std::move(version_file))
		, remote_tag_(std::move(remote_tag))
		, remote_download_(std::move(remote_download))
	{
	}

	bool file_updater::update_if_necessary() const
	{
		update_state update_state;

		const auto local_version = this->read_local_revision_file();
		if (!this->does_require_update(update_state, local_version))
		{
			console::log("%s does not require an update", this->name_.c_str());
			return true;
		}

		console::info("Cleaning up directories");
		this->cleanup_directories();

		console::info("Updating %s", this->name_.c_str());

		if (!this->update_file(this->remote_download_))
		{
			console::error("Update failed");
			return false;
		}
		
		if (!this->deploy_files())
		{
			console::error("Unable to deploy files");
			return false;
		}

		// Do this last to make sure we don't ever create a version file when something failed
		this->create_version_file(update_state.latest_tag);
		return true;
	}

	void file_updater::add_dir_to_clean(const std::string& dir)
	{
		this->cleanup_directories_.emplace_back(this->base_ / dir);
	}

	void file_updater::add_file_to_skip(const std::string& file)
	{
		this->skip_files_.emplace_back(file);
	}

	std::string file_updater::read_local_revision_file() const
	{
		const std::filesystem::path revision_file_path = this->version_file_;

		std::string data;
		if (!utils::io::read_file(revision_file_path.string(), &data) || data.empty())
		{
			console::warn("Could not load \"%s\"", revision_file_path.string().c_str());
			return {};
		}

		rapidjson::Document doc{};
		const rapidjson::ParseResult result = doc.Parse(data);
		if (!result || !doc.IsObject())
		{
			console::error("Could not parse \"%s\"", revision_file_path.string().c_str());
			return {};
		}

		if (!doc.HasMember("version") || !doc["version"].IsString())
		{
			console::error("\"%s\" contains invalid data", revision_file_path.string().c_str());
			return {};
		}

		return doc["version"].GetString();
	}

	bool file_updater::does_require_update(update_state& update_state, const std::string& local_version) const
	{
		console::info("Fetching tags from GitHub");

		const auto raw_files_tag = get_release_tag(this->remote_tag_);
		if (!raw_files_tag.has_value())
		{
			console::warn("Failed to reach GitHub. Aborting the update");

			update_state.requires_update = false;
			return update_state.requires_update;
		}

		update_state.requires_update = local_version != raw_files_tag.value();
		update_state.latest_tag = raw_files_tag.value();

		console::info("Got release tag \"%s\". Requires updating: %s", raw_files_tag.value().c_str(), update_state.requires_update ? "Yes" : "No");
		return update_state.requires_update;
	}

	void file_updater::create_version_file(const std::string& revision_version) const
	{
		console::info("Creating version file \"%s\". Revision is \"%s\"", this->version_file_.c_str(), revision_version.c_str());

		rapidjson::Document doc{};
		doc.SetObject();

		doc.AddMember("version", revision_version, doc.GetAllocator());

		rapidjson::StringBuffer buffer{};
		rapidjson::Writer<rapidjson::StringBuffer, rapidjson::Document::EncodingType, rapidjson::ASCII<>>
			writer(buffer);
		doc.Accept(writer);

		const std::string json(buffer.GetString(), buffer.GetLength());
		if (utils::io::write_file(this->version_file_.string(), json))
		{
			console::info("File \"%s\" was created successfully", this->version_file_.string().c_str());
			return;
		}

		console::error("Error while writing file \"%s\"", this->version_file_.string().c_str());
	}

	bool file_updater::update_file(const std::string& url) const
	{
		console::info("Downloading %s", url.c_str());
		const auto data = utils::http::get_data(url, {});
		if (!data)
		{
			console::error("Failed to download %s", url.c_str());
			return false;
		}

		if (data.value().empty())
		{
			console::error("The data buffer returned by Curl is empty");
			return false;
		}

		// Download the files in the working directory, move them later.
		const auto out_file = std::filesystem::current_path() / this->out_name_;

		console::info("Writing file to \"%s\"", out_file.string().c_str());

		if (!utils::io::write_file(out_file.string(), *data, false))
		{
			console::error("Error while writing file \"%s\"", out_file.string().c_str());
			return false;
		}

		console::info("Done updating file \"%s\"", out_file.string().c_str());
		return true;
	}

	// Not a fan of using exceptions here. Once C++23 is more widespread I'd like to use <expected>
	bool file_updater::deploy_files() const
	{
		const auto out_dir = std::filesystem::current_path() / ".out";

		assert(utils::io::file_exists(this->out_name_.string()));

		// Always try to cleanup
		const auto _ = gsl::finally([this, &out_dir]() -> void
		{
			utils::io::remove_file(this->out_name_.string());

			std::error_code ec;
			std::filesystem::remove_all(out_dir, ec);
		});

		try
		{
			utils::io::create_directory(out_dir);
			utils::compression::zip::archive::decompress(this->out_name_.string(), out_dir);
		}
		catch (const std::exception& ex)
		{
			console::error("Get error \"%s\" while decompressing \"%s\"", ex.what(), this->out_name_.string().c_str());
			return false;
		}

		console::info("\"%s\" was decompressed. Removing files that must be skipped", this->out_name_.string().c_str());
		this->skip_files(out_dir);

		console::info("Deploying files to \"%s\"", this->base_.string().c_str());

		utils::io::copy_folder(out_dir, this->base_);
		return true;
	}

	void file_updater::cleanup_directories() const
	{
		std::for_each(this->cleanup_directories_.begin(), this->cleanup_directories_.end(), [](const auto& dir)
		{
			std::error_code ec;
			std::filesystem::remove_all(dir, ec);
			console::log("Removed directory \"%s\"", dir.string().c_str());
		});
	}

	void file_updater::skip_files(const std::filesystem::path& target_dir) const
	{
		std::for_each(this->cleanup_directories_.begin(), this->cleanup_directories_.end(), [&target_dir](const auto& file)
		{
			const auto target_file = target_dir / file;
			utils::io::remove_file(target_file.string());
			console::log("Removed file \"%s\"", target_file.string().c_str());
		});
	}
}
