cmake_minimum_required(VERSION 3.9)

project (yosys_constraint_propagation VERSION 1.0 DESCRIPTION "yosys extension for constraint propagation")

# LLVM 13 requires this
set(CMAKE_CXX_STANDARD 17)

# Stuff required for Yosys
include_directories(/home/yuzeng/workspace/tools/yosys/share/include)
include_directories(/usr/include/tcl8.6)

add_compile_options(-Wall -Wextra -ggdb -MD -MP -std=c++11 -Os -rdynamic)
add_compile_definitions(
    _YOSYS_
    YOSYS_ENABLE_READLINE YOSYS_ENABLE_PLUGINS
    YOSYS_ENABLE_GLOB YOSYS_ENABLE_ZLIB
    YOSYS_ENABLE_TCL YOSYS_ENABLE_ABC
    YOSYS_ENABLE_COVER
)
set(YOSYS_LIBS -lstdc++ -lm -lrt -lreadline -lffi -ldl -lz -ltcl8.6 -ltclstub8.6)

aux_source_directory(./src SRC_DIR)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g")


# The shared library that will contain the Yosys extension we are making
add_library(${PROJECT_NAME} SHARED ${SRC_DIR})

target_link_libraries(${PROJECT_NAME} ${YOSYS_LIBS})

# Add tags target 
set_source_files_properties(tags PROPERTIES GENERATED true)
add_custom_target(tags
    COMMAND ctags -R  --extra=+f .
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})


enable_testing()
