#pragma once

namespace updater
{
	class file_updater
	{
	public:
		file_updater(std::string name, std::filesystem::path base, std::filesystem::path out_name, std::filesystem::path version_file, std::string remote_tag, std::string remote_download);

		[[nodiscard]] bool update_if_necessary() const;

		void add_dir_to_clean(const std::string& dir);
		void add_file_to_skip(const std::string& file);

	private:
		struct update_state
		{
			bool requires_update = false;
			std::string latest_tag;
		};

		std::string name_;

		std::filesystem::path base_;
		std::filesystem::path out_name_;
		std::filesystem::path version_file_;

		std::string remote_tag_;
		std::string remote_download_;

		// Directories to cleanup
		std::vector<std::filesystem::path> cleanup_directories_;

		// Files to skip
		std::vector<std::string> skip_files_;

		[[nodiscard]] std::string read_local_revision_file() const;
		[[nodiscard]] bool does_require_update(update_state& update_state, const std::string& local_version) const;
		void create_version_file(const std::string& revision_version) const;
		[[nodiscard]] bool update_file(const std::string& url) const;
		[[nodiscard]] bool deploy_files() const;

		void cleanup_directories() const;
		void skip_files(const std::filesystem::path& target_dir) const;
	};
}
