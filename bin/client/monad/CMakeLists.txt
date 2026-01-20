cmake_minimum_required(VERSION 3.15)
project(monad_wrapper C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Build the wrapper as a static library for Rust FFI
add_library(monad_wrapper STATIC
    cpp/wrapper.cpp
)

# Set output directory to where Rust expects it
set_target_properties(monad_wrapper PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Include directories
target_include_directories(monad_wrapper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
)

# For future integration with Monad interpreter
# Uncomment when ready to integrate full interpreter:
# target_include_directories(monad_wrapper PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/cpp/evmc/include
#     ${CMAKE_CURRENT_SOURCE_DIR}/cpp
# )

# Install the library so cargo can find it
install(TARGETS monad_wrapper
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
