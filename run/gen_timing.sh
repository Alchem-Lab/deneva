#!/bin/bash

OUT_PATH=..
awk '/TXN TIMING ENDS/{start = 0} {if (start == 1) print $0} /TXN TIMING STARTS/{start = 1}' $OUT_PATH/0.out $OUT_PATH/1.out $OUT_PATH/2.out $OUT_PATH/3.out > txn_timing.txt
awk '/TXN TIMING ENDS/{start = 0} {if (start == 1) print $0} /TXN TIMING STARTS/{start = 1}' $OUT_PATH/4.out > txn_elapsed.txt
