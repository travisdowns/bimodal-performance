#!/bin/bash

rm -f offset.log


set -e

w=6

first_start=14
second_start=0
max_first=3000
max_second=2048
incr=64

second_seq=`seq 0 ${incr} $max_second` 

printf "%${w}s" ""
for second in $second_seq; do
	printf "%${w}d" $second
done
echo ""

for first in `seq 0 ${incr} $max_first`; do
	printf "%${w}d" $first
	for second in $second_seq; do
		touch weirdo.asm && make LDFLAGS=-fuse-ld=gold ASM_FLAGS="-DFIRSTO=$first -DSECONDO=$second" >> offset.log
		./weirdo-main asm summary 
	done
	echo ""
done