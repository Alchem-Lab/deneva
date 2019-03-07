#!/bin/bash

if [ x$1 == "x" -o x$2 == "x" ]; then
	echo "./run_node.sh SERVER_CNT NODE_IDX [gdb]"
	exit
fi

SERVER_CNT=$1
NODE_IDX=$2

if [ $NODE_IDX -ge $SERVER_CNT ]; then
	if [ x$3 == "xgdb" ]; then
		cd ~/git_repos/deneva/ && gdb --args ./runcl -nid$NODE_IDX 2>&1 | tee ${NODE_IDX}_debug.out
	else
		cd ~/git_repos/deneva/ && ./runcl -nid$NODE_IDX 2>&1 | tee ${NODE_IDX}.out
	fi
else
	if [ x$3 == "xgdb" ]; then
		cd ~/git_repos/deneva/ && gdb --args ./rundb -nid$NODE_IDX 2>&1 | tee ${NODE_IDX}_debug.out
	else
		cd ~/git_repos/deneva/ && ./rundb -nid$NODE_IDX 2>&1 | tee ${NODE_IDX}.out
	fi
fi
