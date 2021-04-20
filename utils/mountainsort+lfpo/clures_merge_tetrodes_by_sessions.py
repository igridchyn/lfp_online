#!/usr/bin/env python3

from sys import argv
import numpy as np
from matplotlib import pyplot as plt
import os
#from call_log import *

if len(argv) < 2:
	print('USAGE: (1)<basename: of daywise parfile and des/desen>')
	exit(0)

# read from par: # of tetrodes and basenames
BASE = argv[1]
if BASE[-1] != '.' : BASE += '.'

parlines = open(BASE + 'par').readlines()

ntet = int(parlines[2].split()[0])
print('ntet', ntet)
sesbases = [s[:-1] + '.' for s in parlines[4+ntet:]]

for sesbase in sesbases:
    print('Merge tetrodes for session', sesbase)

    clushift = 0
    for t in range(1, ntet + 1):
        print('t', t, 'clushift', clushift)
        res = np.loadtxt(sesbase + 'res.' + str(t), dtype=int)
        clu = np.loadtxt(sesbase + 'clu.' + str(t), dtype=int)
        nclu = clu[0]
        clu = clu[1:]
        clu[clu>1] += clushift
        
        clushift += nclu - 1

        res = res.reshape(-1,1)
        clu = clu.reshape(-1,1)

        if t == 1:
            merged = np.hstack([clu,res])
        else:
            merged = np.vstack([merged, np.hstack([clu,res])])

    nclu = int(max(merged[:,0]))

    merged = merged[merged[:,1].argsort(),:]

    print('merged head', merged[:4,:])

    np.savetxt(sesbase + 'clu', [nclu] + list(merged[:,0]), fmt='%d')
    np.savetxt(sesbase + 'res', merged[:,1], fmt='%d')
