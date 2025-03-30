-- eastl

eastl_includedir = path.getabsolute("EASTL-3.16.07/include")
eathread_includedir = path.getabsolute("EAThread/include")
eabase_includedir = path.getabsolute("EABase-2.09.05/include/Common")
eaassert_includedir = path.getabsolute("EAAssert/include")
eastdc_includedir = path.getabsolute("EAStdC-1.26.03/include")

project "eaassert"
	kind "StaticLib"
	
	includedirs {
		eabase_includedir,
		eaassert_includedir
	}

    files {
		"EAAssert/source/*.cpp",
	}

	defines {
		"_CHAR16T",
	}

project "eathread"
	kind "StaticLib"
	
	includedirs {
		eathread_includedir,
		eabase_includedir
	}

    files {
		"EAThread/source/*.cpp",
	}

	defines {
		"_CHAR16T",
		"_CRT_SECURE_NO_WARNINGS",
	}

project "eastdc"
	kind "StaticLib"
	
	includedirs {
		eastdc_includedir,
		eabase_includedir,
		eathread_includedir,
		eaassert_includedir
	}

    files {
		"EAStdC-1.26.03/source/*.cpp",
	}

	defines {
		"_CHAR16T",
		"_CRT_SECURE_NO_WARNINGS",
		"_SCL_SECURE_NO_WARNINGS",
	}

project "eastl"
	kind "StaticLib"
	
	includedirs {
		eastl_includedir,
		eabase_includedir,
		eastdc_includedir
	}

    files {
		"EASTL-3.16.07/source/*.cpp",
	}

	defines {
		"_CHAR16T",
		"_CRT_SECURE_NO_WARNINGS",
		"_SCL_SECURE_NO_WARNINGS",
		"EASTL_OPENSOURCE=1",
	}

	configuration "vs2022"
        defines {
            "_HAS_CXX17=0",  -- VS2022 defaults to newer C++ standards
            "_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING",
            "EASTL_CUSTOM_FLOAT_CONSTANTS_REQUIRED=1"
        }

    