#ifndef R2_CONFIGURE_H
#define R2_CONFIGURE_H

#include "r_version.h"

#define DEBUGGER 1

#if __WINDOWS__ || __CYGWIN__ || MINGW32
#define R2_PREFIX "."
#define R2_LIBDIR "./lib"
#define R2_INCDIR "./include/libr"
#define R2_DATDIR "./share"
#else
#define R2_PREFIX "/usr/local"
#define R2_LIBDIR "/usr/local/lib"
#define R2_INCDIR "/usr/local/include/libr"
#define R2_DATDIR "/usr/local/share"
#endif

#define R2_VERSION "1.6.0-git"
#define HAVE_LIB_MAGIC 0
#define USE_LIB_MAGIC 0
#ifndef HAVE_LIB_SSL
#define HAVE_LIB_SSL 0
#endif

#define HAVE_FORK 1

#define WITH_GPL 1

#define R2_WWWROOT R2_DATDIR "/radare2/" R2_VERSION "/www"

#endif
