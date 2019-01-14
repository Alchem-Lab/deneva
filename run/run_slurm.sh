#!/bin/bash

if [ x$1 == 'x' -o x$2 == 'x' ]; then
	echo "two arguments required!"
	exit
fi

salloc -N $1 -t 00:30:00 ./run.sh $2
