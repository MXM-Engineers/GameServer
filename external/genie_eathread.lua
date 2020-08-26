-- eathread

SRC_DIR = "EAThread/source"
eathread_includedir = path.getabsolute("EAThread/include")
eabase_includedir = path.getabsolute("EABase-2.09.05/include/Common")

project "eathread"
	kind "StaticLib"
	
	includedirs {
		eathread_includedir,
		eabase_includedir
	}

    files {
		SRC_DIR .. "/*.cpp",
	}

	defines {
		"_CHAR16T",
		"_CRT_SECURE_NO_WARNINGS",
	}


    