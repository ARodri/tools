from datetime import datetime
import uuid

class Cache(object):
	
	def __init__(self,maxCacheSize = None):
		super(Cache,self).__init__()
		self.maxCacheSize = -1
		self.cacheSize = 0
		self.cache = {}
		self.MISSING = uuid.uuid4()
		

		if maxCacheSize != None:
			self.maxCacheSize = maxCacheSize

	
	def get(self,key):
		if key in self.cache:
			return self.cache[key]
		else:
			return self.MISSING

	def put(self,key,value):
		if self.maxCacheSize > 0 and self.cacheSize >= self.maxCacheSize:
			self.cache.popitem() # could do this truely randomly but meh
			self.cacheSize -= 1
		self.cache[key] = value
		self.cacheSize += 1

class CachedMapper(Cache, object):
	def __init__(self, func, maxCacheSize, default):
		super(CachedMapper, self).__init__(maxCacheSize)
		self.func = func
		self.default = default

	def map(self, input):
		cached_output = self.get(input)
		if cached_output != self.MISSING:
			return cached_output
		else:
			try:
				output = self.func(input)
				self.put(input,output)
				return output
			except ValueError,e:
				pass
			except TypeError,e:
				pass
			self.put(input,self.default) # Not sure if I like this but I'll stick it here.
			return self.default

class CachedDateParser(CachedMapper, object):

	def __init__(self, fmt="%Y%m%d", maxCacheSize=None, default=""):
		func = lambda s: datetime.strptime(s, fmt)
		super(CachedDateParser, self).__init__(func, maxCacheSize, default)

	def date(self,date_str):
		return self.map(date_str)

class CachedDateWriter(CachedMapper, object):
	def __init__(self, fmt="%Y%m%d", maxCacheSize=None, default=""):
		func = lambda d: datetime.strftime(d, fmt)
		super(CachedDateWriter, self).__init__(func, maxCacheSize, default)

	def str(self,date):
		return self.map(date)

class CachedDateReformatter(object):
	def __init__(self,inputFmt="%Y-%m-%d",outputFmt="%Y%m%d",maxCacheSize=None,default=""):
		self.parser = CachedDateParser(inputFmt,maxCacheSize,default)
		self.writer = CachedDateWriter(outputFmt,maxCacheSize,default)

	def reformat(self,date_str):
		return self.writer.str((self.parser.date(date_str)))

