-- zlib

SRC_DIR = "zlib-1.2.11"
zlib_includedir = path.getabsolute(SRC_DIR)

project "zlib"
    kind "StaticLib"

    files {
		SRC_DIR .. "/*.h",
		SRC_DIR .. "/*.c",
	}
	
	configuration "windows"
		buildoptions{ "/W0" }


    