CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

find_package(spdlog REQUIRED)
find_package(ZLIB REQUIRED)

add_library(libtactmon
  OBJECT
    ${PRIVATE_SOURCES}
)

add_dependencies(libtactmon
  boost
  openssl
  libassert
)

target_include_directories(libtactmon
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE
    # ${CMAKE_SOURCE_DIR}/dep/zlib

    ${Boost_INCLUDE_DIR}
)

set_target_properties(libtactmon
  PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
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

add_library(libtactmon-static STATIC $<TARGET_OBJECTS:libtactmon>)
add_library(libtactmon-shared SHARED $<TARGET_OBJECTS:libtactmon>)

target_compile_definitions(libtactmon-static
  PRIVATE
    LIBTACTMON_STATIC_BUILD
)
