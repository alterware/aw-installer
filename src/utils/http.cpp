#include <std_include.hpp>

#include "http.hpp"
#include <curl/curl.h>

namespace utils::http
{
	namespace
	{
		size_t write_callback(void* contents, const size_t size, const size_t nmemb, void* userp)
		{
			static_cast<std::string*>(userp)->append(static_cast<char*>(contents), size * nmemb);
			return size * nmemb;
		}
	}

	std::optional<std::string> get_data(const std::string& url, const headers& headers)
	{
		curl_slist* header_list = nullptr;
		auto* curl = curl_easy_init();
		if (!curl)
		{
			return {};
		}

		auto _ = gsl::finally([&]()
		{
			curl_slist_free_all(header_list);
			curl_easy_cleanup(curl);
		});

		
		for(const auto& header : headers)
		{
			auto data = header.first + ": "s + header.second;
			header_list = curl_slist_append(header_list, data.c_str());
		}
		
		std::string buffer{};
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "aw-installer/1.0");
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

		if (curl_easy_perform(curl) == CURLE_OK)
		{
			return {std::move(buffer)};
		}

		return {};
	}

	std::future<std::optional<std::string>> get_data_async(const std::string& url, const headers& headers)
	{
		return std::async(std::launch::async, [url, headers]() -> std::optional<std::string>
		{
			return get_data(url, headers);
		});
	}
}
