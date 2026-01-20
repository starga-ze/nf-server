cmake_minimum_required(VERSION 3.10)
project(PacketSimulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(GLOB_RECURSE SRC_FILES
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

include_directories(
        ${CMAKE_SOURCE_DIR}/src
)

set(THIRD_PARTY_INSTALL_DIR "${CMAKE_SOURCE_DIR}/3rd_party/install")
list(APPEND CMAKE_PREFIX_PATH ${THIRD_PARTY_INSTALL_DIR}/spdlog)
list(APPEND CMAKE_PREFIX_PATH ${THIRD_PARTY_INSTALL_DIR}/openssl)


find_package(spdlog REQUIRED)

find_library(CRYPTO_LIBRARY NAMES crypto
        PATHS ${THIRD_PARTY_INSTALL_DIR}/openssl/lib64 ${THIRD_PARTY_INSTALL_DIR}/openssl/lib
        NO_DEFAULT_PATH)

find_library(SSL_LIBRARY NAMES ssl
        PATHS ${THIRD_PARTY_INSTALL_DIR}/openssl/lib64 ${THIRD_PARTY_INSTALL_DIR}/openssl/lib
        NO_DEFAULT_PATH)

find_path(OPENSSL_INCLUDE_DIR NAMES openssl/ssl.h
        PATHS ${THIRD_PARTY_INSTALL_DIR}/openssl/include
        NO_DEFAULT_PATH)

if (NOT CRYPTO_LIBRARY OR NOT SSL_LIBRARY OR NOT OPENSSL_INCLUDE_DIR)
    message(FATAL_ERROR "
        [ERROR] OpenSSL/spdlog pre-installed libraries not found. 
        Please run the manual installation steps first to populate 
        the '3rd_party/install' directory.
    ")
endif ()

add_library(OpenSSL::SSL UNKNOWN IMPORTED)
set_target_properties(OpenSSL::SSL PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}"
        IMPORTED_LOCATION "${SSL_LIBRARY}"
)

add_library(OpenSSL::Crypto UNKNOWN IMPORTED)
set_target_properties(OpenSSL::Crypto PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}"
        IMPORTED_LOCATION "${CRYPTO_LIBRARY}"
)


find_package(Threads REQUIRED)

add_executable(nf-server ${SRC_FILES})

target_link_libraries(nf-server PRIVATE
        spdlog::spdlog
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
        odbc
)
