#!/usr/bin/python

import sys, math
from decimal import *
from optparse import OptionParser

USAGE = "usage: %prog [options]"

oParser = OptionParser(usage=USAGE)
oParser.add_option("-n", "--numeric", action="store_true", dest="isNumeric", default=False, help="treat input as numeric. [default: %default]")
oParser.add_option("-f", "--sortByFrequency", action="store_true", dest="sortByFrequency", default=False, help="sort by frequency. [default: %default]")
oParser.add_option("-p", "--prettyPrint", action="store_true", dest="prettyPrint", default=False, help="pretty print. [default: %default]")
oParser.add_option("-d", "--delim", default="\t", dest="delimiter", help="output delimiter: [Default: tab]")
oParser.add_option("-b", "--bin", default="1", dest="bin", help="bin numeric values by X: [Default: %default]")
oParser.add_option("-r", "--reverse", action="store_true", dest="reverseSort", default=False, help="sort output in reverse. [default: %default]")
oParser.add_option("-s", "--summary", action="store_true", dest="printSummary", default=False, help="output summary statistics (min,max,median,mode,mean,stddev). Note: calculations are done prior to any binning. [default: %default]")
oParser.add_option("-q", "--disableHistogram", action="store_false", dest="printHistogram", default=True, help="output histogram. [default: True]")

(options,args) = oParser.parse_args()

nBin = Decimal(options.bin)

data = {}

def sortData(unsortedData, byFrequency, isNumeric, reverseSort):
	sortFunction = None
	sortedData = None

	if byFrequency:
		sortFunction = lambda tup: int(tup[1])
	else:
		if (isNumeric):
			sortFunction = lambda tup: Decimal(tup[0])
		else:
			sortFunction = lambda tup: str(tup[0])
		
	sortedData = sorted(unsortedData.items(), key=sortFunction)
	if (reverseSort):
		sortedData.reverse()
	return sortedData


for line in sys.stdin:
	val = line.strip('\n').strip('\r')
	if (options.isNumeric):
		nVal = (Decimal(val) - (Decimal(val) % nBin))
		data[nVal] = data.get(nVal,0) + 1
	else:
		data[val] = data.get(val,0) + 1

total = Decimal(sum(data.values()))

if (options.printHistogram):
	sortedData = sortData(data, options.sortByFrequency, options.isNumeric, options.reverseSort)
	
	maxValLen = max(map(lambda tup: len(str(tup[0])), sortedData))
	maxFreqLen = max(map(lambda tup: len(str(tup[1])), sortedData))
	
	header = ["BIN", "FREQUENCY", "BIN %", "CUME", "1-CUME"]
	if (options.prettyPrint):
		header = ["BIN".rjust(maxValLen, ' '), "FREQUENCY".rjust(maxFreqLen, ' '), "BIN %", "CUME", "1-CUME"]
	
	print(options.delimiter.join(header))
	cumeSeen = 0
	for (val,freq) in sortedData:
		cumeSeen += freq
		binPerc = (Decimal(freq) / total).quantize(Decimal('0.0000'))
		cumePerc = (Decimal(cumeSeen) / total).quantize(Decimal('0.0000'))
		m1CumePerc = 1-cumePerc
		if (options.prettyPrint):
			val = str(val).rjust(len(header[0]), ' ')
			freq = str(freq).rjust(len(header[1]), ' ')
			binPerc = str(binPerc).rjust(max(len(header[2]), len(str(binPerc))), ' ')
			cumePerc = str(cumePerc).rjust(max(len(header[3]), len(str(cumePerc))), ' ')
			m1CumePerc = str(m1CumePerc).rjust(max(len(header[4]), len(str(m1CumePerc))), ' ')
		
		print(options.delimiter.join([str(val),str(freq), str(binPerc),str(cumePerc),str(m1CumePerc)]))

print ""
if (options.printSummary):
	print "Summary"
	print "================================="
	valueSorted = sortData(data, False, options.isNumeric, False)
	minValue = None
	maxValue = None
	medianValue = None
	meanValue = None
	modeValue = None
	modeBinCnt = None
	modeBinPct = None
	stdDev = None

	#  Get min/max
	if (len(data) > 0):
		minValue = valueSorted[0][0]
		maxValue = valueSorted[-1][0]

	# Get median
	medianTarget = int(total)/2
	medianCnt = 0
	for (val, cnt) in valueSorted:
		medianCnt += cnt
		if (medianCnt >= medianTarget):
			medianValue = val
			break
	
	# get mode
	
	(modeValue,modeBinCnt) = sortData(data, True, options.isNumeric, True)[0]
	modeBinPct = ((Decimal(modeBinCnt) / Decimal(total)) * 100).quantize(Decimal('00.00'))

	# mean/stddev
	if (options.isNumeric):
		# mean
		meanValue = Decimal(sum(map(lambda tup: tup[0]*tup[1], valueSorted)))/Decimal(total)
		stdDev = math.sqrt( (1/Decimal(total)) * Decimal(sum( map(lambda tup: math.pow(tup[0]-meanValue,2)*tup[1] , valueSorted))))
	print("Total Records: %s" % str(total))
	print("Min: [%s]" % str(minValue))
	print("Max: [%s]" % str(maxValue))
	print("Median: [%s]" % str(medianValue))
	print("Mode: [%s] - %s (%s%%)" % (str(modeValue), str(modeBinCnt), str(modeBinPct)))
	print("Mean: %s" % str(meanValue))
	print("StdDev: %s" % str(stdDev))

