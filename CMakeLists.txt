cmake_minimum_required(VERSION 3.10)
project(TopologicalSortCompressedAdj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# sources
set(SRCS
    topsort.cpp
    core/graph.cpp
)

add_executable(topsort ${SRCS})
target_include_directories(topsort PRIVATE ${CMAKE_SOURCE_DIR}/core)
