#!/usr/bin/env python3

from sys import argv
import numpy as np
from matplotlib import pyplot as plt
import os
#from iutils import *
#from call_log import *
from scipy import stats

if len(argv) < 3:
	print('USAGE: (1)<dir with tet* directories> (2)<output base>')
	print('		MERGE clu/res from multiple tetrode directories')
	exit(0)

# 1e11
TMAX = 100000000000
if argv[1][-1] != '/':
	argv[1] += '/'
BASEDIR = argv[1] + 'tet'
BASEOUT = argv[2]

clus = []
ress = []

tet = 0
while os.path.isdir(BASEDIR + str(tet)):
	clus.append(np.loadtxt(BASEDIR + str(tet) + '/tet' + str(tet) + '.clu', dtype=int))
	ress.append(np.loadtxt(BASEDIR + str(tet) + '/tet' + str(tet) + '.res', dtype=int))
	ress[-1] = np.append(ress[-1], TMAX)
	tet += 1

clum = []
resm = []

# shifts
clu_shifts = [max(clu)-1 if len(clu)>0 else 0 for clu in clus]
clu_shifts = [np.sum(clu_shifts[:i]) for i in range(1, len(clus))]
clu_shifts.insert(0,0)

print(clu_shifts)

nextres = [res[0] for res in ress] 
points = [0] * len(ress)

while min(nextres) < TMAX:
	i = np.argmin(nextres)

	# -1 because cluster 1 is not present in output
	clum.append(clus[i][points[i]] - 1 + clu_shifts[i])
	resm.append(ress[i][points[i]])

	points[i] += 1
	nextres[i] = ress[i][points[i]]

#	print(i, nextres[i])

np.savetxt(BASEOUT + 'clu', clum, fmt='%d')
np.savetxt(BASEOUT + 'res', resm, fmt='%d')
np.savetxt(BASEOUT + 'cluster_shifts', clu_shifts, fmt='%d')
