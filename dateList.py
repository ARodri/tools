#!/usr/bin/python

import sys
from optparse import OptionParser
from datetime import datetime,timedelta

INCREMENT_DAYS = 1
INPUT_FORMAT="%Y-%m-%d"
OUTPUT_FORMAT="%Y-%m-%d"

parser = OptionParser(usage="usage: %prog [options] startDate endDate")
parser.add_option('-i','--inputFormat',metavar="FORMAT",default='%Y-%m-%d',help="Python date format of input. [default: %default]")
parser.add_option('-o','--outputFormat',metavar="FORMAT",default='%Y-%m-%d',help="Python date format of output. [default: %default]")
parser.add_option('-s','--startDate',metavar="DATE",default='Now',help="Start date. [default: %default]")
parser.add_option('-e','--endDate',metavar="DATE",default=False,help="Ending date.")
parser.add_option('-c','--incrementBy',metavar="INT",default=1,help="Increment value. [default: %default]")
parser.add_option('-t','--incrementType',metavar="TYPE",default="days",help="Increment type. Value valids: days, weeks, months, years. [default: %default]")
parser.add_option('-n','--increments',metavar="INT",default=sys.maxint,help="Maximum number of increments. [default: %default]")

(options,args) = parser.parse_args()
if len(args) != 2:
	parser.print_help()
	sys.exit(1)
def getDelta(typeStr,incrementBy):
	inc = int(incrementBy)
	if typeStr == "days":
		timedelta(days=inc)
	else:
		#throw Exception 'Unsupported increment type [%s]' % (typeStr)
		1/0
start = datetime.strptime(args[0],options.inputFormat)
end = datetime.strptime(args[1],options.inputFormat)
delta = getDelta(options.incrementType, options.incrementBy)



cur = start
num = 0
while cur <= end and num < int(options.increments):
	print datetime.strftime(cur,options.outputFormat)
	cur += timedelta(days=int(options.incrementBy))
	num += 1
