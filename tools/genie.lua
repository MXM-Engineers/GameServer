--
BUILD_DIR = path.getabsolute("build")

solution "Tools"
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


project "Lea"
	kind "ConsoleApp"

	configuration {}

	includedirs {
		"lea",
	}
	
	files {
		"lea/**.h",
		"lea/**.c",
		"lea/**.cpp",
	}