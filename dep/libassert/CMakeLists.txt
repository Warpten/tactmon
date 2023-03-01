CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
)

add_library(libassert
  STATIC
    ${PRIVATE_SOURCES}
)

if (WIN32)
  target_link_libraries(libassert
    PUBLIC
      dbghelp
  )
else ()
  target_link_libraries(libassert
    PUBLIC
      dl
  )
endif()

target_include_directories(libassert
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
