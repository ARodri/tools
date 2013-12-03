#!/usr/bin/python

import sys
import data_util
import re
from datetime import datetime
from optparse import OptionParser
from operator import itemgetter
from itertools import groupby

USAGE = "usage: %prog [options] configFile"
CONFIG_DELIM = "\t"

oParser = OptionParser(usage=USAGE)

oParser.add_option("-i", "--inputFile", default="-", help="input file. [default: stdin]")
oParser.add_option("-d", "--delim", dest="delim", default="|", help="input delimiter. [default: %default ]")
oParser.add_option("-o", "--outputFile", default="-", help="output file. [default: stdout]")

(options,args) = oParser.parse_args()

if (len(args) != 1):
	oParser.error("Missing configuration file")

# [(inputField,outputField) -> [(regex,inputFormat,outputFormat)]]
def parseConfig(configfile):
	config = {}
	lines = []
	cfile = open(configfile,'r')
	(headerList, parser) = data_util.makeParser(cfile.readline(), CONFIG_DELIM)
	for line in filter(lambda l: l[0] != "#", cfile.readlines()):
		(parsed, hadError, error) = parser(line)
		if not hadError:
			inputField = parsed['inputField']
			outputField = parsed['outputField']
			regex = re.compile(parsed['matchRegex'])
			inputFormat = parsed['inputDateFormat']
			outputFormat = parsed['outputDateFormat']
			configKey = (inputField, outputField)
			if not configKey in config:
				config[configKey] = []
			config[configKey].append((regex, inputFormat, outputFormat))
		else:
			print(error)
			sys.exit(1)
	cfile.close()
	newFields = list(set(map(lambda tup: tup[1], config.keys())))
	return (sorted(newFields), config.items())

# Parsedg
(newFields, configs) = parseConfig(args[0])
# Open file handlers
#  Handle - for sys.stdout/stdin
fin = None
if (options.inputFile == '-'):
	fin = sys.stdin
else:
	fin = open(options.inputFile,'r')

fout = None
if options.outputFile == '-':
	fout = sys.stdout
else:
	fout = open(options.outputFile,'w')

# Construct readers and writers
headerStr = fin.readline()
(headerList, parser) = data_util.makeParser(headerStr,options.delim)
headerList = headerList + newFields

writer = data_util.makeWriter(headerList, options.delim)


# Do the work
#i = 0
fout.write(options.delim.join(headerList) + "\n")
for line in fin:
	(parsed, hadError, error) = parser(line)
	for ((inputField, outputField), config) in configs:
		dateStr = parsed[inputField]
		outputStr = ''
		for (regex, inputFormat, outputFormat) in config:
			if outputStr == '' and regex.match(dateStr):
				try:
					date = datetime.strptime(dateStr, inputFormat)
					outputStr = date.strftime(outputFormat)
				except ValueError:
					continue
		parsed[outputField] = outputStr
	fout.write(writer(parsed))

# Close files
fin.close()
fout.close()


