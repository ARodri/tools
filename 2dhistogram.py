#!/usr/bin/python

import sys, math
from decimal import *
from optparse import OptionParser

USAGE = "usage: %prog [options]"

oParser = OptionParser(usage=USAGE)
oParser.add_option("-p", "--prettyPrint", action="store_true", dest="prettyPrint", default=False, help="pretty print. [default: %default]")
oParser.add_option("-d", "--delim", default="\t", dest="delimiter", help="output delimiter: [Default: tab]")
oParser.add_option("-f", "--fieldDelim", default="|", dest="fieldDelim", help="field delimiter. [Default: %default]")
oParser.add_option("-k", "--keyField", default="1", dest="keyField", help="field to key on for 2d histogram. [Default: %default]")
oParser.add_option("-e", "--mapBinToEmpty", default=False, action="store_true", dest="mapBinToEmpty", help="map contents of non-key fields to 1 if not empty, 0 if empty. [Default %default]")
oParser.add_option("-E", "--mapKeyToEmpty", default=False, action="store_true", dest="mapKeyToEmpty", help="map conents of key fields to 1/0 if empty/non-empty. [Default: %default]")
oParser.add_option("-b", "--binary", default=False, action="store_true", dest="binary", help="assumes value is 1 or 0, prints only % of 0")

(options,args) = oParser.parse_args()

data = {}
allValues = set([])

if options.binary:
	allValues.add("1")

options.keyField = int(options.keyField)

for line in sys.stdin:
	vals = line.strip('\n').strip('\r').split(options.fieldDelim)
	key = vals.pop(options.keyField - 1)
	
	key = key if not options.mapKeyToEmpty else ( '1' if key != "" else "0" )

	if (options.mapBinToEmpty):
		vals = [ '1' if v != "" else '0' for v in vals ]
	valStr = options.fieldDelim.join(vals)
	
	if key not in data:
		data[key] = {}
	data[key][valStr] = data[key].get(valStr,0) + 1
	if not options.binary:
		allValues.add(valStr)

print data

sortedKeys = data.keys()
sortedKeys.sort()

sortedAllValues = list(allValues)
sortedAllValues.sort()

header = ["KEY", "BIN", "FREQ", "BIN_%", "CUME", "1-CUME"]

maxKeyLen = max([ len(str(k)) for k in data.keys() ] + [len(header[0])])
maxValLen = max([ len(str(v)) for v in data[k] for k in data.keys() ] + [len(header[1])])
maxFreqLen = max([ len(str(data[k][v])) for v in data[k].keys() for k in data.keys() ] + [len(header[2])])

if (options.prettyPrint):
	header = ["KEY".rjust(maxKeyLen,' '), "BIN".rjust(maxValLen, ' '), "FREQUENCY".rjust(maxFreqLen, ' '), "BIN_%", "CUME", "1-CUME"]

print(options.delimiter.join(header))
for key in sortedKeys:
	cumeSeen = 0
	total = sum([ data[key][val] for val in data[key] ])
	for val in sortedAllValues:
		freq = data[key].get(val, 0)
		
		cumeSeen += freq
		binPerc = (Decimal(freq) / total).quantize(Decimal('0.0000'))
		cumePerc = (Decimal(cumeSeen) / total).quantize(Decimal('0.0000'))
		m1CumePerc = 1-cumePerc
		if (options.prettyPrint):
			key = str(key).rjust(maxKeyLen,' ')
			val = str(val).rjust(maxValLen, ' ')
			freq = str(freq).rjust(maxFreqLen, ' ')
			binPerc = str(binPerc).rjust(max(len(header[3]), len(str(binPerc))), ' ')
			cumePerc = str(cumePerc).rjust(max(len(header[4]), len(str(cumePerc))), ' ')
			m1CumePerc = str(m1CumePerc).rjust(max(len(header[5]), len(str(m1CumePerc))), ' ')
			
		print(options.delimiter.join([ str(v) for v in [ key,val,freq,binPerc,cumePerc,m1CumePerc ] ]))
	if (options.prettyPrint):
		print ""
print data
