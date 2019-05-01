#!/bin/bash

projectDIR=`pwd`
mkdir -p deps/libRDMA/build/
cd deps/libRDMA/ralloc && make clean && make && make install && cd $projectDIR
cd deps/libRDMA/build/ && make && make install && cd $projectDIR
mkdir -p obj/
make clean && make -j
