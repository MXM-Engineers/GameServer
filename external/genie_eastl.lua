-- eastl

SRC_DIR = "EASTL-3.16.07/source"
eastl_includedir = path.getabsolute("EASTL-3.16.07/include")
eabase_includedir = path.getabsolute("EABase-2.09.05/include/Common")

project "eastl"
	kind "StaticLib"
	
	includedirs {
		eastl_includedir,
		eabase_includedir
	}

    files {
		SRC_DIR .. "/*.cpp",
	}

	defines {
		"_CHAR16T",
		"_CRT_SECURE_NO_WARNINGS",
		"_SCL_SECURE_NO_WARNINGS",
		"EASTL_OPENSOURCE=1",
	}


    