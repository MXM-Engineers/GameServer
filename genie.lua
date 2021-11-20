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

local natvis = "/NATVIS:" .. path.getabsolute("external/EASTL-3.16.07/doc/EASTL.natvis")

solution "PrivateMxM"
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
	dofile("external/genie_physx.lua");


-- Tools
dofile("tools/genie_tools.lua")

-- Servers
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
		"src/servers/login/**.h",
		"src/servers/login/**.c",
		"src/servers/login/**.cpp",
		"src/servers/login/**.rc",
	}

	configuration "windows"
		buildoptions{ "/W2" }
		linkoptions{ natvis}
		links {
			"ws2_32",
		}
	configuration "linux"
		links {
			"pthread",
		}

local server_common_includedirs = {
	"src",
	"src/servers/common",
	zlib_includedir,
	eastl_includedir,
	eathread_includedir,
	eabase_includedir,
	eastdc_includedir,
	tinyxml2_includedir,
	tracy_includedir,
	glm_includedir
}

local server_common_links = {
	"zlib",
	"eastl",
	"eathread",
	"eastdc",
}

local server_common_links_win = {
	"ws2_32",
	"user32",
	"advapi32"
}

local server_common_links_linux = {
	"pthread",
	"dl",
}

local server_common_files = {
	"src/common/**.h",
	"src/common/**.c",
	"src/common/**.cpp",
	"src/mxm/**.h",
	"src/mxm/**.c",
	"src/mxm/**.cpp",
	tinyxml2_files,
	tracy_files,
	glm_files
}

local server_common_defines = {
	"GLM_FORCE_XYZW_ONLY"
}

project "HubServer"
	kind "ConsoleApp"
	targetname "hub_srv"

	configuration {}

	includedirs {
		server_common_includedirs
	}

	links {
		server_common_links
	}
	
	files {
		server_common_files,
		"src/servers/hub/**.h",
		"src/servers/hub/**.c",
		"src/servers/hub/**.cpp",
		"src/servers/hub/**.rc",
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
		linkoptions{ natvis }

		links {
			server_common_links_win
		}

	configuration "linux"
		links {
			server_common_links_linux
		}

project "GameServer"
	kind "ConsoleApp"
	targetname "game_srv"

	configuration {}

	includedirs {
		server_common_includedirs,
		physx_includedir,
		"src/servers/play",
	}

	links {
		server_common_links,
		physx_libs_win64
	}
	
	files {
		server_common_files,
		"src/servers/play/**.h",
		"src/servers/play/**.c",
		"src/servers/play/**.cpp",
		"src/servers/play/**.rc",
	}

	defines {
		server_common_defines
	}

	configuration "Release"
		defines {
			PROFILE_DEFINE
		}

		libdirs {
			physx_libdir_release
		}

	configuration "Debug"
		libdirs {
			physx_libdir_debug
		}

	configuration "windows"
		buildoptions{ "/W2" }
		linkoptions{ natvis }

		links {
			server_common_links_win
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
			server_common_links_linux
		}

		-- technically can be run on linux with a few modifications, this is mostly so the deployed server does not get it
		-- TODO: make a "production" build
		excludes {
			"src/game/debug/*"
		}

project "MatchMaker"
	kind "ConsoleApp"
	targetname "mm_srv"

	configuration {}

	includedirs {
		server_common_includedirs,
		"src/servers/matchmaker",
	}

	links {
		server_common_links
	}
	
	files {
		server_common_files,
		"src/servers/matchmaker/**.h",
		"src/servers/matchmaker/**.c",
		"src/servers/matchmaker/**.cpp",
		"src/servers/matchmaker/**.rc",
	}

	defines {
		server_common_defines
	}

	configuration "Release"
		defines {
			PROFILE_DEFINE
		}

	configuration "windows"
		buildoptions{ "/W2" }
		linkoptions{ natvis }

		links {
			server_common_links_win
		}

	configuration "linux"
		links {
			server_common_links_linux
		}