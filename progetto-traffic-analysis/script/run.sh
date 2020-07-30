#!/bin/bash

# cmd syntax: <output path> <input path>

THIS_DIR=$PWD
TIE_PATH=$HOME/TIE
OUT_PATH=$1
IN_PATH=$2

if [[ "$1" == "${1#/*}" ]]; then # If path is relative, convert it to absolute
	OUT_PATH=$THIS_DIR/$1;
fi

if [[ "$2" == "${2#/*}" ]]; then # If path is relative, convert it to absolute
	IN_PATH=$THIS_DIR/$2;
fi

if [ "$OUT_PATH" == "$THIS_DIR/output" ]; then
	echo "error: output path conflicts with TIE's output path; try a different path";
	exit;
fi

RAW_QTUPLES_FILE="tshark_qtuples"
RAW_DNS_FILE="tshark_dns"
FINAL_QTUPLES_FILE="qtuples.myfinal"
FINAL_DNS_FILE="dns.myfinal"

if [ -d $IN_PATH ]; then
	if [ -d $OUT_PATH ]; then
		echo "error: output folder already exists";
	else
		mkdir -p $OUT_PATH;
		for file in $IN_PATH/*; do
			if [ -d $file ]; then
				mkdir -p $OUT_PATH/${file##*/};
				echo "classifying file ${file/*.pcap}...";
				$TIE_PATH/bin/TIE -a ndping_1.0 -r $file/traffic.pcap;
				mv $THIS_DIR/output/class.tie $OUT_PATH/${file##*/}/traffic.tie;
				mv $THIS_DIR/output/report.txt $OUT_PATH/${file##*/}/traffic_report.txt;
				cp $file/strace.log $OUT_PATH/${file##*/}/strace.log;
				cp $file/traffic.pcap $OUT_PATH/${file##*/}/traffic.pcap;
				
				$THIS_DIR/run_tshark.sh $OUT_PATH/${file##*/}/$RAW_QTUPLES_FILE $OUT_PATH/${file##*/}/$RAW_DNS_FILE $OUT_PATH/${file##*/}/traffic.pcap
				python3.6 $THIS_DIR/python/script.py $OUT_PATH/${file##*/}/$FINAL_QTUPLES_FILE $OUT_PATH/${file##*/}/$FINAL_DNS_FILE $OUT_PATH/${file##*/}/$RAW_QTUPLES_FILE $OUT_PATH/${file##*/}/$RAW_DNS_FILE $OUT_PATH/${file##*/}/traffic.tie $OUT_PATH/${file##*/}/strace.log $OUT_PATH/ipinfo.common
				rm -rf $THIS_DIR/output;
				rm -rf $OUT_PATH/${file##*/}/*.mytemp
			fi
		done
	fi
else
	echo "error: could not find folder $IN_PATH";
fi
