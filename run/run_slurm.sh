#!/bin/bash

if [ x$1 == 'x' -o x$2 == 'x' ]; then
	echo "Two arguments required!"
	echo "argument-1 is the number of nodes in the cluster including both servers and clients." 
	echo "argument-2 is the number of server nodes."
	exit
fi

if [ x$3 == 'x' ]; then
    salloc -N $1 -t 00:05:00 ./run.sh $2
else
    salloc -N $1 -t 00:05:00 --nodelist=$3 ./run.sh $2 
fi
