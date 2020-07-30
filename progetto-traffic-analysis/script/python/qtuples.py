#python3.6

import csv
import subprocess
import ipinfo

def extract_ips(ipstring):
	return ipstring.split(',')

# -T fields -e dns.a -e dns.qry.name
def preprocess_dns(outfilename, dnsfilename):
	with open(dnsfilename, 'r') as dnsfile:
		with open(outfilename, 'w') as outfile:
			dnsreader = csv.reader(dnsfile, delimiter='\t', quoting=csv.QUOTE_NONE)
			fwriter = csv.writer(outfile, delimiter='\t', quoting=csv.QUOTE_NONE)
			print('preprocessing dns file...')
			for line in dnsreader:
				iplist = extract_ips(line[0])
				for ip in iplist:
					fwriter.writerow([ip, line[1]])
	return

# arglist: [ip.proto, tcp.srcport, tcp.dstport, udp.srcport, udp.dstport]
def proto_ports(arglist):
	proto = arglist[0]
	srcport = ''
	dstport = ''	
	if proto == '6':
		srcport = arglist[1]
		dstport = arglist[2]
	elif proto == '17':
		srcport = arglist[3]
		dstport = arglist[4]
	return [proto, srcport, dstport]

# -e tcp.stream -e ip.src -e ip.dst -e ip.proto -e tcp.srcport -e tcp.dstport -e udp.srcport -e udp.dstport -e ssl.handshake.extensions_server_name -e http.host
# [tcp.stream, ip.src, ip.dst, ip.proto, srcport, dstport, ssl.handshake.extensions_server_name, http.host]
def preprocess_qtuples(outfilename, qtuplesfilename):
	uniqueids = []
	tcppackets = []
	outbuffer = []
	with open(qtuplesfilename, 'r') as qtfile:
		qtreader = csv.reader(qtfile, delimiter='\t', quoting=csv.QUOTE_NONE)
		for packet in qtreader:
			if packet[0]:
				tcppackets.append(packet)
			else:
				outbuffer.append([packet[0], packet[1], packet[2], *proto_ports(packet[3:8]), packet[8], packet[9]])
	print('preprocessing quintuples...')
	for stream in tcppackets:
		if not stream[0] in uniqueids:
			snivalues = ""
			httphost = ""
			for anypacket in tcppackets:
				if stream[0] == anypacket[0]:
					if anypacket[8]:
						snivalues = snivalues + anypacket[8] + ';'
					if anypacket[9]:
						httphost = stream[7]
			uniqueids.append(stream[0])
			outbuffer.append([stream[0], stream[1], stream[2], *proto_ports(stream[3:8]), snivalues, httphost])
	with open(outfilename, 'w') as outfile:
		fwriter = csv.writer(outfile, delimiter='\t', quoting=csv.QUOTE_NONE)
		for data in outbuffer:
			fwriter.writerow(data)
	return

def are_matching(lqtuple, rqtuple):
	if ((lqtuple[0] == rqtuple[0] and lqtuple[1] == rqtuple[1] and lqtuple[3] == rqtuple[3] and lqtuple[4] == rqtuple[4]) or\
		(lqtuple[0] == rqtuple[1] and lqtuple[1] == rqtuple[0] and lqtuple[3] == rqtuple[4] and lqtuple[4] == rqtuple[3])) and\
		(lqtuple[2] == rqtuple[2]):
		return True
	return False

def get_strace(qtuple, stracefilename):
	result = 'UNKNOWN_PROCESS'
	src = '%s:%s' % (qtuple[0], qtuple[3])
	dst = '%s:%s' % (qtuple[1], qtuple[4])
	cmd = "grep -m 1 -E '%s.*%s|%s.*%s' %s" % (src, dst, dst, src, stracefilename)
	sp = subprocess.Popen([cmd], stdout=subprocess.PIPE, shell=True)
	spstdout = sp.communicate()[0].decode('utf-8')
	if spstdout:
		result = spstdout.split()[0]
	return result

def add_qtuples_data(outfilename, qtuplesfilename, tiefilename, stracefilename):
	outbuffer = []
	with open(qtuplesfilename, 'r') as qtfile:
		qtreader = csv.reader(qtfile, delimiter='\t', quoting=csv.QUOTE_NONE)
		print('matching quintuples with TIE and strace...')
		for qtuple in qtreader:
			qtuple.append('NO_TIE_ID')
			qtuple.append('UNKNOWN')
			qtuple.append(get_strace(qtuple[1:6], stracefilename))
			outbuffer.append(qtuple)		
	with open(tiefilename, 'r') as tiefile:
		tiereader = csv.reader(tiefile, delimiter='\t', quoting=csv.QUOTE_NONE)
		for line in tiereader:
			if line and not line[0].startswith('#'):
				for i in range(0, len(outbuffer)):
					if are_matching(outbuffer[i][1:6], line[1:6]):
						outbuffer[i][8] = line[0]
						outbuffer[i][9] = line[14]
						break
	with open(outfilename, 'w') as outfile:
		fwriter = csv.writer(outfile, delimiter='\t', quoting=csv.QUOTE_NONE)
		for data in outbuffer:
			fwriter.writerow(data)
	return
