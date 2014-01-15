from optparse import OptionParser
import time,sys,random,signal


USAGE="usage: %prog [options] <sample_size>"

def main():

	oparser = OptionParser(usage=USAGE)
	oparser.add_option("-s", "--seed", default=None, dest="seed", help="Seed to use for random number generation. [default: current timestamp]")
	oparser.add_option("-i", "--input", default=None, dest="input", help="Input file. [default: stdin]")
	oparser.add_option("-o", "--output", default=None, dest="output", help="Output file. [default: stdin]")
	
	(options,args) = oparser.parse_args()

	if (len(args) != 1):
		oparser.error("Missing sample size")


	sampleSize = 0
	try:
		sampleSize = int(args[0])
	except ValueError as e:
		sys.exit("Invalid sample size: %s" % args[0])

	inFile = sys.stdin
	if (options.input != None):
		if (not os.path.isfile(options.input)):
			sys.exit("Input file does not exist: %s" % options.input)
		else:
			inFile = open(options.input, 'r')
	
	outFile = sys.stdout
	if (options.output != None):
		outFile = open(options.output, 'w')

	random.seed(options.seed)

	samples = {}
	samplesRead = 0

	try:
		for line in inFile:
			if (samplesRead < sampleSize):
				samples[samplesRead] = line
			else:
				sampleIndex = random.randint(0,samplesRead)
				if (sampleIndex < sampleSize):
					samples[sampleIndex] = line
			samplesRead += 1
	except KeyboardInterrupt:
		pass
	
	inFile.close()

	for lineIndex in range(0,sampleSize):
		outFile.write(samples[lineIndex])
	outFile.close()



	
		
	

if __name__=='__main__':
	main()
