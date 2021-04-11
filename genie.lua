--
BUILD_DIR = path.getabsolute("build")

newoption {
	trigger     = "profile",
	description = "Enable profiling",
}

local PROFILE_DEFINE = {}
if _OPTIONS["profile"] then
	print("Profiling enabled")
	PROFILE_DEFINE = "TRACY_ENABLE"
end

solution "Servers"
	location(BUILD_DIR)
	targetdir(BUILD_DIR)

	configurations {
		"Debug",
		"Release"
	}

	platforms {
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
			"CONF_WINDOWS",
		}

		-- disable exception related warnings
		buildoptions{ "/wd4577", "/wd4530" }

	configuration "linux"
		defines {
			"CONF_LINUX",
		}
		buildoptions{ "-fshort-wchar" }


	configuration { "qbs" } -- TODO: fix target dir handling for genie qbs and PR it
		targetdir("../build")

	configuration {}
		flags {
			"Cpp14"
		}

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
	dofile("external/genie_tinyxml2.lua");
	dofile("external/genie_tracy.lua");
	

project "Login"
	kind "ConsoleApp"

	configuration {}

	links {
		"eastl",
		"eathread",
		"eastdc",
	}

	includedirs {
		"src",
		eastl_includedir,
		eabase_includedir,
		eastdc_includedir,
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
		buildoptions{ "/W2" }
		linkoptions{ "/NATVIS:" .. path.getabsolute("external/EASTL-3.16.07/doc/EASTL.natvis")}
		links {
			"ws2_32",
		}
	configuration "linux"
		links {
			"pthread",
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
		eastdc_includedir,
		tinyxml2_includedir,
		tracy_includedir
	}

	links {
		"zlib",
		"eastl",
		"eathread",
		"eastdc",
	}
	
	files {
		"src/common/**.h",
		"src/common/**.c",
		"src/common/**.cpp",
		"src/game/**.h",
		"src/game/**.c",
		"src/game/**.cpp",
		"src/game/model/**.h",
		"src/game/model/**.c",
		"src/game/model/**.cpp",
		tinyxml2_files,
		tracy_files
	}

	configuration "Release"
		defines {
			PROFILE_DEFINE
		}

	configuration {}

	configuration "windows"
		buildoptions{ "/W2" }
		linkoptions{ "/NATVIS:" .. path.getabsolute("external/EASTL-3.16.07/doc/EASTL.natvis")}

		links {
			"ws2_32",
			"user32",
			"advapi32"
		}

	configuration "linux"
		links {
			"pthread",
			"dl",
		}