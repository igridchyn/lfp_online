#!/usr/bin/env python

from sys import argv
import numpy as np
import os

# USE LAST PACKAGE ID FOR SHIFT ?

if len(argv) < 2:
	print('USAGE: (1-)<file with basenames> (2)<64/128 channels> (3)<outout whl>')
	exit(0)

fwhl = open(argv[-1], 'w')

shift = 0
swhl = []

nchan = int(argv[2])
axpaths = [f.strip()+'.axtrk' for f in open(argv[1]).readlines()]

for axpath in axpaths:
	ax = np.loadtxt(axpath, skiprows=1, dtype=int)

	if ax[0,0] > 10000:
		#times -= times[0] - 4
		print('WARNING: Starting sample# is probably wrong in %s, assuming it starts from 4.' % axpath)
		times = ax[:,0] - ax[0,0] + 4 + shift
	else:
		times = ax[:,0] + shift

	# check if there is switch in axtrk Sample# 
	#   and fix it
	ineg = np.argmax(times < 0)
	if ineg > 0:
		negfix = times[ineg-1] - times[ineg] + 240
		print('WARNING: BREAK IN SAMPLE# DETECTED AT POS %d in %s, FIX BY CONTINUING FROM LARGEST TIMESTAMP!' % (ineg, axpath))
		print('		PLEASE, PLOT WHL TIMESTAMPS JUST IN CASE')
		times[ineg:] += negfix

	# detect big changes
	diffs = times[1:] - times[:-1]
	if max(diffs) > 1000:
		print('WARNING: big diff', max(diffs), axpath)

	val = (ax[:,3] == 1023) & (ax[:,5] == 1023)
	val = 1 - val.astype(np.int)

	times = times.reshape(-1,1)
	val = val.reshape(-1, 1)

	for i in [3,4,5,6]:
		ax[ax[:,i] < 1000,i] //= 3

	#swhl = np.concatenate((ax[:,(3,4,5,6)], times.T, val.T), axis=1)
	nswhl = np.hstack((ax[:,(3,4,5,6)], times, val))
	if len(swhl) == 0:
		swhl = nswhl
	else:
		print(swhl.shape, nswhl.shape)
		swhl = np.vstack((swhl, nswhl))

	# TODO: USE FILE LENGTH!
	nsamp = os.stat(axpath.replace('axtrk', 'dat')).st_size // (nchan * 2)
	#shift = times[-1]
	shift += nsamp

np.savetxt(fwhl, swhl[::2], fmt='%d')
