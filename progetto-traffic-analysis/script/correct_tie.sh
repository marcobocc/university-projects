#!/bin/bash

# cmd syntax: <path>

THIS_DIR=$PWD
TIE_PATH=$HOME/TIE
OUT_PATH=$1

if [[ "$1" == "${1#/*}" ]]; then # If path is relative, convert it to absolute
	OUT_PATH=$THIS_DIR/$1;
fi

if [ -d $OUT_PATH ]; then
	for file in $OUT_PATH/*; do
		if [ -d $file ]; then
			python3.6 $THIS_DIR/python/gt.py $OUT_PATH/${file##*/}/traffic.gt.tie $OUT_PATH/${file##*/}/validated.txt $OUT_PATH/${file##*/}/traffic.tie
		fi
	done
else
	echo "error: could not find folder $OUT_PATH";
fi
