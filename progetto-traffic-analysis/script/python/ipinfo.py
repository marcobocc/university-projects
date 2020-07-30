#python3.6

import csv
import subprocess
import ipaddress

def whatorg(ip):
	orgname = 'UNKNOWN_ORGNAME'
	if ip:
		if ipaddress.ip_address(ip).is_private:
			orgname = 'PRIVATE_ADDR'
		else:
			sp = subprocess.Popen(["whois %s |grep -i -m 1 'orgname\|org-name\|net-name\|descr'" % (ip)], stdout=subprocess.PIPE, shell=True)
			spstdout = sp.communicate()[0].decode('utf-8')
			if spstdout:
				orgname = spstdout.split(':')[1]
				orgname = orgname.lstrip()
				orgname = orgname.rstrip()
	return orgname

def whathost(ip):
	hostname = 'UNKNOWN_HOSTNAME'
	if ip:
		if ipaddress.ip_address(ip).is_private:
			hostname = 'PRIVATE_ADDR'
		sp = subprocess.Popen(["dig +short @8.8.8.8 -x %s" % (ip)], stdout=subprocess.PIPE, shell=True)
		spstdout = sp.communicate()[0].decode('utf-8')
		if spstdout:
			hostname = spstdout.replace('\n', ';').strip()
	return hostname

def add_qtuples_ipinfo(outfilename, qtuplesfilename, dnsfilename, ipcachefilename):
	with open(qtuplesfilename, 'r') as qtfile:
		with open(dnsfilename, 'r') as dnsfile:
			qtreader = csv.reader(qtfile, delimiter='\t', quoting=csv.QUOTE_NONE)
			dnsreader = csv.reader(dnsfile, delimiter='\t', quoting=csv.QUOTE_NONE)
			iplist = []
			iphost = []
			iporg = []
			try:
				cachefile = open(ipcachefilename, 'r')
				ipcachereader = csv.reader(cachefile, delimiter='\t', quoting=csv.QUOTE_NONE)
				for cached in ipcachereader:
					iplist.append(cached[0])
					iphost.append(cached[1])
					iporg.append(cached[2])
				cachefile.close()
			except IOError:
				print('could not find ipinfo file cache')
				print('generating ipinfo file cache...')
			must_update_cache = False
			with open(outfilename, 'w') as outfile:
				fwriter = csv.writer(outfile, delimiter='\t', quoting=csv.QUOTE_NONE)
				for line in qtreader:
					qtuple_ip = line[2]
					qtuple_iphost = ''
					qtuple_iporg = ''
					missing_info = True
					for i in range(0, len(iplist)):
						if qtuple_ip == iplist[i]:
							qtuple_iphost = iphost[i]
							qtuple_iporg = iporg[i]
							missing_info = False
							break
					if missing_info:
						missing_dnsline = True
						for dnsline in dnsreader:
							if dnsline[0] == qtuple_ip:
								qtuple_iphost = dnsline[1]
								missing_dnsline = False
								break
						if missing_dnsline:
							qtuple_iphost = whathost(qtuple_ip)
						qtuple_iporg = whatorg(qtuple_ip)
						iplist.append(qtuple_ip)
						iphost.append(qtuple_iphost)
						iporg.append(qtuple_iporg)
						must_update_cache = True
					fwriter.writerow(line + [qtuple_iphost, qtuple_iporg])
			if must_update_cache:
				print('updating cache with new entries...')
				with open(ipcachefilename, 'w') as cachefile:
					cachewriter = csv.writer(cachefile, delimiter='\t', quoting=csv.QUOTE_NONE)
					for i in range(0, len(iplist)):
						cachewriter.writerow([iplist[i], iphost[i], iporg[i]])
			else:
				print('ipinfo file cache does not need updating')
	return
