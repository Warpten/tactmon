if (NOT BUILD_SHARED_LIBS)
  set(OPENSSL_USE_STATIC_LIBS TRUE)
endif ()

find_package(OpenSSL 1.1 REQUIRED COMPONENTS Crypto SSL)

add_library(openssl INTERFACE)

target_link_libraries(openssl
  INTERFACE
    OpenSSL::Crypto
    OpenSSL::SSL
)
