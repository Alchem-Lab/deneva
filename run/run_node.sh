#!/bin/bash

if [ $1 == "" -o $2 == "" ]; then
	echo "please provide SERVER_CNT and NODE_IDX!"
	exit;
fi

SERVER_CNT=$1
NODE_IDX=$2

if [ $NODE_IDX -ge $SERVER_CNT ]; then
	if [ x$3 == "xgdb" ]; then
		cd ~/git_repos/deneva/ && gdb --args ./runcl -nid$NODE_IDX 2>&1 | tee ${NODE_IDX}_debug.out
	else
		cd ~/git_repos/deneva/ && ./runcl -nid$NODE_IDX 2>&1 | tee ${NODE_IDX}_debug.out
	fi
else
	if [ x$3 == "xgdb" ]; then
		cd ~/git_repos/deneva/ && gdb --args ./rundb -nid$NODE_IDX 2>&1 | tee ${NODE_IDX}_debug.out
	else
		cd ~/git_repos/deneva/ && ./rundb -nid$NODE_IDX 2>&1 | tee ${NODE_IDX}_debug.out
	fi
fi
