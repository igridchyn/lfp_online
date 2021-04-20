#!/usr/bin/env python2

from sys import argv
import numpy as np
from matplotlib import pyplot
import os
from iutils import *
from call_log import *

if len(argv) < 3:
	print('USAGE: (1)<merge output base> (2-N)<basenames>')
    print ('Merge clu/res with given basenames in given order to common clu/res')
	exit(0)

outbase = argv[1]
nses = len(argv) - 2

fclu_path = outbase + 'clu'
fres_path = outbase + 'res'

if os.path.isfile(fclu_path) or os.path.isfile(fres_path):
    print('ERROR: output files exist')
    exit(1)

fclu = open(fclu_path, 'w')
fres = open(fres_path, 'w')

ss_cumul = 0

for ses in range(nses):
    ses_base = argv[ses+2]
    
    clu_ses = read_int_list(ses_base + 'clu')
    res_ses = read_int_list(ses_base + 'res')
    ss = read_int_list(os.path.dirname(ses_base) + '/session_shifts.txt')

    for i in range(len(clu_ses)):
        fclu.write('%d\n' % clu_ses[i])
        fres.write('%d\n' % (res_ses[i] + ss_cumul))

    ss_cumul += ss[-1]
