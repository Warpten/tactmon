# CMake policies (can not be handled elsewhere)
cmake_minimum_required(VERSION 3.25.0)
cmake_policy(SET CMP0005 OLD)
if(POLICY CMP0043)
  cmake_policy(SET CMP0043 OLD) # Disable 'Ignore COMPILE_DEFINITIONS_<Config> properties'
endif(POLICY CMP0043)

# add this options before PROJECT keyword
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# set default buildoptions and print them
include(cmake/options.cmake)
include(cmake/showoptions.cmake)

# Handle vcpkg features here, before project()
if (TACTMON_BUILD)
   list(APPEND VCPKG_MANIFEST_FEATURES "frontend")
endif ()

# Set projectname (must be done AFTER setting configurationtypes)
project(libtactmon)

# Set RPATH-handing (CMake parameters)
set(CMAKE_SKIP_BUILD_RPATH 0)
set(CMAKE_BUILD_WITH_INSTALL_RPATH 0)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH 1)

# set macro-directory
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/macros")

# build in Release-mode by default if not explicitly set
if( NOT CMAKE_BUILD_TYPE )
  set(CMAKE_BUILD_TYPE "Release")
endif()

include(CheckCXXSourceRuns)
include(CheckIncludeFiles)
include(CheckPlatform)
include(AutoCollect)
include(GroupSources)

find_package(Threads REQUIRED)

# add sources
add_subdirectory(src)

add_subdirectory(dep)