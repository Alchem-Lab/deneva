#!/bin/bash

cd deps/libRDMA/build/ && make && make install && cd ~/git_repos/deneva/ && make clean && make -j
