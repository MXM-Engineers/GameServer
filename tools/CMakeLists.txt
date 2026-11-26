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

# ---- Accounts (test_accounts) ----
add_executable(test_accounts ${TOOL_COMMON_SOURCES}
	${SRC_DIR}/storage/database.cpp
	${SRC_DIR}/storage/account_db.cpp
	${SRC_DIR}/storage/game_db.cpp
	${SRC_DIR}/storage/game_store.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/accounts/test_accounts.cpp)
target_include_directories(test_accounts PRIVATE ${TOOL_COMMON_INCLUDES})
target_link_libraries(test_accounts PRIVATE ${TOOL_COMMON_LINKS} sqlite3 argon2)
if(NOT WIN32)
	target_link_libraries(test_accounts PRIVATE pthread)
endif()
mxm_debug_suffix(test_accounts)

# ---- GameStore (test_game_store) ----
add_executable(test_game_store ${TOOL_COMMON_SOURCES}
	${SRC_DIR}/storage/database.cpp
	${SRC_DIR}/storage/game_db.cpp
	${SRC_DIR}/storage/game_store.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/game_store/test_game_store.cpp)
target_include_directories(test_game_store PRIVATE ${TOOL_COMMON_INCLUDES})
target_link_libraries(test_game_store PRIVATE ${TOOL_COMMON_LINKS} sqlite3)
if(NOT WIN32)
	target_link_libraries(test_game_store PRIVATE pthread)
endif()
mxm_debug_suffix(test_game_store)

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
if(MXM_HAVE_PHYSX)
add_executable(col ${TOOL_COMMON_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/collision/tool_collision.cpp)
target_include_directories(col PRIVATE
	${TOOL_COMMON_INCLUDES}
	${CMAKE_CURRENT_SOURCE_DIR}/collision)
target_link_libraries(col PRIVATE ${TOOL_COMMON_LINKS} physx)
copy_physx_dlls(col)
mxm_debug_suffix(col)
endif()
