#pragma once

// Provides macros to declare API for libtactmon.

#ifndef LIBTACTMON_STATIC_BUILD
# ifdef LIBTACTMON_BUILD // Building the library
#  ifdef _WIN32
#   define LIBTACTMON_API __declspec(dllexport)
#  else
#   define LIBTACTMON_API __attribute__((visibility("default")))
#  endif
# else // Linking to the library
#   ifdef _WIN32
#    define LIBTACTMON_API __declspec(dllimport)
#   else
#    define LIBTACTMON_API
#   endif
# endif
#else
# define LIBTACTMON_API
#endif
