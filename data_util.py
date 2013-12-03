
def makeParser(header,delim):
  
	headerList = header.strip('\n').strip('\r').split(delim)
	expectedSize = len(headerList)
	
	def parser(line):
		parsed = line.strip('\n').strip('\r').split(delim)
		
		if (len(parsed) != expectedSize):
			return ({}, True, "Non rectagularity. Expected "+str(expectedSize)+", found "+str(len(parsed)))
		else:
				try:
					return (dict(map(list, zip(headerList, parsed))), False, None)
				except Exception as e:
				  return({}, True, e.value)
	return (headerList, parser)

def makeWriter(headerList,delim):

	def writer(parsedMap):
		parsed = []
		for field in headerList:
			parsed.append(str(parsedMap.get(field, '')))
		return delim.join(parsed) + "\n"
	return writer
