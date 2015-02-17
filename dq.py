#!/usr/bin/python
import sys, json, re, StringIO, math, itertools, codecs
#import data_util

from optparse import OptionParser
from decimal import *

USAGE = "usage: %prog [options] configFile"

SAMPLE_CONFIG="""
{
    "delim": "|",
    "header": true,
    "fieldList": [],
	
	#built in: cardinality, histogram
	
    "metrics": {
        "populated": [
            {  
                "name": "nonEmpty",
                "missingValues":[""]
            }
        ],
        "numeric": [
            {  
                "name": "noBinning",
                "binning": -1
            }
        ],
        "regex": [
            {  
                "name": "fieldType",
                "regexes": ["^[0-9]+$", "^[a-zA-Z]+$", "^([a-zA-Z]|[0-9])+$"],
                "labels": ["numeric", "alpha", "alphaNumeric"]
            }
        ],
        "histogram": [
			{
				"name": "basic"
			}
        ]
    },
    "fieldMetrics": [
		{ 
			"field":"f1",
			"metrics": [
				"populated.nonEmpty", 
				"histogram",
				"numeric.noBinning",
				"regex.fieldType",
				"cardinality"
			]
		}
	]
	
}
"""

HIST_SORT_FREQ=0
HIST_SORT_VAL=1

def binFloat(val, binning):
	if binning > 0:
		return math.floor(val / binning) * binning
	else:
		return val

class RecordParser:
	delim = None
	fields = None
	expectedNumFields = None
	
	def __init__(self, delim="|", fields=None, headerStr=None):
		
		if fields == headerStr == None:
			raise AttributeError('require either a list of fields or a header string')
		
		self.delim=delim
		self.fields=fields
		if headerStr != None:
			self.fields = headerStr.strip('\n').strip('\r').split(self.delim)
		self.expectedNumFields = len(self.fields)
	
	def parse(self, line):
		pline = line.strip('\n').strip('\r').split(self.delim)
		if len(pline) == self.expectedNumFields:
			return (dict(zip(self.fields, pline)), len(pline))
		else:
			return ({}, len(pline))
	
class Histogram:
	counts = None
	
	total = None
	sortByValue = None
	reverse = None
	
	def __init__(self, startingKeys=[],sortByValue=False, reverse=True):
		self.counts = dict(zip(startingKeys, [0]*len(startingKeys)))
		self.sortByValue=sortByValue
		self.reverse=reverse
	
	def count(self, value):
		if value not in self.counts:
			self.counts[value] = 0
		self.counts[value] += 1
		
	def update(self, value):
		self.count(value)
		
	def overrideTotal(self, newTotal):
		self.total = newTotal
		
	def __str__(self):
		outStr=[]
		# currently only sort by value
		doCume = False
		sorted_items = self.counts.items()
		if self.sortByValue:
			sorted_items.sort(key=lambda x: x[0])
		else:
			sorted_items.sort(key=lambda x: x[1])
		if self.reverse:
			sorted_items.reverse()
			
		if self.total == None:
			self.total = sum(self.counts.values())
			doCume = True
		outStr.append("|".join(['values','count','bin_perc','cume_perc']))
		cume = 0.0
		for key,cnt in sorted_items:
			perc = 0.0
			if self.total != None and self.total != 0:
				perc = float(cnt) / float(self.total)
				if doCume:
					cume += perc
			outStr.append("|".join([str(key), str(cnt), str(perc), str(cume)]))
		return '%s\n' % '\n'.join(outStr)
		
class HistogramFactory:
	jsonConfig = None
	name = "histogram"
	
	def __init__(self, name=None, jsonConfig=None):
		self.jsonConfig = jsonConfig
		self.name = name if name != None else self.name
	
	def produce(self):
		return Histogram()
	
	def fromJSON(self):			
		return self.produce()

class Cardinality:
	values = None
	total = None
	def __init__(self):
		self.values = set([])
		self.total = 0
	def update(self,value):
		self.values.add(value)
		self.total += 1
		
	def __str__(self):
		
		numUnique = len(self.values)
		cardinality = float(numUnique) / float(self.total) if self.total > 0 else 0.0
		
		outStr=[]
		outStr.append("numUnique=%s" % numUnique)
		outStr.append("total=%s" % self.total)
		outStr.append("cardinality=%s" % cardinality)
		
		return '\n'.join(outStr) + '\n'

class CardinalityFactory:
	name = None
	jsonConfig = None
	
	def __init__(self, name=None, jsonConfig=None):
		self.name = name if name != None else "unique"
		self.jsonConfig = jsonConfig
		
	def produce(self):
		return Cardinality()
		
	def fromJSON(self):
		return self.produce()
	

class RegexMatcher:

	regexes = None
	hist = None
	nomatch_key = 'UNMATCHED'

	def __init__(self, regexStrs=[], labels=[]):
		self.regexes = []
		realLabels = regexStrs
		if labels != []:
			if len(regexStrs) != len(labels):
				raise AttributeError('labels are neither empty or the same '+
					'length as regex strings (regexStrs=%s, labels=%s)' % 
					(name, ','.join(regexStrs), ','.join(labels)))
			else:
				realLabels = labels
		
		for i in range(len(regexStrs)):
			self.regexes.append((realLabels[i],re.compile(regexStrs[i])))
		self.hist = Histogram(realLabels + [self.nomatch_key])

	
	def update(self, value):
		matchFound = False
		if value != None:
			for (label, regex) in self.regexes:
				if regex.match(str(value)):
					matchFound = True
					self.hist.count(label)
		if not matchFound:
			self.hist.count(self.nomatch_key)

	def __str__(self):
		return str(self.hist)

class RegexMatcherFactory:
	jsonConfig = None
	name = None
	
	def __init__(self, name=None, jsonConfig=None):
		self.jsonConfig = jsonConfig
		self.name = name if name != None else "regex"
		
	def produce(self):
		if self.jsonConfig != None:
			return self.fromJSON()
		else:
			return None
			
	def fromJSON(self):				
		regexStrs = self.jsonConfig.get('regexes')
		regexStrs = regexStrs if regexStrs != "" else []
			
		labels = self.jsonConfig.get('labels')
		labels = labels if labels != '' else []
			
		return RegexMatcher(regexStrs, labels)
						
class Populated:
	missing_key = 'MISSING'
	populated_key = 'POPULATED'
	
	hist = None
	
	def __init__(self, missingValues=set([''])):
		self.missingValues = missingValues
		self.hist = Histogram([self.missing_key, self.populated_key])
	
	def update(self, value):
		key = self.missing_key if value == None or value in self.missingValues else self.populated_key
		self.hist.count(key)
	
	def __str__(self):
		return str(self.hist)

class PopulatedFactory:
	jsonConfig = None
	name = "populated"
	
	def __init__(self, name = None, jsonConfig = None):
		self.jsonConfig = jsonConfig
		self.name = name if name != None else self.name
	
	def produce(self):
		if self.jsonConfig != None:
			return self.fromJSON()
		else:
			return None
	
	def fromJSON(self):
		missingValues = self.jsonConfig.get('missingValues')
		missingValues = set(missingValues) if missingValues != "" else set([''])
			
		return Populated(missingValues)

class Numeric:
	minimum = None
	maximum = None
	mean = None
	median = None
	stddev = None
	mode = None
	modeCnt = None
	hist = None
	binnedHist = None
	total = None
	nonNumeric = None
	binning = None
	
	__metricsUpdated = False

	def __init__(self, binning=-1):
		self.total = 0
		self.nonNumeric = 0			
		self.hist = Histogram(sortByValue=True, reverse=False)
		self.binning = binning
		if self.binning > 0:
			self.binnedHist = Histogram(sortByValue=True, reverse=False)
		else:
			self.binnedHist = self.hist
	
	def update(self, value):
		if value != None:
			try:
				fval = float(value)
				self.hist.count(fval)
				if self.binning > 0:
					bfval = binFloat(fval, self.binning)
					self.binnedHist.count(bfval)
				
				self.total += 1
			except ValueError as e:
				self.nonNumeric += 1
		else:
			self.nonNumeric += 1
		self.__metricsUpdated = False
		
	def __updateMetrics(self):
		mode = None
		modeCnt = 0
		curSum = 0
		minimum = sys.float_info.max
		maximum = sys.float_info.min
		
		if len(self.hist.counts.keys()) > 0:		
			# Get min, max, mode, mean
			for (key,cnt) in self.hist.counts.items():
				if key < minimum:
					minimum = key
				if key > maximum or (key == 0.0 and maximum == sys.float_info.min):
					maximum = key
				if cnt > modeCnt:
					modeCnt = cnt
					mode = key
				curSum += key*cnt
			mean = float(curSum) / float(self.total)
			
			# Second Pass
			# Calculate stddev, median
			targetMedianCnt = self.total/2
			median = None
			curMedianCnt = 0
			curStdDevSum = 0.0
			for (val,cnt) in sorted(self.hist.counts.items()):
				curMedianCnt += cnt
				if median == None and curMedianCnt >= targetMedianCnt:
					median = val
				curStdDevSum += cnt*(val-mean)*(val-mean)
			
			self.stddev = math.sqrt(float(curStdDevSum) / float(self.total))
			self.minimum = minimum if minimum != sys.float_info.max else None
			self.maximum = maximum if maximum != sys.float_info.min else None
			self.mean = mean
			self.mode = mode
			self.modeCnt = modeCnt
			self.median = median
		
			self.__metricsUpdated = True
		
	def __str__(self):
		if not self.__metricsUpdated:
			self.__updateMetrics()
		sl = []
		sl.append('min=%s' % self.minimum)
		sl.append('max=%s' % self.maximum)
		sl.append('mean=%s' % self.mean)
		sl.append('median=%s' % self.median)
		sl.append('stddev=%s' % self.stddev)
		modePerc = 'N/A' if self.total == 0 else 'None' if self.total == None or self.modeCnt == None else float(self.modeCnt)/float(self.total)
		sl.append('mode=%s, cnt=%s, perc=%s' % (self.mode, self.modeCnt, modePerc))
		sl.append('total=%s' % self.total)
		sl.append('non_numeric=%s' % self.nonNumeric)
				
		
		sl.append('')
		sl.append(str(self.binnedHist))
		return '\n'.join(sl)
		
class NumericFactory:
	jsonConfig=None
	name="numeric"
	def __init__(self, name=None, jsonConfig=None):
		self.jsonConfig = jsonConfig
		self.name = name if name != None else self.name
		
	def produce(self):
		if self.jsonConfig != None:
			return self.fromJSON()
		else:
			return None
	
	def fromJSON(self):		
		binning = self.jsonConfig.get('binning')
		binning = float(binning) if binning != '' else -1.0
			
		return Numeric(binning)
			
class DataQuality:
	fin = None
	fout = None
	parser = None
	delim = None
	
	def __init__(self, inputFile, outputFile, fieldMetrics, hasHeader=True, fields=None, delim='|'):
		self.delim = delim
		
		self.fin = sys.stdin if inputFile in ('-', None) else codecs.open(inputFile, encoding='utf-8', mode='r')
		self.fout = sys.stdout if outputFile in ('-', None) else codecs.open(outputFile, encoding='utf-8', mode='w')
			
		if hasHeader:
			headerStr = self.fin.readline()
			self.parser = RecordParser(delim, fields, headerStr)
		else:
			self.parser = RecordParser(delim, fields)
			
		self.fieldMetrics = [ (group, list(metrics)) for (group, metrics) in itertools.groupby(fieldMetrics, lambda x: x[0]) ]
		
	def run(self):
		numLines = 0
		fieldCntHist = Histogram()
				
		for line in self.fin:
			(parsed, nf) = self.parser.parse(line)
			fieldCntHist.count(nf)
			numLines += 1
			for (field, metrics) in self.fieldMetrics:
				value = None if parsed == None else parsed.get(field)
				for (field, name, metric) in metrics:
					metric.update(value)
		
		self.fout.write('='*64 + '\n')
		self.fout.write('FILE STATS\n')
		self.fout.write('='*64 + '\n')
		self.fout.write('numLines=%s\n' % numLines)
		self.fout.write('numFields=\n')
		self.fout.write(str(fieldCntHist))
		
		for (field, metrics) in self.fieldMetrics:
			
			self.fout.write('='*64 + '\n')
			self.fout.write('%s\n' % field)
			self.fout.write('='*64 + '\n')
					
			for (field, name, metric) in metrics:
				self.fout.write('_'*32 + '\n')
				self.fout.write('|\n')
				self.fout.write('| %s\n' % name)
				self.fout.write('|'+ '_'*31 + '\n')
				self.fout.write(str(metric))
		
		if self.fin != sys.stdin:
			self.fin.close()
		if self.fout != sys.stdout:
			self.fout.close()
					
def parseDataQualityConfig(configJSONFile):
	with open(configJSONFile, 'r') as configFile:
		
		configString = configFile.read()
		jconfig = json.loads(configString)
		delim = jconfig.get('delim')
		delim = delim if delim != '' else '|'
	
		hasHeader = jconfig.get('header')
		hasHeader = hasHeader if hasHeader != '' else True
	
		fieldList = jconfig.get('fieldList')
		fieldList = fieldList if fieldList != '' else None
	
		histogramFactory = HistogramFactory()
		numericFactory = NumericFactory()
		populatedFactory = PopulatedFactory()
		regexMatcherFactory = RegexMatcherFactory()
		
		metricFactories = {
			"histogram" : HistogramFactory("histogram"),
			"cardinality": CardinalityFactory("cardinality")
		}
		for configType in jconfig.get('metrics'):
			for config in jconfig.get('metrics').get(configType):
				name = config.get('name')
				factory = None
				if configType == 'numeric':
					factory = NumericFactory(name, jsonConfig=config)
				elif configType == 'populated':
					factory = PopulatedFactory(name, jsonConfig=config)
				elif configType == 'regex':
					factory = RegexMatcherFactory(name, jsonConfig=config)
				else:
					raise AttributeError("invalid configuration type: %s" % configType)		
					
				metricFactories["%s.%s" % (configType, name)] = factory
		
		metrics = []
		for fieldConfig in jconfig.get('fieldMetrics'):
			field = fieldConfig.get('field')
			for metric in fieldConfig.get('metrics'):
				metrics.append((field, metric, metricFactories[metric].produce()))
				
		return (delim, hasHeader, fieldList, metrics)
		
def main():
	oParser = OptionParser(add_help_option=False, usage=USAGE)

	oParser.add_option("-i", "--inputFile", default="-", help="input file. [default: stdin]")
	oParser.add_option("-o", "--outputFile", default="-", help="output file. [default: stdout]")
	#oParser.add_option("-p", "--prettyPrint", action="store_true", dest="prettyPrint", default=False, help="pretty print. [default: %default]")
	oParser.add_option("-h", "--help", action="store_true", default=False, help="print this message")
	(options,args) = oParser.parse_args()
	
	if options.help:
		print oParser.print_help()
		print "Sample Config"
		print SAMPLE_CONFIG
		sys.exit(0)
	
	if len(args) == 0:
		oParser.error("missing configuration file")
	configFile = args[0]
	
	(delim, hasHeader, fieldList, metrics) = parseDataQualityConfig(configFile)
	
	dqr = DataQuality(options.inputFile, options.outputFile, metrics, hasHeader, fieldList, delim)
	dqr.run()
		
	

	
if __name__=='__main__':
	main()
		
		

#for line in fin:
#  parsed = line.strip('\n').strip('\r').split(options.delim)
#  if len(parsed) != expected_size:
###    nonConform += 1
#  else:
#    for i in range(0,len(parsed)):
#       if parsed[i].strip() != "":
#         cnts[i] = cnts[i] + 1
#    total += 1
#fin.close()

#outputHeader = ["FIELD", "MISSING", "POPULATED", "PERCENT_POPULATED"]

#keyFieldLen = max(max(map(lambda l: len(l), headerList)), len(outputHeader[0]))
#popFieldLen = max(max(map(lambda n: len(str(n)), cnts)), len(outputHeader[2]))
#misFieldLen = max([popFieldLen, len(str(total)), len(outputHeader[1])])
#perFieldLen = max(len(options.quantize), len(outputHeader[3]))

#if (options.prettyPrint):
#  outputHeader[0] = outputHeader[0].rjust(keyFieldLen,' ')
#  outputHeader[1] = outputHeader[1].rjust(misFieldLen,' ')
#  outputHeader[2] = outputHeader[2].rjust(popFieldLen,' ')
#  outputHeader[3] = outputHeader[3].rjust(perFieldLen,' ')
#print outputHeader

#fout.write(options.outputDelim.join(outputHeader) + "\n")

#for i in range(0,expected_size):
#  field = labels[i]
#  pop = cnts[i]
#  missing = total - pop
#  perc_pop = (Decimal(pop) / Decimal(total)).quantize(Decimal(options.quantize))

#  if options.prettyPrint:
#    field = str(field).rjust(keyFieldLen,' ')
#    missing = str(missing).rjust(misFieldLen,' ')
#    pop = str(pop).rjust(popFieldLen,' ')
#    perc_pop = str(perc_pop).rjust(perFieldLen,' ')

  #fout.write(options.outputDelim.join([field, str(missing), str(pop), str(perc_pop)]) + "\n")
#fout.write("Total Records: %s\n" % total)
#fout.write("Non-confirming Records: %s\n" % nonConform)
#fout.close()

