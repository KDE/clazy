
#ifndef CLAZYLIB_EXPORT_H
#define CLAZYLIB_EXPORT_H

#ifdef CLAZYLIB_STATIC_DEFINE
#  define CLAZYLIB_EXPORT
#  define CLAZYLIB_NO_EXPORT
#else
#  ifndef CLAZYLIB_EXPORT
#    ifdef clazylib_EXPORTS
        /* We are building this library */
#      define CLAZYLIB_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define CLAZYLIB_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef CLAZYLIB_NO_EXPORT
#    define CLAZYLIB_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef CLAZYLIB_DEPRECATED
#  define CLAZYLIB_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef CLAZYLIB_DEPRECATED_EXPORT
#  define CLAZYLIB_DEPRECATED_EXPORT CLAZYLIB_EXPORT CLAZYLIB_DEPRECATED
#endif

#ifndef CLAZYLIB_DEPRECATED_NO_EXPORT
#  define CLAZYLIB_DEPRECATED_NO_EXPORT CLAZYLIB_NO_EXPORT CLAZYLIB_DEPRECATED
#endif

#define DEFINE_NO_DEPRECATED 0
#if DEFINE_NO_DEPRECATED
# define CLAZYLIB_NO_DEPRECATED
#endif

#endif
