from sys import argv
import numpy as np
import pylab as P

path = argv[1]
nbin = int(argv[2])

data = [float(line[:-1]) for line in open(path)]

if len(argv) > 3:
	dmax = float(argv[3])
else:
	dmax = max(data)

n, bins, patches = P.hist(data, nbin, range=[min(data), dmax], normed=0, histtype='stepfilled')
P.show()
