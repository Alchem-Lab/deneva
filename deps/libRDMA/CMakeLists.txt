cmake_minimum_required(VERSION 3.7)

project(rdmaio)
set(CMAKE_CXX_COMPILER g++)
ADD_DEFINITIONS(-std=c++0x)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g -pthread -libverbs")

include(cmake/deps.cmake)

add_library(rdma STATIC
	src/qp.cc
	src/rc.cc
	src/rdma_ctrl.cc
	src/ring_imm_msg.cc
	src/ring_msg.cc
	src/uc.cc
	src/ud.cc
	src/ud_msg.cc
)

find_library(LIBIBVERBS NAMES ibverbs HINTS /usr/lib64/)
if(NOT LIBIBVERBS)
  message(FATAL_ERROR "ibverbs library not found")
endif()

target_link_libraries(rdma INTERFACE ${LIBIBVERBS})

set_target_properties(rdma PROPERTIES
	PUBLIC_HEADER include/rdmaio.h)
target_include_directories(rdma PRIVATE include/)

set(CMAKE_INSTALL_PREFIX "..")
INSTALL(TARGETS rdma
        ARCHIVE DESTINATION "lib"
        )
