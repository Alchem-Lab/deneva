# what flags you want to pass to the C compiler & linker
AM_CXXFLAGS = --std=gnu++11 -O2 -pthread -libverbs -I$(top_srcdir)/include
lib_LTLIBRARIES = librdma.la

include_HEADERS = ../include/rdmaio.h ../include/rdma_header.h ../include/simple_map.h

librdma_la_SOURCES = rdma_ctrl.cc qp.cc rc.cc uc.cc ud.cc
librdma_la_LDFLAGS = -version-info 0:9:0 
