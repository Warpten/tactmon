CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(wow-build-monitor
  ${PRIVATE_SOURCES}
)

add_dependencies(wow-build-monitor
  boost
  openssl
)

target_include_directories(wow-build-monitor
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/dep/zlib

    ${Boost_INCLUDE_DIR}
)

target_compile_definitions(wow-build-monitor
  PRIVATE
    BOOST_BEAST_USE_STD_STRING_VIEW
)

set_target_properties(wow-build-monitor
  PROPERTIES
  LINK_FLAGS "${wownitor_LINK_FLAGS}"
)

set_property(TARGET wow-build-monitor
  PROPERTY
    CXX_STANDARD 20
)
set_property(TARGET wow-build-monitor
  PROPERTY
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(wow-build-monitor
  PRIVATE
    boost
    openssl
    ${ZLIB_LIBRARIES}
)

install(TARGETS wow-build-monitor
  DESTINATION "${CMAKE_INSTALL_PREFIX}"
)
