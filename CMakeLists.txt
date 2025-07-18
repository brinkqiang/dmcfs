cmake_minimum_required(VERSION 3.21)

PROJECT(dmcfs)

LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
INCLUDE(cmake/ModuleImport.cmake)
INCLUDE(cmake/ModuleCompileOptions.cmake)
ModuleSetCompileOptions()

SET(DMCFS_VERSION_MAJOR "1")
SET(DMCFS_VERSION_MINOR "0")
SET(DMCFS_VERSION_PATCH "1")
SET(DMCFS_VERSION "${DMCFS_VERSION_MAJOR}.${DMCFS_VERSION_MINOR}.${DMCFS_VERSION_PATCH}")

MESSAGE(STATUS "VERSION: ${DMCFS_VERSION}")

OPTION(USE_DMCFS "use dmcfs" OFF)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/dmcfs_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/dmcfs_config.h)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

ModuleImportAll("thirdparty")

LibImport("libdmcfs" "src/libdmcfs")

if(PROJECT_IS_TOP_LEVEL)
    ExeImport("test" "libdmcfs;dmtest")
    ExeImport("examples" "libdmcfs")
endif()

AddInstall("libdmcfs" "${CMAKE_CURRENT_SOURCE_DIR}/include")
