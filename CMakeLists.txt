cmake_minimum_required(VERSION 3.10)

project(quark LANGUAGES C VERSION 0.1.0)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Build Options
option(BUILD_SHARED_LIBS "Build shared libraries (rather than static ones)" ON)
option(MEMLEAK_STACKTRACES "Include stack traces for memory leaks on linux systems" OFF)

add_subdirectory(deps/libuv)

add_library(quarkstd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/std/utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/std/alloc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/std/buf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/std/spooky.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/std/hashmap.c
)

set_target_properties(quarkstd PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

target_include_directories(quarkstd PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
)

if (UNIX AND NOT APPLE)
    if (MEMLEAK_STACKTRACES)
        target_compile_definitions(quarkstd PUBLIC QRK_MALLOC_DEBUG=1)
    endif()
endif()

add_library(quarknet
    ${CMAKE_CURRENT_SOURCE_DIR}/src/net/dns.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/net/tcp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/net/tls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws.c
)

set_target_properties(quarknet PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

target_include_directories(quarknet PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
)

target_link_libraries(quarknet PUBLIC quarkstd uv_a)

add_executable(quarktest
    ${CMAKE_CURRENT_SOURCE_DIR}/src/demo.c
)

target_include_directories(quarktest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
)

target_link_libraries(quarktest PUBLIC quarkstd)

set_target_properties(quarktest PROPERTIES ENABLE_EXPORTS 1)
