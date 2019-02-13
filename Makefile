#CC=g++
CC=c++
CFLAGS=-Wall -g -gdwarf-3 -std=c++0x 
#CFLAGS += -fsanitize=address -fno-omit-frame-pointer
ROCC_ROOT=$(HOME)/git_repos/rocc/
BOOST=$(ROCC_ROOT)/third_party/boost/
JEMALLOC=$(HOME)/install/jemalloc-5.1.0/
NNMSG=$(HOME)/install/nanomsg-1.1.4/
#RDMA=./deps/librdma-0.9.0-install/
RDMA=./deps/libRDMA/

.SUFFIXES: .o .cpp .h

SRC_DIRS = ./ ./benchmarks/ ./client/ ./concurrency_control/ ./storage/ ./transport/ ./system/  ./core/utils/ ./core/ ./rtx/ ./statistics/ ./util/ #./unit_tests/
DEPS = -I. -I./benchmarks -I./client/ -I./concurrency_control -I./storage -I./transport -I./system -I./statistics -I$(BOOST)/include -I$(JEMALLOC)/include -I$(NNMSG)/include -I$(RDMA)/include #-I./unit_tests 

CFLAGS += $(DEPS) -std=c++0x -Wall -Werror -Wno-sizeof-pointer-memaccess -DNOGRAPHITE=1 -DLEVELDB_PLATFORM_POSIX -DOS_LINUX -pthread -mrtm
LDFLAGS = -L. -L$(BOOST)/lib -L$(NNMSG)/lib64 -L$(JEMALLOC)/lib -L$(RDMA)/lib -Wl,-rpath,$(JEMALLOC)/lib -Wl,-rpath,$(RDMA)/lib -Wl,-rpath,$(NNMSG)/lib64 -gdwarf-3 -rdynamic 
LDFLAGS += $(CFLAGS)

BOOSTLIBS=$(BOOST)/lib/libboost_coroutine.a $(BOOST)/lib/libboost_chrono.a $(BOOST)/lib/libboost_thread.a $(BOOST)/lib/libboost_context.a $(BOOST)/lib/libboost_system.a
LIBS =-lrdma -lssmalloc -lnanomsg -lanl -ljemalloc -ldl -rdynamic -lzmq -lrt -libverbs $(BOOSTLIBS)

DB_MAINS = ./client/client_main.cpp ./system/sequencer_main.cpp ./unit_tests/unit_main.cpp
CL_MAINS = ./system/main.cpp ./system/sequencer_main.cpp ./unit_tests/unit_main.cpp
UNIT_MAINS = ./system/main.cpp ./client/client_main.cpp ./system/sequencer_main.cpp

CPPS_DB = $(foreach dir,$(SRC_DIRS),$(filter-out $(DB_MAINS), $(wildcard $(dir)*.cpp)))
CCS_DB = $(foreach dir,$(SRC_DIRS),$(filter-out $(DB_MAINS), $(wildcard $(dir)*.cc)))

CPPS_CL = $(foreach dir,$(SRC_DIRS),$(filter-out $(CL_MAINS), $(wildcard $(dir)*.cpp)))
CCS_CL = $(foreach dir,$(SRC_DIRS),$(filter-out $(CL_MAINS), $(wildcard $(dir)*.cc)))

CPPS_UNIT = $(foreach dir,$(SRC_DIRS),$(filter-out $(UNIT_MAINS), $(wildcard $(dir)*.cpp)))
CCS_UNIT = $(foreach dir,$(SRC_DIRS),$(filter-out $(UNIT_MAINS), $(wildcard $(dir)*.cc)))

#CPPS = $(wildcard *.cpp)
OBJS_DB = $(addprefix obj/, $(notdir $(CPPS_DB:.cpp=.o)) $(notdir $(CCS_DB:.cc=.o)))
OBJS_CL = $(addprefix obj/, $(notdir $(CPPS_CL:.cpp=.o)) $(notdir $(CCS_CL:.cc=.o)))
OBJS_UNIT = $(addprefix obj/, $(notdir $(CPPS_UNIT:.cpp=.o)) $(CCS_UNIT:.cc=.o)))

#NOGRAPHITE=1

all:rundb runcl 
#unit_test

.PHONY: deps_db
deps:$(CPPS_DB)
	$(CC) $(CFLAGS) -MM $^ > obj/deps
	sed '/^[^ ]/s/^/obj\//g' obj/deps > obj/deps.tmp
	mv obj/deps.tmp obj/deps
-include obj/deps

unit_test :  $(OBJS_UNIT)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)
./obj/%.o: transport/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: unit_tests/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: benchmarks/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: storage/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: system/%.cpp
	$(CC) -c -DSTATS_ENABLE=false $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: concurrency_control/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: client/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: rtx/%.cc
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: core/utils/%.cc
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: core/%.cc
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: util/%.cc
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: %.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<


rundb : $(OBJS_DB)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)
./obj/%.o: transport/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
#./deps/%.d: %.cpp
#	$(CC) -MM -MT $*.o -MF $@ $(CFLAGS) $<
./obj/%.o: benchmarks/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: storage/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: system/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: statistics/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: concurrency_control/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: client/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: rtx/%.cc
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: core/utils/%.cc
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: core/%.cc
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: util/%.cc
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: %.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<


runcl : $(OBJS_CL)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)
#	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)
./obj/%.o: transport/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
#./deps/%.d: %.cpp
#	$(CC) -MM -MT $*.o -MF $@ $(CFLAGS) $<
./obj/%.o: benchmarks/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: storage/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: system/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: statistics/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: concurrency_control/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: client/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: rtx/%.cc
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: core/utils/%.cc
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: core/%.cc
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: util/%.cc
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: %.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<

.PHONY: clean
clean:
	rm -f obj/*.o obj/.depend rundb runcl runsq unit_test
