CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

find_package(spdlog REQUIRED)
find_package(ZLIB REQUIRED)

# Uses BUILD_SHARED_LIBS to determine if this is a static or shared library.
add_library(libtactmon ${PRIVATE_SOURCES})

if (NOT BUILD_SHARED_LIBS)
  target_compile_definitions(libtactmon
    PUBLIC
      LIBTACTMON_STATIC_BUILD
  )
endif ()

add_dependencies(libtactmon
  boost
  openssl
  libassert
)

target_include_directories(libtactmon
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE
    ${Boost_INCLUDE_DIR}
)

set_target_properties(libtactmon
  PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS}
)

target_link_libraries(libtactmon
  PRIVATE
    boost
    openssl
    libassert
  PUBLIC
    ZLIB::ZLIB
    spdlog::spdlog
    spdlog::spdlog_header_only
)

target_compile_definitions(libtactmon
  PRIVATE
    LIBTACTMON_BUILD
)

if (TACTMON_ENABLE_ADDRESS_SANITIZER)
  target_compile_options(libtactmon
    PRIVATE
      -fno-omit-frame-pointer
      -fsanitize=address
      -fsanitize-recover=address
      -fsanitize-address-use-after-scope)

  target_link_options(libtactmon
    PRIVATE
      -fno-omit-frame-pointer
      -fsanitize=address
      -fsanitize-recover=address
      -fsanitize-address-use-after-scope)
endif ()

install(TARGETS libtactmon
  DESTINATION "${CMAKE_INSTALL_PREFIX}"
)
