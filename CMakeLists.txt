# Set the minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(PapiTest)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -O3)  

# Include directories
include_directories(/usr/local/include)
include_directories(include)

# Add the source files
add_executable(testing main.cpp)

# Link the static library
target_link_libraries(testing /usr/local/lib/libpapi.a)

