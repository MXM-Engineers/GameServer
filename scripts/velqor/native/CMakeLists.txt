cmake_minimum_required(VERSION 3.20)

project(velqor LANGUAGES CXX)

if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "velqor native harness requires the MSVC x86 (Win32) toolchain")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(VELQOR_DETOURS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Detours-4.0.1/src")

set(VELQOR_DETOURS_SOURCES
  "${VELQOR_DETOURS_DIR}/detours.cpp"
  "${VELQOR_DETOURS_DIR}/modules.cpp"
  "${VELQOR_DETOURS_DIR}/disasm.cpp"
  "${VELQOR_DETOURS_DIR}/image.cpp"
  "${VELQOR_DETOURS_DIR}/creatwth.cpp"
  "${VELQOR_DETOURS_DIR}/disolx86.cpp"
)

add_library(velqor SHARED
  dllmain.cpp
  velqor.cpp
  os_input.cpp
  client_bridge.cpp
  sha256.cpp
  ${VELQOR_DETOURS_SOURCES}
)

add_executable(velqor_inject
  inject.cpp
  sha256.cpp
  ${VELQOR_DETOURS_SOURCES}
)

foreach(velqor_target velqor velqor_inject)
  target_include_directories(${velqor_target} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${VELQOR_DETOURS_DIR}"
  )
  target_compile_definitions(${velqor_target} PRIVATE
    UNICODE
    _UNICODE
    DETOURS_X86
    WIN32_LEAN_AND_MEAN
    _WIN32_WINNT=0x0601
  )
  target_link_libraries(${velqor_target} PRIVATE
    bcrypt
    advapi32
    user32
    psapi
  )
  set_target_properties(${velqor_target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}"
  )
endforeach()

target_link_options(velqor PRIVATE "/export:DetourFinishHelperProcess,@1,NONAME")
