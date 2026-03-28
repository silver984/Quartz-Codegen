# quartz-gen-bindings

cmake_minimum_required(VERSION 3.21)

project(quartz-gen-bindings VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

set(EXE_NAME bindings)

add_executable(${EXE_NAME} ${SOURCES})

target_include_directories(${EXE_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

include(FetchContent)

# fmt
FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 12.1.0
)

FetchContent_MakeAvailable(fmt)

target_link_libraries(${EXE_NAME}
    PRIVATE
        fmt::fmt
)