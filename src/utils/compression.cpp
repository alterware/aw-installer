#include <std_include.hpp>

#include "compression.hpp"

#include <unzip.h>

#include <zlib.h>
#include <zip.h>

#include <gsl/gsl>

#include "io.hpp"
#include "string.hpp"

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

namespace utils::compression
{
	namespace zlib
	{
		namespace
		{
			class zlib_stream
			{
			public:
				zlib_stream()
				{
					memset(&stream_, 0, sizeof(stream_));
					valid_ = inflateInit(&stream_) == Z_OK;
				}

				zlib_stream(zlib_stream&&) = delete;
				zlib_stream(const zlib_stream&) = delete;
				zlib_stream& operator=(zlib_stream&&) = delete;
				zlib_stream& operator=(const zlib_stream&) = delete;

				~zlib_stream()
				{
					if (valid_)
					{
						inflateEnd(&stream_);
					}
				}

				z_stream& get()
				{
					return stream_; //
				}

				bool is_valid() const
				{
					return valid_;
				}

			private:
				bool valid_{false};
				z_stream stream_{};
			};
		}

		std::string decompress(const std::string& data)
		{
			std::string buffer{};
			zlib_stream stream_container{};
			if (!stream_container.is_valid())
			{
				return {};
			}

			int ret{};
			size_t offset = 0;
			static thread_local uint8_t dest[CHUNK] = {0};
			auto& stream = stream_container.get();

			do
			{
				const auto input_size = std::min(sizeof(dest), data.size() - offset);
				stream.avail_in = static_cast<uInt>(input_size);
				stream.next_in = reinterpret_cast<const Bytef*>(data.data()) + offset;
				offset += stream.avail_in;

				do
				{
					stream.avail_out = sizeof(dest);
					stream.next_out = dest;

					ret = inflate(&stream, Z_NO_FLUSH);
					if (ret != Z_OK && ret != Z_STREAM_END)
					{
						return {};
					}

					buffer.insert(buffer.end(), dest, dest + sizeof(dest) - stream.avail_out);
				}
				while (stream.avail_out == 0);
			}
			while (ret != Z_STREAM_END);

			return buffer;
		}

		std::string compress(const std::string& data)
		{
			std::string result{};
			auto length = compressBound(static_cast<uLong>(data.size()));
			result.resize(length);

			if (compress2(reinterpret_cast<Bytef*>(result.data()), &length,
			              reinterpret_cast<const Bytef*>(data.data()), static_cast<uLong>(data.size()),
			              Z_BEST_COMPRESSION) != Z_OK)
			{
				return {};
			}

			result.resize(length);
			return result;
		}
	}

	namespace zip
	{
		namespace
		{
			bool add_file(zipFile& zip_file, const std::string& filename, const std::string& data)
			{
				const auto zip_64 = data.size() > 0xffffffff ? 1 : 0;
				if (ZIP_OK != zipOpenNewFileInZip64(zip_file, filename.c_str(), nullptr, nullptr, 0, nullptr, 0, nullptr,
				                                    Z_DEFLATED, Z_BEST_COMPRESSION, zip_64))
				{
					return false;
				}

				const auto _ = gsl::finally([&zip_file]() -> void
				{
					zipCloseFileInZip(zip_file);
				});

				return ZIP_OK == zipWriteInFileInZip(zip_file, data.c_str(), static_cast<unsigned>(data.size()));
			}
		}

		void archive::add(const std::string& filename, const std::string& data)
		{
			this->files_[filename] = data;
		}

		bool archive::write(const std::string& filename, const std::string& comment)
		{
			// Hack to create the directory :3
			io::write_file(filename, {});
			io::remove_file(filename);

			auto* zip_file = zipOpen64(filename.c_str(), false);
			if (!zip_file)
			{
				return false;
			}

			const auto _ = gsl::finally([&zip_file, &comment]() -> void
			{
				zipClose(zip_file, comment.empty() ? nullptr : comment.c_str());
			});

			for (const auto& file : this->files_)
			{
				if (!add_file(zip_file, file.first, file.second))
				{
					return false;
				}
			}

			return true;
		}

		// I apologize for writing such a huge function
		void archive::decompress(const std::string& filename, const std::filesystem::path& out_dir)
		{
			unzFile file = unzOpen(filename.c_str());
			if (!file)
			{
				throw std::runtime_error(string::va("unzOpen failed on %s", filename.c_str()));
			}

			unz_global_info global_info;
			if (unzGetGlobalInfo(file, &global_info) != UNZ_OK)
			{
				unzClose(file);
				throw std::runtime_error(string::va("unzGetGlobalInfo failed on %s", filename.c_str()));
			}

			constexpr std::size_t READ_BUFFER_SIZE = 65336;
			const auto read_buffer_large = std::make_unique<char[]>(READ_BUFFER_SIZE);
			// No need to memset this to 0
			auto* read_buffer = read_buffer_large.get();

			// Loop to extract all the files
			for (uLong i = 0; i < global_info.number_entry; ++i)
			{
				// Get info about the current file.
				unz_file_info file_info;
				char out_file[MAX_PATH]{};

				if (unzGetCurrentFileInfo(file, &file_info, out_file, sizeof(out_file) - 1,
				                          nullptr, 0, nullptr, 0) != UNZ_OK)
				{
					continue;
				}

				// Check if this entry is a directory or a file.
				const auto filename_length = std::strlen(out_file);
				if (out_file[filename_length - 1] == '/' || out_file[filename_length - 1] == '\\') // ZIP is not directory-separator-agnostic
				{
					// Entry is a directory. Create it.
					const auto dir = out_dir / out_file;
					io::create_directory(dir);
				}
				else
				{
					// Entry is a file. Extract it.
					if (unzOpenCurrentFile(file) != UNZ_OK)
					{
						// Could not read file from the ZIP
						throw std::runtime_error(string::va("Failed to read file \"%s\" from \"%s\"", out_file, filename.c_str()));
					}

					// Open a stream to write out the data.
					const auto path = out_dir / out_file;
					std::ofstream out(path.string(), std::ios::binary | std::ios::trunc);
					if (!out.is_open())
					{
						throw std::runtime_error("Failed to open stream");
					}

					auto read_bytes = UNZ_OK;
					while (true)
					{
						read_bytes = unzReadCurrentFile(file, read_buffer, READ_BUFFER_SIZE);
						if (read_bytes < 0)
						{
							throw std::runtime_error(string::va("Error while reading \"%s\" from the archive", out_file));
						}

						if (read_bytes > 0)
						{
							out.write(read_buffer, read_bytes);
						}
						else
						{
							// No more data to read, the loop will break
							// This is normal behaviour
							break;
						}
					}

					out.close();
				}

				// Go the the next entry listed in the ZIP file.
				if ((i + 1) < global_info.number_entry)
				{
					if (unzGoToNextFile(file) != UNZ_OK)
					{
						break;
					}
				}
			}

			unzClose(file);
		}
	}
}
