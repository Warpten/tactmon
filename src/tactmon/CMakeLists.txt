CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

find_package(dpp CONFIG REQUIRED)
find_package(libpqxx REQUIRED)
find_package(spdlog REQUIRED)

add_executable(tactmon
  ${PRIVATE_SOURCES}
)

add_dependencies(tactmon
  boost
  openssl
  libtactmon
  libpqxx
)

target_include_directories(tactmon
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/dep/zlib

    ${Boost_INCLUDE_DIR}
)

target_compile_definitions(tactmon
  PRIVATE
    BOOST_BEAST_USE_STD_STRING_VIEW
)

set_property(TARGET tactmon
  PROPERTY
    CXX_STANDARD 20
)
set_property(TARGET tactmon
  PROPERTY
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(tactmon
  PRIVATE
    boost
    openssl
    ${ZLIB_LIBRARIES}
    dpp::dpp
    libtactmon
    pqxx
    spdlog::spdlog
    spdlog::spdlog_header_only
)

install(TARGETS tactmon
  DESTINATION "${CMAKE_INSTALL_PREFIX}"
)
