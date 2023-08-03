/** \file CutterCommon.h
 * This file contains any definition that is useful in the whole project.
 * For example, it may contain custom types (RVA, ut64), list iterators, etc.
 */
#ifndef CUTTERCORE_H
#define CUTTERCORE_H

#include "rz_core.h"
#include <QString>
#include "RizinCpp.h"

// Workaround for compile errors on Windows
#ifdef Q_OS_WIN
#    undef min
#    undef max
#endif // Q_OS_WIN

// Global information for Cutter
#define APPNAME "Cutter"

/**
 * @brief Type to be used for all kinds of addresses/offsets in rizin address space.
 */
typedef ut64 RVA;

/**
 * @brief Maximum value of RVA. Do NOT use this for specifying invalid values, use RVA_INVALID
 * instead.
 */
#define RVA_MAX UT64_MAX

/**
 * @brief Value for specifying an invalid RVA.
 */
#define RVA_INVALID RVA_MAX

inline QString RzAddressString(RVA addr)
{
    return QString::asprintf("%#010llx", addr);
}

inline QString RzSizeString(RVA size)
{
    return QString::asprintf("%#llx", size);
}

inline QString RzHexString(RVA size)
{
    return QString::asprintf("%#llx", size);
}

#ifdef CUTTER_SOURCE_BUILD
#    define CUTTER_EXPORT Q_DECL_EXPORT
#else
#    define CUTTER_EXPORT Q_DECL_IMPORT
#endif

#if defined(__has_cpp_attribute)
#    if __has_cpp_attribute(deprecated)
#        define CUTTER_DEPRECATED(msg) [[deprecated(msg)]]
#    endif
#endif
#if !defined(CUTTER_DEPRECATED)
#    define CUTTER_DEPRECATED(msg)
#endif

#endif // CUTTERCORE_H
