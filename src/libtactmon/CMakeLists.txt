CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

find_package(spdlog REQUIRED)

add_library(libtactmon
  STATIC
    ${PRIVATE_SOURCES}
)

add_dependencies(libtactmon
  boost
  openssl
)

target_include_directories(libtactmon
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE
    ${CMAKE_SOURCE_DIR}/dep/zlib

    ${Boost_INCLUDE_DIR}
)

target_compile_definitions(libtactmon
  PRIVATE
    BOOST_BEAST_USE_STD_STRING_VIEW
)

set_target_properties(libtactmon
  PROPERTIES
  LINK_FLAGS "${wownitor_LINK_FLAGS}"
)

set_property(TARGET libtactmon
  PROPERTY
    CXX_STANDARD 20
)
set_property(TARGET libtactmon
  PROPERTY
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(libtactmon
  PRIVATE
    boost
    openssl
    ${ZLIB_LIBRARIES}
  PUBLIC
    spdlog::spdlog
    spdlog::spdlog_header_only
)

install(TARGETS libtactmon
  DESTINATION "${CMAKE_INSTALL_PREFIX}"
)
