#!/usr/bin/env python

from sys import argv
import numpy as np
from matplotlib import pyplot
import os
from iutils import *
#from call_log import *

def read_ints(path):
    return [int(s[:-1]) for s in open(path).readlines()]

if len(argv) < 3:
    print 'USAGE: (1)<base for clu/res> (2)<output dir>'
    print '     Split merged clu and res into tetrodewise ones'
    exit(0)

BASE = argv[1]
BASENAME = os.path.basename(BASE)
OUTDIR = argv[2]

shifts = read_ints(BASE + 'cluster_shifts')
clu = read_ints(BASE + 'clu')
res = read_ints(BASE + 'res')

# print shifts
# add dummy 0 at start
shifts.insert(0, 0)

for t in range(1, len(shifts)):

    ftclu = open(OUTDIR + BASENAME + 'clu.'+ str(t), 'w')
    ftres = open(OUTDIR + BASENAME + 'res.'+ str(t), 'w')
    ftclu.write('%d\n' % (shifts[t] - shifts[t-1]))

    # WRITE NUMBER OF CLU !
    # CHECK IF NEED CLU 1 OR NEED START FROM 2 ?

    for i in range(len(clu)):
        if clu[i] > shifts[t-1] and clu[i] <= shifts[t]:
            ftclu.write('%d\n' % (clu[i]-shifts[t-1])) # may be + 1 if 1st cluster will be ignored
            ftres.write('%d\n' % (res[i])) # may be + 1 if 1st cluster will be ignored
