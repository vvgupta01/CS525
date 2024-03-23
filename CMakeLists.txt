cmake_minimum_required(VERSION 3.0.0)
project(FairShares VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
enable_testing()

add_library(karma src/karma/allocator.cpp src/karma/bheap.cpp)
target_include_directories(karma PUBLIC include)

add_executable(main src/main.cpp)
target_include_directories(main PUBLIC include)
target_link_libraries(main PRIVATE karma)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
