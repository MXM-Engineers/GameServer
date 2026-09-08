cmake_minimum_required(VERSION 3.26)

project(PrivateMxM LANGUAGES C CXX)

# Enable Tracy profiling in Release builds (was: genie --profile)
option(PROFILE "Enable Tracy profiling in Release builds" OFF)

set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# EA library include roots (shared by src/ and tools/)
set(EA_INCLUDES
	${EXTERNAL_DIR}/EASTL-3.16.07/include
	${EXTERNAL_DIR}/EAThread/include
	${EXTERNAL_DIR}/EABase-2.09.05/include/Common
	${EXTERNAL_DIR}/EAStdC-1.26.03/include
)

# ---- Global compile settings (mirrors the genie.lua solution block) ----

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
	# drop the /EHsc CMake adds so our /EHs-c- doesn't override it
	string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	# Cpp14 / NoExceptions / NoRTTI / EnableAVX2
	add_compile_options(/arch:AVX2 /EHs-c- /GR- /wd4577 /wd4530)
	# VS2022+ defaults to newer C++ standards; force the old one like genie did
	add_compile_definitions(
		_HAS_CXX17=0
		_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
		EASTL_CUSTOM_FLOAT_CONSTANTS_REQUIRED=1
	)
	# NoBufferSecurityCheck on Release
	add_compile_options("$<$<NOT:$<CONFIG:Debug>>:/GS->")
else()
	add_compile_options(-fno-exceptions -fno-rtti -fshort-wchar)
endif()

if(WIN32)
	add_compile_definitions(
		_CRT_SECURE_NO_DEPRECATE
		_CRT_NONSTDC_NO_DEPRECATE
		WIN32_LEAN_AND_MEAN
		NOMINMAX
		CONF_WINDOWS
	)
else()
	add_compile_definitions(CONF_LINUX)
endif()

add_compile_definitions(
	"$<$<CONFIG:Debug>:_DEBUG>"
	"$<$<CONFIG:Debug>:CONF_DEBUG>"
	"$<$<NOT:$<CONFIG:Debug>>:NDEBUG>"
	"$<$<NOT:$<CONFIG:Debug>>:CONF_RELEASE>"
)

# genie gave Debug targets a "_dbg" suffix; keep the same output names.
# Per-target DEBUG_POSTFIX: the global CMAKE_DEBUG_POSTFIX does not apply to
# executables, and OUTPUT_NAME generator expressions are rejected by CMake 4.x.
function(mxm_debug_suffix target)
	set_target_properties(${target} PROPERTIES DEBUG_POSTFIX "_dbg")
endfunction()

# PhysX ships DLLs next to the import libs; drop them beside the binaries.
set(PHYSX_DIR ${EXTERNAL_DIR}/physx-4.1)
set(PHYSX_DLLS PhysXCommon_64.dll PhysXCooking_64.dll PhysXFoundation_64.dll PhysX_64.dll)
function(copy_physx_dlls target)
	foreach(dll IN LISTS PHYSX_DLLS)
		add_custom_command(TARGET ${target} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"${PHYSX_DIR}/lib/$<IF:$<CONFIG:Debug>,checked,release>/${dll}"
				"$<TARGET_FILE_DIR:${target}>/${dll}")
	endforeach()
endfunction()

# Sync gamedata/ into build/gamedata so servers can resolve "gamedata" from
# their CWD (they run from build/). The stamp output lets ninja skip the step
# entirely when nothing changed; copy_directory_if_different only rewrites
# files that actually changed.
set(GAMEDATA_SRC ${CMAKE_SOURCE_DIR}/gamedata)
set(GAMEDATA_DST ${CMAKE_SOURCE_DIR}/build/gamedata)
set(GAMEDATA_STAMP ${CMAKE_BINARY_DIR}/gamedata.synced)
file(GLOB_RECURSE GAMEDATA_FILES CONFIGURE_DEPENDS "${GAMEDATA_SRC}/*")
add_custom_command(
	OUTPUT "${GAMEDATA_STAMP}"
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${GAMEDATA_SRC}" "${GAMEDATA_DST}"
	COMMAND ${CMAKE_COMMAND} -E touch "${GAMEDATA_STAMP}"
	DEPENDS ${GAMEDATA_FILES}
	COMMENT "Syncing gamedata -> build/gamedata")
add_custom_target(sync_gamedata DEPENDS "${GAMEDATA_STAMP}")

# Keep server executables and tools in build/ regardless of config/preset
# (debug targets carry the _dbg postfix, so Debug and Release never collide).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
# Multi-config generators (Visual Studio) append $<CONFIG> unless overridden:
if(CMAKE_CONFIGURATION_TYPES)
	foreach(_cfg ${CMAKE_CONFIGURATION_TYPES})
		string(TOUPPER ${_cfg} _CFG_UPPER)
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_CFG_UPPER} ${CMAKE_SOURCE_DIR}/build)
	endforeach()
endif()

add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(tools)
