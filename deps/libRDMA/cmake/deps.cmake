cmake_minimum_required(VERSION 3.7)

option( LINK_STATIC_LIB "Link static version of libssmalloc and libboost" true)

## install Rmalloc (built from ssmalloc)
include( ExternalProject )
set( SSMALLOC_INSTALL_DIR ${CMAKE_SOURCE_DIR})

ExternalProject_Add( ralloc
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/ralloc
  CONFIGURE_COMMAND mkdir -p ${CMAKE_SOURCE_DIR}/lib
  BUILD_COMMAND make
  BUILD_IN_SOURCE 1
  INSTALL_COMMAND make install
)
set( LIBSSMALLOC_HEADERS ${SSMALLOC_INSTALL_DIR}/include )
set( LIBSSMALLOC_LIBRARIES ${SSMALLOC_INSTALL_DIR}/lib )

#
# Configure dependencies: both built-in and external
#
# find_library( LIBZMQ NAMES zmq HINTS /usr/lib64/)
find_library( LIBIBVERBS NAMES ibverbs HINTS /usr/lib64/)
if( LINK_STATIC_LIB )
  add_library( ssmalloc STATIC IMPORTED )
  set_target_properties( ssmalloc PROPERTIES
    IMPORTED_LOCATION ${LIBSSMALLOC_LIBRARIES}/libssmalloc.a
    )
else()
  add_library( ssmalloc SHARED IMPORTED )
  set_target_properties( ssmalloc PROPERTIES
    IMPORTED_LOCATION ${LIBSSMALLOC_LIBRARIES}/libssmalloc.so
    )
endif()

# find_path(ZMQ_CPP NAMES zmq.h HINTS  /usr/include/)
include_directories( BEFORE ${LIBSSMALLOC_HEADERS} )
# include_directories( BEFORE ${ZMQ_CPP} )
