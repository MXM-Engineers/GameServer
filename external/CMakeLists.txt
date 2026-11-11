# External dependencies (port of external/genie_*.lua)

set(PHYSX_DIR ${EXTERNAL_DIR}/physx-4.1)

# ---- zlib ----
file(GLOB ZLIB_SOURCES CONFIGURE_DEPENDS ${EXTERNAL_DIR}/zlib-1.2.11/*.c)
add_library(zlib STATIC ${ZLIB_SOURCES})
target_include_directories(zlib PUBLIC ${EXTERNAL_DIR}/zlib-1.2.11)
if(MSVC)
	target_compile_options(zlib PRIVATE /W0)
else()
	# zlib only includes <unistd.h> when its configure script says so; without
	# it gz*.c implicitly declares read/write/lseek/close, an error from GCC 14 on.
	target_compile_definitions(zlib PRIVATE Z_HAVE_UNISTD_H)
endif()

# ---- EA foundation libraries ----

add_library(eaassert STATIC ${EXTERNAL_DIR}/EAAssert/source/eaassert.cpp)
target_include_directories(eaassert PUBLIC
	${EXTERNAL_DIR}/EABase-2.09.05/include/Common
	${EXTERNAL_DIR}/EAAssert/include)
target_compile_definitions(eaassert PRIVATE _CHAR16T)

file(GLOB EATHREAD_SOURCES CONFIGURE_DEPENDS ${EXTERNAL_DIR}/EAThread/source/*.cpp)
add_library(eathread STATIC ${EATHREAD_SOURCES})
target_include_directories(eathread PUBLIC
	${EXTERNAL_DIR}/EAThread/include
	${EXTERNAL_DIR}/EABase-2.09.05/include/Common)
target_compile_definitions(eathread PRIVATE _CHAR16T _CRT_SECURE_NO_WARNINGS)

file(GLOB EASTDC_SOURCES CONFIGURE_DEPENDS ${EXTERNAL_DIR}/EAStdC-1.26.03/source/*.cpp)
add_library(eastdc STATIC ${EASTDC_SOURCES})
target_include_directories(eastdc PUBLIC
	${EXTERNAL_DIR}/EAStdC-1.26.03/include
	${EXTERNAL_DIR}/EABase-2.09.05/include/Common
	${EXTERNAL_DIR}/EAThread/include
	${EXTERNAL_DIR}/EAAssert/include)
target_compile_definitions(eastdc PRIVATE _CHAR16T _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS)
# EAStdC sources use EA_ASSERT, which is live in Debug builds (_DEBUG -> EA_DEBUG)
target_link_libraries(eastdc PUBLIC eaassert)

file(GLOB EASTL_SOURCES CONFIGURE_DEPENDS ${EXTERNAL_DIR}/EASTL-3.16.07/source/*.cpp)
add_library(eastl STATIC ${EASTL_SOURCES})
target_include_directories(eastl PUBLIC
	${EXTERNAL_DIR}/EASTL-3.16.07/include
	${EXTERNAL_DIR}/EABase-2.09.05/include/Common
	${EXTERNAL_DIR}/EAStdC-1.26.03/include)
target_compile_definitions(eastl PRIVATE
	_CHAR16T
	_CRT_SECURE_NO_WARNINGS
	_SCL_SECURE_NO_WARNINGS
	EASTL_OPENSOURCE=1)
target_link_libraries(eastl PUBLIC eastdc)

if(NOT WIN32)
	# the lea tool is a shared library, so everything it pulls in must be PIC
	set_target_properties(eaassert eathread eastdc eastl PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# ---- tinyxml2 (compiled into the server targets, as genie did) ----
add_library(tinyxml2 INTERFACE)
target_include_directories(tinyxml2 INTERFACE ${EXTERNAL_DIR}/tinyxml2-8.0.0)

# ---- sokol (header-only; impls compiled in window.cpp) ----
add_library(sokol INTERFACE)
target_include_directories(sokol INTERFACE ${EXTERNAL_DIR}/sokol)

# ---- imgui (compiled into game_srv, as genie did) ----
file(GLOB IMGUI_SOURCES CONFIGURE_DEPENDS ${EXTERNAL_DIR}/imgui-1.81/*.cpp)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${EXTERNAL_DIR}/imgui-1.81)

# ---- PhysX ----
if(MXM_HAVE_PHYSX)
add_library(physx INTERFACE)
target_include_directories(physx INTERFACE
	${PHYSX_DIR}/include/physx
	${PHYSX_DIR}/include/pxshared)
if(WIN32)
	target_link_directories(physx INTERFACE
		"$<$<CONFIG:Debug>:${PHYSX_DIR}/lib/debug>"
		"$<$<CONFIG:Debug>:${PHYSX_DIR}/lib/checked>"
		"$<$<NOT:$<CONFIG:Debug>>:${PHYSX_DIR}/lib/release>"
	)
	target_link_libraries(physx INTERFACE
		PhysXExtensions_static_64
		PhysX_64
		PhysXPvdSDK_static_64
		PhysXCommon_64
		PhysXFoundation_64
		PhysXCooking_64
		PhysXCharacterKinematic_static_64)
else()
	# PhysX 4.1 builds every module static on Linux and they reference each
	# other, so they have to be resolved as a single link group.
	target_link_directories(physx INTERFACE ${PHYSX_LINUX_LIB_DIR})
	target_link_libraries(physx INTERFACE
		-Wl,--start-group
		PhysXExtensions_static_64
		PhysX_static_64
		PhysXPvdSDK_static_64
		PhysXCommon_static_64
		PhysXFoundation_static_64
		PhysXCooking_static_64
		PhysXCharacterKinematic_static_64
		-Wl,--end-group)
endif()
endif()

# copy_physx_dlls() is defined in the root CMakeLists.txt
