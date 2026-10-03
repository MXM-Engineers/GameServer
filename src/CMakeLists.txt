# Server executables (port of the genie.lua server projects).
# Sources are globbed recursively per target exactly like genie's `**.h/.c/.cpp`
# file lists, so per-target defines (e.g. login without glm/tracy) stay identical.

file(GLOB_RECURSE COMMON_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/common/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/common/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp)
file(GLOB_RECURSE MXM_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/mxm/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/mxm/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/mxm/*.cpp)
file(GLOB_RECURSE LOGIN_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/servers/login/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/servers/login/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/servers/login/*.cpp)
file(GLOB_RECURSE HUB_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/servers/hub/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/servers/hub/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/servers/hub/*.cpp)
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/servers/game/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/servers/game/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/servers/game/*.cpp)
file(GLOB_RECURSE MM_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/servers/matchmaker/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/servers/matchmaker/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/servers/matchmaker/*.cpp)

# shared by hub/game/mm (was server_common_* in genie.lua)
set(SERVER_COMMON_INCLUDES
	${SRC_DIR}
	${EXTERNAL_DIR}/zlib-1.2.11
	${EA_INCLUDES}
	${EXTERNAL_DIR}/tinyxml2-8.0.0
	${EXTERNAL_DIR}/tracy-0.8
	${EXTERNAL_DIR}/glm_0.9.9.8)
set(SERVER_COMMON_LINKS zlib eastl eathread eastdc tinyxml2)
set(SERVER_COMMON_LINKS_WIN ws2_32 user32 advapi32)
set(SERVER_COMMON_DEFINES GLM_FORCE_XYZW_ONLY)

set(NATVIS ${EXTERNAL_DIR}/EASTL-3.16.07/doc/EASTL.natvis)

# ---- Login (login_srv) ----
add_executable(login_srv ${COMMON_SOURCES} ${LOGIN_SOURCES})
if(WIN32)
	target_sources(login_srv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/servers/login/login_rc.rc)
endif()
target_include_directories(login_srv PRIVATE ${SRC_DIR} ${EA_INCLUDES})
target_link_libraries(login_srv PRIVATE eastl eathread eastdc ws2_32)
if(MSVC)
	target_sources(login_srv PRIVATE ${NATVIS})
endif()
mxm_debug_suffix(login_srv)

# ---- HubServer (hub_srv) ----
add_executable(hub_srv
	${COMMON_SOURCES} ${MXM_SOURCES} ${HUB_SOURCES}
	${EXTERNAL_DIR}/tinyxml2-8.0.0/tinyxml2.cpp
	${EXTERNAL_DIR}/tracy-0.8/TracyClient.cpp)
if(WIN32)
	target_sources(hub_srv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/servers/hub/hub_rc.rc)
endif()
target_include_directories(hub_srv PRIVATE ${SERVER_COMMON_INCLUDES})
target_link_libraries(hub_srv PRIVATE ${SERVER_COMMON_LINKS} ${SERVER_COMMON_LINKS_WIN})
target_compile_definitions(hub_srv PRIVATE ${SERVER_COMMON_DEFINES})
if(MSVC)
	target_sources(hub_srv PRIVATE ${NATVIS})
endif()
add_dependencies(hub_srv sync_gamedata)
mxm_debug_suffix(hub_srv)

# ---- GameServer (game_srv) ----
add_executable(game_srv
	${COMMON_SOURCES} ${MXM_SOURCES} ${GAME_SOURCES}
	${EXTERNAL_DIR}/tinyxml2-8.0.0/tinyxml2.cpp
	${EXTERNAL_DIR}/tracy-0.8/TracyClient.cpp)
if(WIN32)
	target_sources(game_srv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/servers/game/game_rc.rc)
endif()
target_include_directories(game_srv PRIVATE
	${SERVER_COMMON_INCLUDES}
	${EXTERNAL_DIR}/sokol
	${EXTERNAL_DIR}/imgui-1.81
	${CMAKE_CURRENT_SOURCE_DIR}/servers/game)
target_link_libraries(game_srv PRIVATE
	${SERVER_COMMON_LINKS} ${SERVER_COMMON_LINKS_WIN} imgui sokol physx)
target_compile_definitions(game_srv PRIVATE ${SERVER_COMMON_DEFINES})
if(MSVC)
	target_sources(game_srv PRIVATE ${NATVIS})
endif()
copy_physx_dlls(game_srv)
add_dependencies(game_srv sync_gamedata)
mxm_debug_suffix(game_srv)

if(BUILD_TESTING)
	add_executable(physics_tests
		${COMMON_SOURCES} ${MXM_SOURCES}
		${CMAKE_CURRENT_SOURCE_DIR}/servers/game/physics.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/servers/game/world.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/servers/game/replication.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/servers/game/config.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/physics_tests.cpp
		${EXTERNAL_DIR}/tinyxml2-8.0.0/tinyxml2.cpp
		${EXTERNAL_DIR}/tracy-0.8/TracyClient.cpp)
	target_include_directories(physics_tests PRIVATE
		${SERVER_COMMON_INCLUDES}
		${CMAKE_CURRENT_SOURCE_DIR}/servers/game)
	target_link_libraries(physics_tests PRIVATE
		${SERVER_COMMON_LINKS} ${SERVER_COMMON_LINKS_WIN} physx)
	target_compile_definitions(physics_tests PRIVATE ${SERVER_COMMON_DEFINES})
	mxm_debug_suffix(physics_tests)
	copy_physx_dlls(physics_tests)
	add_dependencies(physics_tests sync_gamedata)
	add_test(NAME physics_tests COMMAND physics_tests)
	set_tests_properties(physics_tests PROPERTIES
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
	add_executable(network_tests
		${COMMON_SOURCES}
		${CMAKE_CURRENT_SOURCE_DIR}/tests/network_tests.cpp
		${EXTERNAL_DIR}/tracy-0.8/TracyClient.cpp)
	target_include_directories(network_tests PRIVATE ${SERVER_COMMON_INCLUDES})
	target_link_libraries(network_tests PRIVATE ${SERVER_COMMON_LINKS} ${SERVER_COMMON_LINKS_WIN})
	target_compile_definitions(network_tests PRIVATE ${SERVER_COMMON_DEFINES})
	mxm_debug_suffix(network_tests)
	add_test(NAME network_tests COMMAND network_tests)
	set_tests_properties(network_tests PROPERTIES
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
endif()

# ---- MatchMaker (mm_srv) ----
add_executable(mm_srv
	${COMMON_SOURCES} ${MXM_SOURCES} ${MM_SOURCES}
	${EXTERNAL_DIR}/tinyxml2-8.0.0/tinyxml2.cpp
	${EXTERNAL_DIR}/tracy-0.8/TracyClient.cpp)
if(WIN32)
	target_sources(mm_srv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/servers/matchmaker/mm_rc.rc)
endif()
target_include_directories(mm_srv PRIVATE
	${SERVER_COMMON_INCLUDES}
	${CMAKE_CURRENT_SOURCE_DIR}/servers/matchmaker)
target_link_libraries(mm_srv PRIVATE ${SERVER_COMMON_LINKS} ${SERVER_COMMON_LINKS_WIN})
target_compile_definitions(mm_srv PRIVATE ${SERVER_COMMON_DEFINES})
if(MSVC)
	target_sources(mm_srv PRIVATE ${NATVIS})
endif()
mxm_debug_suffix(mm_srv)

# ---- Profiling (was: genie --profile -> TRACY_ENABLE on Release) ----
if(PROFILE)
	foreach(srv hub_srv game_srv mm_srv)
		target_compile_definitions(${srv} PRIVATE "$<$<NOT:$<CONFIG:Debug>>:TRACY_ENABLE>")
	endforeach()
endif()
