cmake_minimum_required(VERSION 3.10)

project(LiteEdit C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/vendor/termbox2)
include_directories(${CMAKE_SOURCE_DIR}/vendor/tree-sitter/lib/src)
include_directories(${CMAKE_SOURCE_DIR}/vendor/tree-sitter/lib/include)

# Add Tree-sitter source files
set(TREE_SITTER_SOURCES ${CMAKE_SOURCE_DIR}/vendor/tree-sitter/lib/src/lib.c
                        ${CMAKE_SOURCE_DIR}/vendor/tree-sitter-c/src/parser.c)

# Add your project source files
set(SOURCES ${CMAKE_SOURCE_DIR}/src/main.c)

# Add executable target
add_executable(${PROJECT_NAME} ${SOURCES} ${TREE_SITTER_SOURCES})
