#!/usr/bin/python
import sys
import data_util
from optparse import OptionParser
from decimal import *

USAGE = "usage: %prog [options] configFile"
CONFIG_DELIM = "\t"

oParser = OptionParser(usage=USAGE)

oParser.add_option("-i", "--inputFile", default="-", help="input file. [default: stdin]")
oParser.add_option("-d", "--delim", dest="delim", default="|", help="input delimiter. [default: %default]")
oParser.add_option("-o", "--outputFile", default="-", help="output file. [default: stdout]")
oParser.add_option("-t", "--outputDelim", dest="outputDelim", default=",", help="output delimiter. [default: %default]")
oParser.add_option("-q", "--quantize", dest="quantize", default="0.0000", help="preceision of percent. [default: %default]")
oParser.add_option("-p", "--prettyPrint", action="store_true", dest="prettyPrint", default=False, help="pretty print. [default: %default]")

(options,args) = oParser.parse_args()

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

lines = fin.readline()

labels = {}
cnts = {}
expected_size = len(headerList)

for i in range(0,expected_size):
  field = headerList[i]
  labels[i] = field
  cnts[i] = 0

total = 0
nonConform = 0


for line in fin:
  parsed = line.strip('\n').strip('\r').split(options.delim)
  if len(parsed) != expected_size:
    nonConform += 1
  else:
    for i in range(0,len(parsed)):
       if parsed[i].strip() != "":
         cnts[i] = cnts[i] + 1
    total += 1
fin.close()

outputHeader = ["FIELD", "MISSING", "POPULATED", "PERCENT_POPULATED"]

keyFieldLen = max(max(map(lambda l: len(l), headerList)), len(outputHeader[0]))
popFieldLen = max(max(map(lambda n: len(str(n)), cnts)), len(outputHeader[2]))
misFieldLen = max([popFieldLen, len(str(total)), len(outputHeader[1])])
perFieldLen = max(len(options.quantize), len(outputHeader[3]))

if (options.prettyPrint):
  outputHeader[0] = outputHeader[0].rjust(keyFieldLen,' ')
  outputHeader[1] = outputHeader[1].rjust(misFieldLen,' ')
  outputHeader[2] = outputHeader[2].rjust(popFieldLen,' ')
  outputHeader[3] = outputHeader[3].rjust(perFieldLen,' ')
print outputHeader

fout.write(options.outputDelim.join(outputHeader) + "\n")

for i in range(0,expected_size):
  field = labels[i]
  pop = cnts[i]
  missing = total - pop
  perc_pop = (Decimal(pop) / Decimal(total)).quantize(Decimal(options.quantize))

  if options.prettyPrint:
    field = str(field).rjust(keyFieldLen,' ')
    missing = str(missing).rjust(misFieldLen,' ')
    pop = str(pop).rjust(popFieldLen,' ')
    perc_pop = str(perc_pop).rjust(perFieldLen,' ')

  fout.write(options.outputDelim.join([field, str(missing), str(pop), str(perc_pop)]) + "\n")
fout.write("Total Records: %s\n" % total)
fout.write("Non-confirming Records: %s\n" % nonConform)
fout.close()

