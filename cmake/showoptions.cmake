# output generic information
if( UNIX )
  message("")
  message("* Build type      : ${CMAKE_BUILD_TYPE}")
endif()

message("")

# output information about installation-directories and locations

message("* Install to       : ${CMAKE_INSTALL_PREFIX}")
message("* Build tactmon    : ${TACTMON_BUILD}")
message("")

# Show infomation about the options selected during configuration

message("")

