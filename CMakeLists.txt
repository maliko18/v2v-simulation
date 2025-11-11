cmake_minimum_required(VERSION 3.25)
project(V2V_Simulator VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# Configuration pour CLion - Projet Professionnel V2V
# ============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands pour CLion IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# Options de Build
# ============================================================================

option(BUILD_TESTS "Build unit tests" OFF)
option(ENABLE_PROFILING "Enable profiling support" OFF)
option(USE_CCACHE "Use ccache if available" ON)

# Detect build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ============================================================================
# Flags de Compilation Haute Performance
# ============================================================================

if(CMAKE_BUILD_TYPE MATCHES "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -DNDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math -flto")
    message(STATUS "Release build - Optimizations enabled")
elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -Wall -Wextra -Wpedantic")
    message(STATUS "Debug build - All warnings enabled")
endif()

# Enable LTO (Link Time Optimization) pour Release
if(CMAKE_BUILD_TYPE MATCHES "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "LTO enabled")
    endif()
endif()

# ccache support
if(USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        message(STATUS "ccache found: ${CCACHE_PROGRAM}")
    endif()
endif()

# ============================================================================
# Find Qt6 (Obligatoire)
# ============================================================================

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    OpenGLWidgets
    Network
    Concurrent
    Svg
    Positioning
)

message(STATUS "Qt6 found: ${Qt6_VERSION}")

# ============================================================================
# Find Boost (Graph, Geometry, etc.)
# ============================================================================

find_package(Boost 1.74 REQUIRED COMPONENTS
    system
    filesystem
    program_options
)

message(STATUS "Boost found: ${Boost_VERSION}")

# ============================================================================
# Find libosmium (Optional - not currently used, but may be needed in future)
# ============================================================================

find_path(OSMIUM_INCLUDE_DIR osmium/osm.hpp
    PATHS /usr/include /usr/local/include
)

if(OSMIUM_INCLUDE_DIR)
    message(STATUS "libosmium found: ${OSMIUM_INCLUDE_DIR}")
else()
    message(WARNING "libosmium not found (optional - not currently used)")
    set(OSMIUM_INCLUDE_DIR "")
endif()

# ============================================================================
# Find TBB (Intel Threading Building Blocks)
# ============================================================================

find_package(TBB REQUIRED)
message(STATUS "TBB found")

# ============================================================================
# Find Eigen3 (Math library)
# ============================================================================

find_package(Eigen3 CONFIG REQUIRED)   # no version
message(STATUS "Eigen3 found: ${EIGEN3_VERSION}")

# ============================================================================
# Find cURL
# ============================================================================

find_package(CURL REQUIRED)
message(STATUS "CURL found")

# ============================================================================
# Find SQLite3
# ============================================================================

find_package(SQLite3 REQUIRED)
message(STATUS "SQLite3 found")

# ============================================================================
# OpenGL (pour QOpenGLWidget)
# ============================================================================

find_package(OpenGL REQUIRED)

# ============================================================================
# Include Directories
# ============================================================================

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)

if(OSMIUM_INCLUDE_DIR)
    include_directories(${OSMIUM_INCLUDE_DIR})
endif()

# ============================================================================
# Source Files
# ============================================================================

set(CORE_SOURCES
    src/core/Vehicle.cpp
    src/core/SimulationEngine.cpp
    src/core/TimeController.cpp
)

set(NETWORK_SOURCES
    src/network/RoadGraph.cpp
    src/network/InterferenceGraph.cpp
    src/network/PathPlanner.cpp
)

set(VISUALIZATION_SOURCES
    src/visualization/MainWindow.cpp
    src/visualization/MapView.cpp
)

set(DATA_SOURCES
    src/data/OSMParser.cpp
    src/data/TileManager.cpp
    src/data/GeometryUtils.cpp
)

set(UTILS_SOURCES
    src/utils/Logger.cpp
    src/utils/Profiler.cpp
    src/utils/Config.cpp
)

set(ALL_SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${NETWORK_SOURCES}
    ${VISUALIZATION_SOURCES}
    ${DATA_SOURCES}
    ${UTILS_SOURCES}
)

# ============================================================================
# Header Files (pour CLion)
# ============================================================================

set(CORE_HEADERS
    include/core/Vehicle.hpp
    include/core/SimulationEngine.hpp
    include/core/TimeController.hpp
)

set(NETWORK_HEADERS
    include/network/RoadGraph.hpp
    include/network/InterferenceGraph.hpp
    include/network/PathPlanner.hpp
)

set(VISUALIZATION_HEADERS
    include/visualization/MainWindow.hpp
    include/visualization/MapView.hpp
)

set(DATA_HEADERS
    include/data/OSMParser.hpp
    include/data/TileManager.hpp
    include/data/GeometryUtils.hpp
)

set(UTILS_HEADERS
    include/utils/Logger.hpp
    include/utils/Profiler.hpp
    include/utils/Config.hpp
)

set(ALL_HEADERS
    ${CORE_HEADERS}
    ${NETWORK_HEADERS}
    ${VISUALIZATION_HEADERS}
    ${DATA_HEADERS}
    ${UTILS_HEADERS}
)

# ============================================================================
# Executable
# ============================================================================

add_executable(v2v_simulator
    ${ALL_SOURCES}
    ${ALL_HEADERS}
)

# ============================================================================
# Link Libraries
# ============================================================================

target_link_libraries(v2v_simulator PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::OpenGLWidgets
    Qt6::Network
    Qt6::Concurrent
    Qt6::Svg
    Qt6::Positioning
    ${Boost_LIBRARIES}
    TBB::tbb
    Eigen3::Eigen
    CURL::libcurl
    SQLite::SQLite3
    OpenGL::GL
    pthread
)

# ============================================================================
# Compiler Definitions
# ============================================================================

target_compile_definitions(v2v_simulator PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x060500
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
    $<$<CONFIG:Release>:QT_NO_WARNING_OUTPUT>
)

# ============================================================================
# Installation
# ============================================================================

install(TARGETS v2v_simulator
    RUNTIME DESTINATION bin
)

install(DIRECTORY config/
    DESTINATION share/v2v_simulator/config
)

install(DIRECTORY resources/
    DESTINATION share/v2v_simulator/resources
)

# ============================================================================
# Tests (Optionnel)
# ============================================================================

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "V2V Simulator Configuration Summary")
message(STATUS "========================================")
message(STATUS "Build type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt6 version:     ${Qt6_VERSION}")
message(STATUS "Boost version:   ${Boost_VERSION}")
message(STATUS "Compiler:        ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CXX Standard:    C++${CMAKE_CXX_STANDARD}")
message(STATUS "CXX Flags:       ${CMAKE_CXX_FLAGS}")
if(CMAKE_BUILD_TYPE MATCHES "Release")
    message(STATUS "Release Flags:   ${CMAKE_CXX_FLAGS_RELEASE}")
endif()
message(STATUS "========================================")
message(STATUS "")
