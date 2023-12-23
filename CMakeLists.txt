cmake_minimum_required(VERSION 3.24)

project(smi)
# Use C++11
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
    message(STATUS "optional:-std=c++11")
endif(CMAKE_COMPILER_IS_GNUCXX)

# Set SAFESEH:NO
if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")
endif()

set(ENABLE_CONFIG_TESTS on)
# Set Version
set(SMI_MAJOR 1)
set(SMI_MINOR 0)
set(SMI_PATCH 0)
set(PROJECT_VERSION ${SMI_MAJOR}.${SMI_MINOR}.${SMI_PATCH})

include(3rdparty.cmake)

if (ENABLE_CONFIG_TESTS)
    add_subdirectory(config.tests)
endif()
add_subdirectory(depends)
add_subdirectory(src)
add_subdirectory(examples)
