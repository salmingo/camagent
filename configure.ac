dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(camagent, 0.3)

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE()

AC_PROG_CXX
AC_PROG_CC
CXXFLAGS="-std=c++0x"

AC_CHECK_FUNCS([bzero ftruncate gettimeofday memset mkdir select socket strcasecmp strerror])
AC_CHECK_HEADERS([fcntl.h limits.h netdb.h stdlib.h string.h strings.h sys/socket.h sys/vfs.h syslog.h unistd.h arpa/inet.h netinet/in.h])
AC_CHECK_HEADER_STDBOOL
AC_FUNC_FORK
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT
