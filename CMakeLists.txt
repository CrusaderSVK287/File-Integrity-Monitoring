cmake_minimum_required(VERSION 3.18)
project(CPPPythonFileMonitor LANGUAGES CXX)

# -------------------------------------------------------------
# Project settings
# -------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -------------------------------------------------------------
# Output directories
# -------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/)

# -------------------------------------------------------------
# Include directories
# -------------------------------------------------------------
include_directories(${PROJECT_SOURCE_DIR}/include)

# -------------------------------------------------------------
# Find Python and pybind11
# -------------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG QUIET)

# -------------------------------------------------------------
# Gather C++ sources and build executable
# -------------------------------------------------------------
file(GLOB_RECURSE CPP_SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_executable(monitor ${CPP_SOURCES})

target_include_directories(monitor PRIVATE ${Python3_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(monitor PRIVATE ${Python3_LIBRARIES})
# Link pybind11 embed if available (Dont uncomment, fucks shit up for some reason)
#target_link_libraries(monitor PRIVATE pybind11::embed)

# Platform-specific adjustments
if(WIN32)
    add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
elseif(UNIX)
    target_link_libraries(monitor PRIVATE pthread dl)
endif()

# -------------------------------------------------------------
# Create output directory if it doesnt exist
# -------------------------------------------------------------
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/output)

# -------------------------------------------------------------
# Copy python modules into output
# -------------------------------------------------------------
find_program(PYTHON_EXECUTABLE python3 REQUIRED)
file(GLOB PYTHON_SCRIPTS ${PROJECT_SOURCE_DIR}/python/*.py)
foreach(pyfile ${PYTHON_SCRIPTS})
    add_custom_command(
        TARGET monitor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${pyfile} ${CMAKE_SOURCE_DIR}/output/
        COMMENT "Copying ${pyfile} to output"
    )
endforeach()

# -------------------------------------------------------------
# Developer info
# -------------------------------------------------------------
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Python3 include dir: ${Python3_INCLUDE_DIRS}")
message(STATUS "Python3 library: ${Python3_LIBRARIES}")
message(STATUS "Executable output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "Python modules will be copied here: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
