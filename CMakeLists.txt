cmake_minimum_required(VERSION 3.10)

# tool name
set(TOOL clang-xform)

# The version number.
set (CLANG_XFORM_VERSION_MAJOR 1)
set (CLANG_XFORM_VERSION_MINOR 1)
set (CLANG_XFORM_VERSION_PATCH 0)

set(VERSION ${CLANG_XFORM_VERSION_MAJOR}.${CLANG_XFORM_VERSION_MINOR}.${CLANG_XFORM_VERSION_PATCH})
message(STATUS "${TOOL} version ${VERSION}")

project(${TOOL} CXX)

if (NOT DEFINED LLVM_ROOT_PATH)
  message(FATAL_ERROR "Clang 9.0.0 required. Please provide LLVM root path.\n"
  "Usage: cmake <dir> -DLLVM_ROOT_PATH=<LLVM root path> \n"
  "Clang prebuilt binaries are available at http://releases.llvm.org/download.html")
endif (NOT DEFINED LLVM_ROOT_PATH)

# get absolute path for LLVM_ROOT_PATH
get_filename_component(LLVM_ROOT_PATH ${LLVM_ROOT_PATH} ABSOLUTE)

find_package(Clang REQUIRED CONFIG
  HINTS "${LLVM_ROOT_PATH}/lib/cmake/clang")

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/clang_xform_config.hpp.in"
  "${PROJECT_SOURCE_DIR}/include/clang_xform_config.hpp"
  )

# source files
file(GLOB_RECURSE SRC_CPP
    src/*.cpp
)

# include/link path
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
link_directories(${LLVM_LIBRARY_DIRS})

# set compile_options
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  add_definitions(${LLVM_DEFINITIONS} -DCXXOPTS_NO_RTTI -DLLVM_ROOT_PATH="${LLVM_ROOT_PATH}")
  add_compile_options(-std=c++14 -fno-rtti)
else ()
  add_definitions(${LLVM_DEFINITIONS} /DCXXOPTS_NO_RTTI /DLLVM_ROOT_PATH=${LLVM_ROOT_PATH})
  add_compile_options(/std:c++14 /GR-)
endif()

# clang libs to link
set(CLANG_LIBS clangTooling clangToolingCore clangFrontendTool clangFrontend clangDriver clangBasic)
set(CLANG_LIBS ${CLANG_LIBS} clangSerialization clangParse clangSema clangAnalysis clangEdit)
set(CLANG_LIBS ${CLANG_LIBS} clangRewrite clangRewriteFrontend clangAST clangASTMatchers clangLex)
set(CLANG_LIBS ${CLANG_LIBS} clangToolingRefactoring clangFormat clangToolingInclusions)

# enable testing
enable_testing()
option(BUILD_TESTS "Set to ON to build tests" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(${TOOL} ${SRC_CPP})
target_link_libraries(${TOOL} ${CLANG_LIBS})

# install binary
install(TARGETS ${TOOL} DESTINATION bin)

add_subdirectory(test)
