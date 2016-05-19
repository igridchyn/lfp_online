from math import *
from sys import argv
import os, time
import numpy as np
#import pylab as P
from matplotlib import pyplot as P
from subprocess import call, Popen
import datetime

def decoding_errors():
	print 'File Creation time: %s' % time.ctime(os.path.getctime(argv[1])) 

	f = open(argv[1])
	sum = 0
	ndist = 0

	sumnosb = 0
	nnosb = 0

	errb = 0

	sbs = [[113, 10], [319, 137]]

	errs = []

	classn = 0.0
	classcorr = 0

	for line in f:
		vals = line.split(' ')
		vals = [float(v) for v in vals]

		gtx = vals[2]
		gty = vals[3]
		px = vals[0]
		py = vals[1]

		dist = sqrt( (px-gtx)**2 + (py-gty)**2 )

		if gtx > 1000 or gtx < 0: # or px < 5
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
		occmap[min(gtyb, nbinsy-1), min(gtxb, nbinsx-1)] += 1
		errmap[min(gtyb, nbinsy-1), min(gtxb, nbinsx-1)] += dist

		xpb = round((px-bsize/2)/bsize)
		ypb = round((py-bsize/2)/bsize)
		predmap[ypb, xpb] += 1

		errb += sqrt((xb-gtx)**2 + (yb-gty)**2)

		# classification
		classn += 1
		# was : nbinsx/2*bsize
		#if (vals[0] - envlimit) * (gtx - envlimit) > 0:
		#	classcorr += 1
		egt = int(floor(gtx / envlimit))
		edec = int(floor(vals[0] / envlimit))
		if (edec - egt) % 2 == 0:
			classcorr += 1


	return (sum, ndist, errs, sumnosb, nnosb, classcorr, classn, errb)
#========================================================================================================
def run_model_and_decoding(pnames, pvals):
	parstring = ['./lfp_online']
	parstring.append(argv[6])
	for p in range(0, len(pnames)):
		parstring.append(pnames[p] + '=' + str(pvals[p]) + ' ')
	
	# run model build
	log('Start model build with params ' + str(parstring))
	subp=Popen(parstring, cwd = '/home/igor/code/ews/lfp_online/sdl_example/Debug')
	subp.wait()
	# run decoding
	parstring[1] = argv[7]
	log('Start decoding with params ' + str(parstring))
	subp=Popen(parstring, cwd = '/home/igor/code/ews/lfp_online/sdl_example/Debug')
	subp.wait()

#========================================================================================================
def log(s):
	print s
	flog.write(s + '\n')
	flog.flush()

#========================================================================================================
def gradient_descent():
	ma = 0.0
	me = 1.0

	# read param names, starting values and steps
	pnames=[]
	pvals=[]
	psteps=[]
	fpar = open(argv[5])
	pnum = int(fpar.readline())
	for p in range(0, pnum):
		pnames.append(fpar.readline()[:-1])
		pvals.append(float(fpar.readline()))
		psteps.append(float(fpar.readline()))

	log('Optimize params: ' + str(pnames))
	print os.getcwd()
	# exit(0)

	psteps_min = psteps[:]

	run_model_and_decoding(pnames, pvals)
        sum, ndist, errs, sumnosb, nnosb, classcorr, classn, errb = decoding_errors()
	mederr = np.median(np.array(errs))
        classprec = classcorr * 100 / classn
	log('BASELINE (starting precision): %.2f / %.1f%%' %(mederr, classprec))
	log('Number of error records: %d' % len(errs))
	log('Criteria multipliers: ACC = %.2f, ERR = %.2f' % (ma, me))

	parbest = pvals[:]
	precbest = classprec
	precthold = classprec
	errthold = 7.4#mederr
	errbest = mederr

	crit_best = ma * classprec - me * mederr
	log('BASELINE crit = %.3f' % crit_best)
	stepfac = 1

	# iteratively find new best set of parameters while have improvement
	while True:
		better_found = True
		while better_found:
			better_found = False
			for p in range(0, pnum):
				# run with changed p-th param (+/-)
				for dp in (-psteps[p], psteps[p]):
					pvals[p] += dp
					run_model_and_decoding(pnames, pvals)
					sum, ndist, errs, sumnosb, nnosb, classcorr, classn, errb = decoding_errors()
					mederr = np.median(np.array(errs))
					if classn > 0:
						classprec = classcorr * 100 / classn
					else:
						classprec = 0
						log('Model buliding / decoding has likely failed')
	
					log('Done estimation for params ' + str(pvals))
					log('Med. error / classification accuracy: %.2f / %.1f%%' %(mederr, classprec))
					log('Number of error records: %d' % len(errs))
	
					crit = ma * classprec - me * mederr
					
					if crit > crit_best:
					# if classprec > precbest and mederr < errthold:
					#if classprec >= precthold and mederr < errbest:
					# if mederr < errbest:
						log('NEW BEST! (%.1f * ACC - %.1f * ERR)' % (ma, me))
						log('Steps will be reset to initial values!')
						precbest = classprec
						parbest = pvals[:]
						errbest = mederr
					        psteps = psteps_min[:]
						stepfac = 1
						log('OLD BEST CRIT VALUE: %.3f, NEW BEST CRIT VALUE: %.3f' % (crit_best, crit))
						crit_best = ma * classprec - me * mederr
						better_found = True
	
					# return old param value
					pvals[p] -= dp
	
			log('Iteration over, new best params: ' + str(parbest))
			log('BEST Med. error / classification error / crit: %.2f / %.1f%% / %.3f' %(errbest, precbest, crit_best))

			pvals = parbest[:] # while better_found

		log('NO IMPROVEMENT, START WITH STEPS %dX' % (stepfac * 2));
		# increase steps 2X and continue
		for i in range(0, len(psteps)):
			psteps[i] = psteps[i] * 2

		stepfac *= 2
		if stepfac > 8:
			log('STEP limit reached (8X), OPTIMIZATION SESSION OVER')
			log('BEST PARAMS: ' + str(parbest))
			log('BEST Med. error / classification error / crit: %.2f / %.1f%% / %.3f' %(errbest, precbest, crit_best))
			log('\n')
			exit(0) # while True
#============================================================================================================
if len(argv) < 6:
	print 'Usage: decoding_error.py (1)<decoder_output_file_name> (2)<nbinsx> (3)<nbinsy> (4)<environment border> (5)<plot distribution or not> (6)<bin size>'
	print 'Or:    decoding_error.py (1)<error_file_name> (2)<nbinsx> (3)<nbinsy> (4)<environment border> (5)<opt_config> (6)<initial_build_model_config> (7)<initial_decoding_config> (8)<bin size>'
	exit(0)

nbinsx = int(argv[2])
nbinsy = int(argv[3])
envlimit = int(argv[4])
bsize = float(argv[6 if len(argv) < 9 else 8])
predmap = np.zeros((nbinsy, nbinsx))
occmap = np.zeros((nbinsy, nbinsx))
errmap = np.zeros((nbinsy, nbinsx))

if len(argv) == 9:
	flog = open('log_opt.txt', 'a')
	dt = datetime.datetime.now()
	flog.write('\n\nOPTIMIZATION SESSION: ' + str(dt) + '\n')
	gradient_descent()

sum, ndist, errs, sumnosb, nnosb, classcorr, classn, errb = decoding_errors()
errmap = np.divide(errmap, occmap)
errmap = np.nan_to_num(errmap)

# P.figure()

print classcorr, classn

print "Average error: ", sum/ndist
print "Median error: ", np.median(np.array(errs))
print "Average error outside of SB: ", sumnosb/nnosb
print ("Classification precision: %.1f%%") % (classcorr * 100 / classn)
print "Binning error: ", errb/ndist

plot_distr = int(argv[5])
if plot_distr:
	#n, bins, patches = P.hist(errs, 200, normed=0, histtype='stepfilled')
	#P.show()

	im=P.imshow(predmap, cmap='hot', interpolation='none')
	P.title('Predicted locations')
	P.colorbar(orientation='horizontal')	
	P.show()
	
	#imocc=P.imshow(occmap, cmap='hot', interpolation='none')
	#P.show()

	P.title('Error map')
	imerr=P.imshow(errmap, cmap='hot', interpolation='none')
	P.colorbar(orientation='horizontal')
	P.show()

# print errmap
sum1 = np.sum(errmap[:, 0:nbinsx/2])
sum2 = np.sum(errmap[:, nbinsx/2:])
print 'Error in env 1 = %f' % sum1
print 'Error in env 2 = %f' % sum2
