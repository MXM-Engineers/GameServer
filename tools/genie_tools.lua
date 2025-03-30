--
local common_includes = {
	SRC_DIR,
	eastl_includedir,
	eathread_includedir,
	eabase_includedir,
	eastdc_includedir,
}

local common_files = {
	SRC_DIR .. "/common/base.cpp",
	SRC_DIR .. "/common/logger.cpp",
	SRC_DIR .. "/common/platform_windows.cpp",
	SRC_DIR .. "/common/platform_linux.cpp",
}

local common_links = {
	"eastl",
	"eathread",
	"eastdc",
}

project "Lea"
	kind "ConsoleApp"
	targetname "test_lea"

	configuration {}

	includedirs {
		common_includes,
		"lea",
	}

	links {
		common_links
	}
	
	files {
		common_files,
		"lea/**.h",
		"lea/**.c",
		"lea/**.cpp",
	}

project "LeaDll"
	kind "SharedLib"
	targetname "lea"

	configuration {}

	includedirs {
		common_includes,
		"lea",
	}

	links {
		common_links
	}
	
	files {
		common_files,
		"lea/**.h",
		"lea/**.c",
		"lea/**.cpp",
	}

	defines {
		"CONF_DLL"
	}

project "NavMesh"
	kind "ConsoleApp"
	targetname "navmesh"

	configuration {}

	includedirs {
		common_includes,
		"navmesh",
	}

	links {
		common_links
	}
	
	files {
		common_files,
		"navmesh/**.h",
		"navmesh/**.c",
		"navmesh/**.cpp",
	}

project "ToolCollision"
	kind "ConsoleApp"
	targetname "col"

	configuration {}

	includedirs {
		common_includes,
		"collision",
		physx_includedir,
	}

	links {
		common_links,
		physx_libs_win64
	}
	
	files {
		common_files,
		"collision/**.h",
		"collision/**.c",
		"collision/**.cpp",
	}

	configuration "Release"
		libdirs {
			physx_libdir_release
		}

	configuration "Debug"
		libdirs {
			physx_libdir_debug
		}