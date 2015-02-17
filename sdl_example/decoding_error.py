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

predmap = np.zeros((nbins, nbins*2))
occmap = np.zeros((nbins, nbins*2))

for line in f:
	vals = line.split(' ')
	vals = [float(v) for v in vals]

	gtx = vals[2]
	gty = vals[3]
	px = vals[0]
	py = vals[1]

	dist = sqrt( (px-gtx)**2 + (py-gty)**2 )

	if gtx > 1000 or px < 5 or gtx < 0:
		continue

	distsb1 = sqrt((gtx-sbs[0][0])**2 + (gty-sbs[0][1])**2)
	distsb2 = sqrt((gtx-sbs[1][0])**2 + (gty-sbs[1][1])**2)

	if min(distsb1, distsb2) > 30:
		sumnosb += dist
		nnosb += 1

	sum += dist
	ndist += 1

	errs.append(dist)

	xb = (round(gtx / bsize) + 0.5) * bsize
	yb = (round(gty / bsize) + 0.5) * bsize

	gtxb=round((gtx-bsize/2.0)/bsize)
	gtyb=round((gty-bsize/2.0)/bsize)
	occmap[gtyb, gtxb] += 1

	xpb = round((px-bsize/2)/bsize)
	ypb = round((py-bsize/2)/bsize)
	predmap[ypb, xpb] += 1

	errb += sqrt((xb-gtx)**2 + (yb-gty)**2)

	# classification
	classn += 1
	if (vals[0] - nbins*bsize) * (gtx - nbins*bsize) > 0:
		classcorr += 1

# P.figure()

print "Average error: ", sum/ndist
print "Median error: ", np.median(np.array(errs))
print "Average error outside of SB: ", sumnosb/nnosb
print ("Classification precision: %.1f%%") % (classcorr * 100 / classn)
print "Binning error: ", errb/ndist

if len(argv) > 2:
	n, bins, patches = P.hist(errs, 200, normed=0, histtype='stepfilled')
	P.show()

	im=P.imshow(predmap, cmap='hot', interpolation='none')
	P.show()
	
	imocc=P.imshow(occmap, cmap='hot', interpolation='none')
	P.show()
