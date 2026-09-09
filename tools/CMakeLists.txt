# Tools (port of tools/genie_tools.lua)

# genie compiled exactly these four common sources into every tool
set(TOOL_COMMON_SOURCES
	${SRC_DIR}/common/base.cpp
	${SRC_DIR}/common/logger.cpp
	${SRC_DIR}/common/platform_windows.cpp
	${SRC_DIR}/common/platform_linux.cpp)

set(TOOL_COMMON_INCLUDES ${SRC_DIR} ${EA_INCLUDES})
set(TOOL_COMMON_LINKS eastl eathread eastdc)

# ---- Lea (test_lea) ----
add_executable(test_lea ${TOOL_COMMON_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/lea/lea.cpp)
target_include_directories(test_lea PRIVATE ${TOOL_COMMON_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/lea)
target_link_libraries(test_lea PRIVATE ${TOOL_COMMON_LINKS})
mxm_debug_suffix(test_lea)

# ---- LeaDll (lea.dll) ----
add_library(lea SHARED ${TOOL_COMMON_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/lea/lea.cpp)
target_include_directories(lea PRIVATE ${TOOL_COMMON_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/lea)
target_link_libraries(lea PRIVATE ${TOOL_COMMON_LINKS})
target_compile_definitions(lea PRIVATE CONF_DLL)
# the packet-mining scripts expect the built dll at scripts/lea.dll
add_custom_command(TARGET lea POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:lea>
		${PROJECT_SOURCE_DIR}/scripts/lea.dll)
mxm_debug_suffix(lea)

# ---- NavMesh (navmesh) ----
add_executable(navmesh ${TOOL_COMMON_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/navmesh/navmesh.cpp)
target_include_directories(navmesh PRIVATE ${TOOL_COMMON_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/navmesh)
target_link_libraries(navmesh PRIVATE ${TOOL_COMMON_LINKS})
mxm_debug_suffix(navmesh)

# ---- ToolCollision (col) ----
add_executable(col ${TOOL_COMMON_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/collision/tool_collision.cpp)
target_include_directories(col PRIVATE
	${TOOL_COMMON_INCLUDES}
	${CMAKE_CURRENT_SOURCE_DIR}/collision)
target_link_libraries(col PRIVATE ${TOOL_COMMON_LINKS} physx)
copy_physx_dlls(col)
mxm_debug_suffix(col)
