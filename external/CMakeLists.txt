# External dependencies (port of external/genie_*.lua)

set(PHYSX_DIR ${EXTERNAL_DIR}/physx-4.1)

# ---- zlib ----
file(GLOB ZLIB_SOURCES CONFIGURE_DEPENDS ${EXTERNAL_DIR}/zlib-1.2.11/*.c)
add_library(zlib STATIC ${ZLIB_SOURCES})
target_include_directories(zlib PUBLIC ${EXTERNAL_DIR}/zlib-1.2.11)
if(MSVC)
	target_compile_options(zlib PRIVATE /W0)
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

set(PHYSX_CCT_DIR ${EXTERNAL_DIR}/physx-cct/physx/source)
set(PHYSX_CCT_SRC ${PHYSX_CCT_DIR}/physxcharacterkinematic/src)
add_library(physx_cct STATIC
	${PHYSX_CCT_SRC}/CctBoxController.cpp
	${PHYSX_CCT_SRC}/CctCapsuleController.cpp
	${PHYSX_CCT_SRC}/CctCharacterController.cpp
	${PHYSX_CCT_SRC}/CctCharacterControllerCallbacks.cpp
	${PHYSX_CCT_SRC}/CctCharacterControllerManager.cpp
	${PHYSX_CCT_SRC}/CctController.cpp
	${PHYSX_CCT_SRC}/CctObstacleContext.cpp
	${PHYSX_CCT_SRC}/CctSweptBox.cpp
	${PHYSX_CCT_SRC}/CctSweptCapsule.cpp
	${PHYSX_CCT_SRC}/CctSweptVolume.cpp)
target_include_directories(physx_cct
	PUBLIC
		${PHYSX_DIR}/include/physx
		${PHYSX_DIR}/include/pxshared
	PRIVATE
		${PHYSX_CCT_SRC}
		${PHYSX_CCT_DIR}/common/src
		${PHYSX_CCT_DIR}/foundation/include
		${PHYSX_CCT_DIR}/geomutils/include
		${PHYSX_CCT_DIR}/geomutils/src)
target_compile_definitions(physx_cct PRIVATE
	PX_DEBUG=0
	PX_NVTX=0
	"$<$<CONFIG:Debug>:PX_CHECKED=1;PX_SUPPORT_PVD=1>"
	"$<$<NOT:$<CONFIG:Debug>>:PX_CHECKED=0;PX_SUPPORT_PVD=0>")
if(MSVC)
	target_compile_options(physx_cct PRIVATE /W0 /fp:fast)
endif()

target_link_directories(physx_cct PUBLIC
	"$<$<CONFIG:Debug>:${PHYSX_DIR}/lib/debug>"
	"$<$<CONFIG:Debug>:${PHYSX_DIR}/lib/checked>"
	"$<$<NOT:$<CONFIG:Debug>>:${PHYSX_DIR}/lib/release>"
)
target_link_libraries(physx_cct PUBLIC
	PhysXExtensions_static_64
	PhysX_64
	PhysXCommon_64
	PhysXFoundation_64)

add_library(physx INTERFACE)
target_link_libraries(physx INTERFACE
	physx_cct
	PhysXPvdSDK_static_64
	PhysXCooking_64)

# copy_physx_dlls() is defined in the root CMakeLists.txt
