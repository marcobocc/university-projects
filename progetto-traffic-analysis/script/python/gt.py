#python3.6

import sys
import csv

def correct_tie(outfilename, qtuplesfilename, tiefilename):
	qtuples = []
	outbuffer = []
	with open(qtuplesfilename, 'r') as qtfile:
		qtreader = csv.reader(qtfile, delimiter='\t', quoting=csv.QUOTE_NONE)
		print('fetching quintuples...')
		for qtuple in qtreader:
			qtuples.append(qtuple)		
	with open(tiefilename, 'r') as tiefile:
		tiereader = csv.reader(tiefile, delimiter='\t', quoting=csv.QUOTE_NONE)
		print('correcting TIE file...')
		for line in tiereader:
			if line and not line[0].startswith('#'):
				for qtuple in qtuples:
					if qtuple[0] and qtuple[13] and qtuple[8] == line[0]:
						prev = '#previously:' + line[14]
						line[14] = qtuple[13]
						line.append(prev)
						break
			outbuffer.append(line)
	with open(outfilename, 'w') as outfile:
		fwriter = csv.writer(outfile, delimiter='\t', quoting=csv.QUOTE_NONE)
		for data in outbuffer:
			fwriter.writerow(data)
	return

correct_tie(sys.argv[1], sys.argv[2], sys.argv[3])
