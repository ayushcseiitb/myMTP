cmake_minimum_required(VERSION 3.0)

project(llvm-demo)

# Find LLVM
find_package(LLVM 15 REQUIRED CONFIG)

set(CMAKE_BUILD_TYPE Debug)

# If LLVM is not found, provide an error message and stop the CMake configuration
if(NOT LLVM_FOUND)
    message(FATAL_ERROR "LLVM 15 not found. Please make sure LLVM 15 is installed and set LLVM_DIR.")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Include LLVM headers
include_directories(${LLVM_INCLUDE_DIRS})

# Add the path to LLVM /usr/lib/llvm-15/includelibraries to the linker search paths
link_directories(${LLVM_LIBRARY_DIRS})

# Find Clang
find_package(Clang REQUIRED CONFIG HINTS "/usr/lib/llvm-15/lib/cmake/clang")

# If Clang is not found, provide an error message and stop the CMake configuration
if(NOT Clang_FOUND)
    message(FATAL_ERROR "Clang 15 not found. Please make sure Clang 15 is installed and set Clang_DIR.")
endif()

message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

# Include Clang headers
include_directories(${Clang_INCLUDE_DIRS})

# Add the path to Clang libraries to the linker search paths
link_directories(${Clang_LIBRARY_DIRS})

# Add your source file for the executable
add_executable(demo demo.cpp)
# add_executable(helloworld helloworld.cpp)

# Link LLVM and Clang libraries
target_link_libraries(demo clangDaemon clangFrontendTool)

