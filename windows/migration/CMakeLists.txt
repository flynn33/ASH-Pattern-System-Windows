cmake_minimum_required(VERSION 3.15)
project(ash-pattern-system-windows-migration VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add executable
add_executable(ash-pattern-system-windows-migration
    src/main.cpp
)

# Include directories
target_include_directories(ash-pattern-system-windows-migration PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link libraries (if any)
# target_link_libraries(ash-pattern-system-windows-migration PRIVATE ...)

# Set properties
set_target_properties(ash-pattern-system-windows-migration PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
