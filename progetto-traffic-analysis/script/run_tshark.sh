#!/bin/bash

# cmd syntax: <out tshark file> <out dns file> <pcap file>

THIS_DIR=$PWD
OUT_FILE=$1
DNS_FILE=$2
PCAP_FILE=$3

tshark -r "$PCAP_FILE" -T fields -e tcp.stream -e ip.src -e ip.dst -e ip.proto -e tcp.srcport -e tcp.dstport -e udp.srcport -e udp.dstport -e ssl.handshake.extensions_server_name -e http.host > "$OUT_FILE"
tshark -r "$PCAP_FILE" -T fields -e dns.a -e dns.qry.name -Y "(dns.flags.response == 1 )" > "$DNS_FILE"
