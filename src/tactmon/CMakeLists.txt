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
  libassert
)

target_include_directories(tactmon
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}

    ${Boost_INCLUDE_DIR}
)

if (TACTMON_ENABLE_ADDRESS_SANITIZER)
  target_compile_options(tactmon
    PRIVATE
      -fno-omit-frame-pointer
      -fsanitize=address
      -fsanitize-recover=address
      -fsanitize-address-use-after-scope)

  target_link_options(tactmon
    PRIVATE
      -fno-omit-frame-pointer
      -fsanitize=address
      -fsanitize-recover=address
      -fsanitize-address-use-after-scope)
endif ()

set_target_properties(tactmon
  PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(tactmon
  PRIVATE
    boost
    openssl
    dpp::dpp
    libtactmon-static
    libpqxx::pqxx
    spdlog::spdlog
    spdlog::spdlog_header_only
    libassert
)

install(TARGETS tactmon
  DESTINATION "${CMAKE_INSTALL_PREFIX}"
)
