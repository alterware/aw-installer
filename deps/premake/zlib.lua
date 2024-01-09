zlib = {
	source = path.join(dependencies.basePath, "zlib"),
}

function zlib.import()
	links { "zlib" }
	zlib.includes()
end

function zlib.includes()
	includedirs {
		zlib.source
	}

	defines {
		"ZLIB_CONST",
	}
end

function zlib.project()
	project "zlib"
		language "C"
		cdialect "C89"

		zlib.includes()

		files {
			path.join(zlib.source, "*.h"),
			path.join(zlib.source, "*.c"),
		}

		filter { "system:windows" }
			defines "_CRT_SECURE_NO_DEPRECATE"
		filter {}

		warnings "Off"
		kind "StaticLib"
end

table.insert(dependencies, zlib)
