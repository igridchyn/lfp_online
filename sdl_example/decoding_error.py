from math import *
from sys import argv
import os, time

print 'File Creation time: %s' % time.ctime(os.path.getctime(argv[1])) 

f = open(argv[1])
sum = 0
ndist = 0

bsize = 7;
errb = 0

for line in f:
	vals = line.split(' ')
	vals = [float(v) for v in vals]

	xc = vals[2]
	yc = vals[3]

	dist = sqrt( (vals[0]-vals[2])**2 + (vals[1]-vals[3])**2 )

	if (vals[2] > 1000):
		continue

	sum += dist
	ndist += 1

	xb = (round(xc / bsize) + 0.5) * bsize
	yb = (round(yc / bsize) + 0.5) * bsize

	errb += sqrt((xb-xc)**2 + (yb-yc)**2)


print "HMM error: ", sum/ndist
print "Binning error: ", errb/ndist
