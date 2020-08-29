--
BUILD_DIR = path.getabsolute("build")

solution "Servers"
	location(BUILD_DIR)

	configurations {
		"Debug",
		"Release"
	}

	platforms {
		"x32",
		"x64"
	}

	language "C++"

	configuration {"Debug"}
		targetsuffix "_debug"
		flags {
			"Symbols",
			"FullSymbols"
		}
		defines {
			"_DEBUG",
			"CONF_DEBUG"
		}

	configuration {"Release"}
		flags {
			"Optimize",
			"Symbols",
			"NoBufferSecurityCheck"
		}
		defines {
			"NDEBUG",
			"CONF_RELEASE",
		}

	configuration "windows"
		defines {
			"_CRT_SECURE_NO_DEPRECATE",
			"_CRT_NONSTDC_NO_DEPRECATE",
			"WIN32_LEAN_AND_MEAN",
			"NOMINMAX",
		}

		-- disable exception related warnings
		buildoptions{ "/wd4577", "/wd4530" }

	configuration {}
		flags {
			"Cpp14"
		}

	targetdir(BUILD_DIR)

	configuration { "qbs" } -- TODO: fix target dir handling for genie qbs and PR it
		targetdir("../build")
	configuration {}

	flags {
		"NoExceptions",
		"NoRTTI",
		"EnableSSE",
		"EnableSSE2",
		"EnableAVX",
		"EnableAVX2",
		"EnableMinimalRebuild",
		--"StaticRuntime",
	}

	-- erternal dependencies
	dofile("external/genie_zlib.lua");
	dofile("external/genie_eastl.lua");
	dofile("external/genie_eathread.lua");
	dofile("external/genie_tinyxml2.lua");
	

project "Login"
	kind "ConsoleApp"

	configuration {}

	links {
		"eastl",
		"eathread"
	}

	includedirs {
		"src",
		eastl_includedir,
		eabase_includedir,
		eathread_includedir
	}
	
	files {
		"src/common/**.h",
		"src/common/**.c",
		"src/common/**.cpp",
		"src/login/**.h",
		"src/login/**.c",
		"src/login/**.cpp",
	}

	configuration "windows"
		linkoptions{ "/NATVIS:" .. path.getabsolute("external/EASTL-3.16.07/doc/EASTL.natvis")}
		links {
			"ws2_32",
		}

project "Game"
	kind "ConsoleApp"

	configuration {}

	includedirs {
		"src",
		zlib_includedir,
		eastl_includedir,
		eathread_includedir,
		eabase_includedir,
		tinyxml2_includedir
	}

	links {
		"zlib",
		"eastl",
		"eathread",
	}
	
	files {
		"src/common/**.h",
		"src/common/**.c",
		"src/common/**.cpp",
		"src/game/**.h",
		"src/game/**.c",
		"src/game/**.cpp",
		tinyxml2_files
	}

	configuration "windows"
		linkoptions{ "/NATVIS:" .. path.getabsolute("external/EASTL-3.16.07/doc/EASTL.natvis")}

		links {
			"ws2_32",
		}