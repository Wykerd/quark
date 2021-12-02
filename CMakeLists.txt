cmake_minimum_required(VERSION 3.10)

project(quark LANGUAGES C VERSION 0.1.0)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Build Options
option(BUILD_SHARED_LIBS "Build shared libraries (rather than static ones)" ON)
option(MEMLEAK_STACKTRACES "Include stack traces for memory leaks on linux systems" OFF)
# TODO: define
option(QRK_BUILD_NET "Build the networking libraries" ON)
option(QRK_BUILD_CRYPTO "Build the cryptography libraries" ON)
option(QRK_BUILD_URL "Build the WHATWG URL library" ON)

add_subdirectory(deps/libuv)
add_subdirectory(deps/llhttp)
find_package(OpenSSL REQUIRED)
find_package(ICU REQUIRED i18n io le lx tu uc data)

add_library(quarkstd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/std/utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/std/alloc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/std/buf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/std/spooky.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/std/hashmap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/std/err.c
)

set_target_properties(quarkstd PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

target_include_directories(quarkstd PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(quarkstd PUBLIC
        uv_a
        OpenSSL::SSL
        OpenSSL::Crypto
)


if (QRK_BUILD_NET)
    target_link_libraries(quarkstd PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endif()

if (UNIX AND NOT APPLE)
    if (MEMLEAK_STACKTRACES)
        target_compile_definitions(quarkstd PUBLIC QRK_MALLOC_DEBUG=1)
    endif()
endif()

if (QRK_BUILD_URL)
    add_library(quarkurl
        ${CMAKE_CURRENT_SOURCE_DIR}/src/url/url.c
    )
    target_include_directories(quarkurl PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${ICU_INCLUDE_DIRS}
    )
    set_target_properties(quarkurl PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
    )
    target_link_libraries(quarkurl PUBLIC
        quarkstd
        ${ICU_LIBRARIES}
    )
endif()

if (QRK_BUILD_CRYPTO)
    add_library(quarkcrypto
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto/bio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto/context.c
    )

    set_target_properties(quarkcrypto PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
    )

    target_include_directories(quarkcrypto PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OPENSSL_INCLUDE_DIR}
    )

    target_link_libraries(quarkcrypto PUBLIC
        quarkstd
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endif()

if (QRK_BUILD_NET)
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
        ${OPENSSL_INCLUDE_DIR}
    )

    target_link_libraries(quarknet PUBLIC
        quarkstd
        quarkcrypto
        llhttp
        uv_a
    )
endif()

add_executable(quarktest
    ${CMAKE_CURRENT_SOURCE_DIR}/src/demo.c
)

target_include_directories(quarktest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
)

target_link_libraries(quarktest PUBLIC quarkstd quarknet quarkcrypto quarkurl)

set_target_properties(quarktest PROPERTIES ENABLE_EXPORTS 1)
