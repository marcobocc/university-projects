#python3.6 <out file> <qtuples file> <dns file> <tie file> >strace file> <ipinfo file>

import sys
import qtuples
import ipinfo

if len(sys.argv) >= 7:
	QTUPLES_FINAL_FILE = sys.argv[1]
	DNS_FINAL_FILE = sys.argv[2]
	QTUPLES_IN_FILE = sys.argv[3]
	DNS_IN_FILE = sys.argv[4]
	TIE_IN_FILE = sys.argv[5]
	STRACE_IN_FILE = sys.argv[6]
	IPINFO_IN_FILE = sys.argv[7]

	QTUPLES_PREPROC_FILE = "qtuples.preproc.mytemp"
	QTUPLES_TIESTRACE_FILE = "qtuples.tiestrace.mytemp"

	qtuples.preprocess_dns(DNS_FINAL_FILE, DNS_IN_FILE)
	qtuples.preprocess_qtuples(QTUPLES_PREPROC_FILE, QTUPLES_IN_FILE)
	qtuples.add_qtuples_data(QTUPLES_TIESTRACE_FILE, QTUPLES_PREPROC_FILE, TIE_IN_FILE, STRACE_IN_FILE)
	ipinfo.add_qtuples_ipinfo(QTUPLES_FINAL_FILE, QTUPLES_TIESTRACE_FILE, DNS_FINAL_FILE, IPINFO_IN_FILE)
else:
	print("error: missing arguments")
