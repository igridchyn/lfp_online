from math import *
from sys import argv
import os, time
import numpy as np
import pylab as P
from subprocess import call, Popen
from datetime import *

def decoding_errors():
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
		# print gtyb, gtxb
		occmap[min(gtyb, nbins-1), min(gtxb, 2*nbins-1)] += 1

		xpb = round((px-bsize/2)/bsize)
		ypb = round((py-bsize/2)/bsize)
		predmap[ypb, xpb] += 1

		errb += sqrt((xb-gtx)**2 + (yb-gty)**2)

		# classification
		classn += 1
		if (vals[0] - nbins*bsize) * (gtx - nbins*bsize) > 0:
			classcorr += 1

	return (sum, ndist, errs, sumnosb, nnosb, classcorr, classn, errb)
#========================================================================================================
def run_model_and_decoding(pnames, pvals):
	parstring = ['./lfp_online']
	parstring.append(argv[3])
	for p in range(0, len(pnames)):
		parstring.append(pnames[p] + '=' + str(pvals[p]) + ' ')
	
	# run model build
	print 'Start model build with params ', parstring
	subp=Popen(parstring, cwd = '/home/igor/code/ews/lfp_online/sdl_example/Debug')
	subp.wait()
	# run decoding
	parstring[1] = argv[4]
	print 'Start decoding with params ', parstring
	subp=Popen(parstring, cwd = '/home/igor/code/ews/lfp_online/sdl_example/Debug')
	subp.wait()

#========================================================================================================
def log(s):
	print s
	flog.write(s + '\n')

#========================================================================================================
def gradient_descent():
	# read param names, starting values and steps
	pnames=[]
	pvals=[]
	psteps=[]
	fpar = open(argv[2])
	pnum = int(fpar.readline())
	for p in range(0, pnum):
		pnames.append(fpar.readline()[:-1])
		pvals.append(float(fpar.readline()))
		psteps.append(float(fpar.readline()))

	print 'Optimize params: ', pnames
	print os.getcwd()
	# exit(0)

	parbest = []
	precbest = 0
	errthold = 25
	errbest = 0
	prevbest = -1
	# iteratively find new best set of parameters while have improvement
	while precbest > prevbest:
		prevbest = precbest
		for p in range(0, pnum):
			# run with changed p-th param (+/-)
			for dp in (-psteps[p], psteps[p]):
				pvals[p] += dp
				run_model_and_decoding(pnames, pvals)
				sum, ndist, errs, sumnosb, nnosb, classcorr, classn, errb = decoding_errors()
				mederr = np.median(np.array(errs))
				classprec = classcorr * 100 / classn

				log('Done estimation for params ' + str(pvals))
				log('Med. error / classification error: %.2f / %.1f%%' %(mederr, classprec))

				if classprec > precbest and mederr < errthold:
					print 'New BEST!'
					precbest = classprec
					parbest = pvals[:]
					errbest = mederr

				# return old param value
				pvals[p] -= dp

		log('Iteration over, new best params: ' + str(parbest))
		log('BEST Med. error / classification error: %.2f / %.1f%%' %(errbest, precbest))

		pvals = parbest[:]
#============================================================================================================
if len(argv) > 3:
	flog = open('log_opt.txt', 'a')
	dt = datetime.now()
	flog.write('OPTIMIZATION SESSION: ' + str(dt) + '\n')
	gradient_descent()
	

sum, ndist, errs, sumnosb, nnosb, classcorr, classn, errb = decoding_errors()

# P.figure()

print classcorr, classn

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
