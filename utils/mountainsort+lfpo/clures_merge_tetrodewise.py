#!/usr/bin/env python

from sys import argv
import numpy as np
from matplotlib import pyplot
import os
#from iutils import *
#from call_log import *

def read_ints(path):
    return [int(s[:-1]) for s in open(path).readlines()]

if len(argv) < 3:
	print('USAGE: (1)<output base for tetrode-wise clu/res merged for given sessions> (2-N)<path bases for tetrode-wise session-wise files>')
	exit(0)

OUTBASE = argv[1]
BASES = argv[2:]

fresofs = open(OUTBASE + 'resofs', 'w')

t = 1
while os.path.isfile(BASES[0] + 'fet.' + str(t)) or os.path.isfile(BASES[0] + 'fet.' + str(t)):
    for base in BASES:
        if os.path.isfile(base + 'clu.' + str(t)):
            fclu = open(OUTBASE + 'clu.' + str(t), 'w')
            fres = open(OUTBASE + 'res.' + str(t), 'w')
        break

    ffet = open(OUTBASE + 'fet.' + str(t), 'w')

    tshift = 0
    ncluprev = 0
    for base in BASES:
        if os.path.isfile(base + 'clu.' + str(t)):
            clus = read_ints(base + 'clu.' + str(t))
            ress = read_ints(base + 'res.' + str(t))
    
            # print('Time shift = %d, tet = %d, base=%s' % (tshift, t, base))
    
            nclu = clus[0]
            if base == BASES[0]:
                fclu.write('%d\n' % nclu)
            else:
                if nclu != ncluprev:
                    print('WARNING: number of clusters is different from previous session: %d vs. %d, tetrode = %s' % (nclu, ncluprev, t))
            ncluprev = nclu
    
            # add current cumulative time shift
            ress = [r+tshift for r in ress]
    
            for i in range(len(ress)):
                fclu.write('%d\n' % clus[i+1])
                fres.write('%d\n' % (ress[i]))

        # fet: add shift to last feature (time)
        ffet_in = open(base + 'fet.' + str(t))
        for line in ffet_in:
            ws = line.split(' ')

            # first line
            if len(ws) < 5:
                # onyl need to write once
                if base == BASES[0]:
                    ffet.write(line)
                continue

            time = int(ws[-2]) + tshift
            ws[-1] = str(time)
            ffet.write(' '.join(ws) + '\n')

        sspath = os.path.dirname(base) + '/session_shifts.txt'
        if not os.path.isfile(sspath):
            sspath1 = sspath
            sspath = os.path.dirname(base) + '/../session_shifts.txt'
            if not os.path.isfile(sspath):
                print('ERROR: session_shifts.txt is not available neither in %s not in %s!' % (sspath1, sspath))
                exit(1)

        session_shifts = read_ints(sspath)
        tshift += session_shifts[-1]

        if t == 1:
        # add duration of the session to the time shift
            fresofs.write('%d\n' % tshift)

    print('Done merging clu/res/fet for tetrode %d' % t)
    t = t + 1

    if len(session_shifts) > 1:
        print('WARNING: every input base directory is supposed to have only a single session!')
