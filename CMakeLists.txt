# qimgv root
cmake_minimum_required(VERSION 3.13)
include(CMakePrintHelpers)

project(qimgv
    VERSION 0.8.9
    HOMEPAGE_URL "https://github.com/easymodo/qimgv"
    LANGUAGES CXX)


set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -pedantic")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-inconsistent-missing-override")
endif()

cmake_print_variables(
    CMAKE_SYSTEM
    CMAKE_SYSTEM_NAME
    CMAKE_SYSTEM_VERSION
    CMAKE_CXX_COMPILER_ID
    CMAKE_CXX_COMPILER_VERSION
    CMAKE_CXX_FLAGS )

#if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
#    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
#    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
#        Debug Release MinSizeRel RelWithDebInfo)
#endif()
message(STATUS "Build configuration: " ${CMAKE_BUILD_TYPE})

# detect if libstdc++ is in use to know if libstdc++fs should be linked
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("#include <iostream>
    #ifndef __GLIBCXX__
    #error not using libstdc++
    #endif
    int main() { return 0; }"
    USING_LIBSTDCXX)

# TODO: what about clang/libstdc++ (alternative linux configuration)
# check requirements for std::filesystem
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
        CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
    message(FATAL_ERROR "\n!!! THIS PROJECT REQUIRES GCC 8 OR LATER !!!\n"
        "If you already have gcc 8+ you can force it like this:\n"
        "CC=gcc-8 CXX=g++-8 cmake [...etc]")
    # force gcc 8
    #set(CMAKE_C_COMPILER "gcc-8")
    #set(CMAKE_CXX_COMPILER "g++-8")
endif()

# AUTOMOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# USER OPTIONS
# Usage: cmake -DVIDEO_SUPPORT=ON [...]
option(EXIV2 "For reading/writing exif tags" OFF)
option(VIDEO_SUPPORT "Enable video support" OFF)
option(KDE_SUPPORT "Support blur when using KDE" OFF)
option(BOOST_FS "Use boost filesystem instead of std" OFF)

# FIND PACKAGES
find_package(Qt5 REQUIRED COMPONENTS Core Concurrent Widgets)

if(BOOST_FS)
    find_package(Boost REQUIRED COMPONENTS filesystem)
endif()

if(EXIV2)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Exiv2 REQUIRED IMPORTED_TARGET exiv2)
endif()

if(KDE_SUPPORT)
    find_package(KF5WindowSystem REQUIRED)
endif()

##############################################################

add_subdirectory(qimgv)

if(VIDEO_SUPPORT)
    add_subdirectory(qimgv_libs/qimgv_player_mpv)
endif()
