--
BUILD_DIR = path.getabsolute("build")
SRC_DIR = path.getabsolute("./src")

newoption {
	trigger     = "profile",
	description = "Enable profiling",
}

local PROFILE_DEFINE = {}
if _OPTIONS["profile"] then
	print("Profiling enabled")
	PROFILE_DEFINE = "TRACY_ENABLE"
end

solution "GameServer"
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
		targetsuffix "_dbg"
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
		"EnableAVX",
		"EnableAVX2",
		--"StaticRuntime",
	}

	-- erternal dependencies
	dofile("external/genie_zlib.lua");
	dofile("external/genie_eastl.lua");
	dofile("external/genie_tinyxml2.lua");
	dofile("external/genie_tracy.lua");
	dofile("external/genie_glm.lua");
	dofile("external/genie_sokol.lua");
	dofile("external/genie_imgui.lua");
	

project "Login"
	kind "ConsoleApp"
	targetname "login_srv"

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
	targetname "game_srv"

	configuration {}

	includedirs {
		"src",
		zlib_includedir,
		eastl_includedir,
		eathread_includedir,
		eabase_includedir,
		eastdc_includedir,
		tinyxml2_includedir,
		tracy_includedir,
		glm_includedir
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
		tinyxml2_files,
		tracy_files,
		glm_files
	}

	defines {
		"GLM_FORCE_XYZW_ONLY"
	}

	configuration "Release"
		defines {
			PROFILE_DEFINE
		}

	configuration "windows"
		buildoptions{ "/W2" }
		linkoptions{ "/NATVIS:" .. path.getabsolute("external/EASTL-3.16.07/doc/EASTL.natvis")}

		links {
			"ws2_32",
			"user32",
			"advapi32"
		}

		files {
			sokol_files,
			imgui_files
		}

		includedirs {
			sokol_includedir,
			imgui_includedir
		}

	configuration "linux"
		links {
			"pthread",
			"dl",
		}

		-- technically can be run on linux with a few modifications, this is mostly so the deployed server does not get it
		-- TODO: make a "production" build
		excludes {
			"src/game/debug/*"
		}


dofile("tools/genie_tools.lua");