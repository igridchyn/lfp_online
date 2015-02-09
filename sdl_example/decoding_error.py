from math import *
from sys import argv
import os, time
import numpy as np
import pylab as P

print 'File Creation time: %s' % time.ctime(os.path.getctime(argv[1])) 

f = open(argv[1])
sum = 0
ndist = 0

sumnosb = 0
nnosb = 0

bsize = 4
nbins = 43
errb = 0

sbs = [[113, 10], [319, 137]]

errs = []

classn = 0.0
classcorr = 0

for line in f:
	vals = line.split(' ')
	vals = [float(v) for v in vals]

	xc = vals[2]
	yc = vals[3]

	dist = sqrt( (vals[0]-vals[2])**2 + (vals[1]-vals[3])**2 )

	if vals[2] > 1000 or vals[0] < 5:
		continue

	distsb1 = sqrt((vals[2]-sbs[0][0])**2 + (vals[3]-sbs[0][1])**2)
	distsb2 = sqrt((vals[2]-sbs[1][0])**2 + (vals[3]-sbs[1][1])**2)

	if min(distsb1, distsb2) > 30:
		sumnosb += dist
		nnosb += 1

	sum += dist
	ndist += 1

	errs.append(dist)

	xb = (round(xc / bsize) + 0.5) * bsize
	yb = (round(yc / bsize) + 0.5) * bsize

	errb += sqrt((xb-xc)**2 + (yb-yc)**2)

	# classification
	classn += 1
	if (vals[0] - nbins*bsize) * (vals[2] - nbins*bsize) > 0:
		classcorr += 1

n, bins, patches = P.hist(errs, 200, normed=0, histtype='stepfilled')
# P.figure()

print "Average error: ", sum/ndist
print "Median error: ", np.median(np.array(errs))
print "Average error outside of SB: ", sumnosb/nnosb
print ("Classification precision: %.1f%%") % (classcorr * 100 / classn)
print "Binning error: ", errb/ndist

P.show()
