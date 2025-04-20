cmake_minimum_required(VERSION 3.15)

project(SharifBPE LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------------------------------------------------
# --- PCRE2 Submodule Configuration ---

# Set PCRE2 build options (BEFORE add_subdirectory)
# Build the common 8-bit library, statically
set(PCRE2_BUILD_PCRE2_8 ON CACHE BOOL "Build the PCRE2 8-bit library")
set(PCRE2_BUILD_PCRE2_16 OFF CACHE BOOL "Build the PCRE2 16-bit library")
set(PCRE2_BUILD_PCRE2_32 OFF CACHE BOOL "Build the PCRE2 32-bit library")

# Prefer static linking for submodules
set(PCRE2_BUILD_STATIC ON CACHE BOOL "Build PCRE2 static libraries")
set(PCRE2_BUILD_SHARED OFF CACHE BOOL "Build PCRE2 shared libraries") # Typically OFF for static

# Enable common features
set(PCRE2_SUPPORT_UNICODE ON CACHE BOOL "Enable Unicode support in PCRE2")
set(PCRE2_SUPPORT_JIT ON CACHE BOOL "Enable JIT support in PCRE2")

# Optionally disable tests to speed up build
set(PCRE2_BUILD_TESTS OFF CACHE BOOL "Build PCRE2 tests")
# You might need to check PCRE2's CMakeLists.txt for options to disable tools/docs if desired

# Tell CMake to process the PCRE2 CMakeLists.txt
# The path should match where you added the submodule
add_subdirectory(ThirdParty/pcre2)

get_target_property(PCRE2_INCLUDES pcre2-8 INTERFACE_INCLUDE_DIRECTORIES)

# --- End PCRE2 Configuration ---
# -------------------------------------------------------------------------------------------------
# Lib
add_library(SharifBPELib 
        "MMFile.h"
        "MaxHeap.h"
        "PairHasher.h"
        "MultiThreadFileReader.h"
        "MultiThreadFileReader.cpp"
        "BPELearner.h"
        "BPELearner.cpp"
        "BPETokenizer.h"
        "BPETokenizer.cpp"
)

target_link_libraries(SharifBPELib PRIVATE pcre2-8)

if (PCRE2_BUILD_STATIC)
    target_compile_definitions(SharifBPELib PRIVATE PCRE2_STATIC)
endif()

target_include_directories(SharifBPELib PUBLIC "ThirdParty" ${PCRE2_INCLUDES})

# -------------------------------------------------------------------------------------------------
# Main
add_executable(SharifBPE
            "Main.cpp"
)

target_link_libraries(SharifBPE PRIVATE SharifBPELib)

# -------------------------------------------------------------------------------------------------
# Tests

add_executable(UnitTests
        "Tests/TestMain.cpp"
        "Tests/TestMaxHeap.cpp"
)

 target_include_directories("UnitTests" PUBLIC 
        "${PROJECT_SOURCE_DIR}"
)

target_link_libraries(UnitTests PRIVATE SharifBPELib)

# -------------------------------------------------------------------------------------------------