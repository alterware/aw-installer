curl = {
	source = path.join(dependencies.basePath, "curl"),
}

function curl.import()
	links { "curl" }
	
	filter "system:windows"
		links { "Crypt32.lib" }
	filter {}
	
	curl.includes()
end

function curl.includes()
filter "system:windows"
	includedirs {
		path.join(curl.source, "include"),
	}

	defines {
		"CURL_STRICTER",
		"CURL_STATICLIB",
		"CURL_DISABLE_LDAP",
	}
filter {}
end

function curl.project()
	if not os.istarget("windows") then
		return
	end

	project "curl"
		language "C"

		curl.includes()
		
		includedirs {
			path.join(curl.source, "lib"),
		}

		files {
			path.join(curl.source, "lib/**.c"),
			path.join(curl.source, "lib/**.h"),
		}
		
		defines {
			"BUILDING_LIBCURL",
		}
		
		filter "system:windows"

			defines {
				"USE_SCHANNEL",
				"USE_WINDOWS_SSPI",
				"USE_THREADS_WIN32",
			}

		filter {}

		warnings "Off"
		kind "StaticLib"
end

table.insert(dependencies, curl)
