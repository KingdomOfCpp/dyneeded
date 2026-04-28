cmake_minimum_required(VERSION 3.23)
project(dyneeded LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# LTO on release
include(CheckIPOSupported)
check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_output)
if(_ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()

# MT(d) on Windows
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Deps (via vcpkg)
find_package(fmt CONFIG REQUIRED)
find_package(LIEF CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
find_package(glaze CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS unordered leaf)

# The core
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "core/*.cpp")
add_library(dyneeded_core STATIC ${CORE_SOURCES})
target_include_directories(dyneeded_core PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(dyneeded_core PUBLIC
        fmt::fmt
        LIEF::LIEF
        ftxui::screen
        ftxui::dom
        ftxui::component
        glaze::glaze
        Boost::unordered
        Boost::leaf
)

# Platform specific defs
if(UNIX AND NOT APPLE)
    target_compile_definitions(dyneeded_core PUBLIC DYNEEDED_LINUX)
elseif(WIN32)
    target_compile_definitions(dyneeded_core PUBLIC DYNEEDED_WINDOWS NOMINMAX)
    target_compile_options(dyneeded_core PUBLIC /utf-8)
elseif (APPLE)
    target_compile_definitions(dyneeded_core PUBLIC DYNEEDED_MACOS)
endif ()

# the binary
file(GLOB_RECURSE DYNEEDED_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(dyneeded ${DYNEEDED_SOURCES})
target_link_libraries(dyneeded PRIVATE dyneeded_core)

# install with CPACK
install(TARGETS dyneeded DESTINATION bin)
set(CPACK_PACKAGE_NAME "dyneeded")
set(CPACK_PACKAGE_VERSION "2.1.0")
set(CPACK_PACKAGE_VENDOR "KingdomOfCpp")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cross platform dynamic dependency CLI tool")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/KingdomOfCpp/dyneeded")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "dyneeded")
    set(CPACK_NSIS_MODIFY_PATH ON)
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "RPM;DEB;TGZ")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
    set(CPACK_RPM_PACKAGE_AUTOREQ ON)
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
    set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")

    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "KingdomOfCpp")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

include(CPack)