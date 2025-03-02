# Set the minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(PapiTest)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compile options
add_compile_options(-Wall -Wextra -O3)

# Allow the user to specify the PAPI path (useful for custom installations)
# Default to searching in standard locations
find_path(PAPI_INCLUDE_DIR papi.h)
find_library(PAPI_LIBRARIES NAMES papi)

# If the include directory is found, print its path
if (PAPI_INCLUDE_DIR)
    message(STATUS "PAPI include found at: ${PAPI_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "PAPI include directory (papi.h) not found.")
endif()

# If the library is found, print its path
if (PAPI_LIBRARIES)
    message(STATUS "PAPI library found at: ${PAPI_LIBRARIES}")
else()
    message(FATAL_ERROR "PAPI library (libpapi.a or libpapi.so) not found.")
endif()

# Include directories
include_directories(${PAPI_INCLUDE_DIR} include)

# Add the source files
add_executable(testing main.cpp)

# Link the PAPI library
target_link_libraries(testing ${PAPI_LIBRARIES})

