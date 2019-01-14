#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([librdam], [1.0], [windybeing@sjtu.edu.cn])
AM_INIT_AUTOMAKE([librdma], [1.0])
AC_CONFIG_SRCDIR([include/rdmaio.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

LT_INIT
LTVER=2:0:0
# Checks for libraries.
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main])

# Checks for header files.
AC_CHECK_HEADERS([malloc.h stddef.h stdint.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime gettimeofday memset munmap socket strerror])

AC_OUTPUT([Makefile] [src/Makefile])
