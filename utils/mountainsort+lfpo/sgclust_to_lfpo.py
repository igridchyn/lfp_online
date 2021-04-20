#!/usr/bin/env python3

from sys import argv
import shutil
import os
import subprocess
import numpy as np

# TODO: automate whl copying/extraction

if len(argv) < 3:
    print('USAGE: (1)<directory containing tet#/ directory> (2)<tetrode number>')
    exit(0)

tdir = argv[1]
tet = int(argv[2])
tets = argv[2]
tetn = 'tet' + str(tet)

os.chdir(argv[1] + '/' + tetn)

# move tet#.spkb.# to tet#.spk.0 and fet.tet# to fet.0
# THIS IS CREATED BY C++
#if os.path.isfile(tetn + '.spkb.' + tets):
#    shutil.move(tetn + '.spkb.' + tets, tetn + '.spk.0')
#else:
#    print('WARINING: spkb file not found')
shutil.move(tetn + '.fet.' + tets, tetn + '.fet.0')

# create session shifts and cluster shifts files
# TODO - align ...
fss = open('session_shifts.txt', 'w')
fss.write('0\n')
fss.close()
fcs = open(tetn + '.cluster_shifts', 'w')
fcs.write('0\n')
fcs.close()

# remove first line from clu
with open(tetn  + '.clu.' + tets, 'r') as fin:
    data = fin.read().splitlines(True)
with open(tetn + '.clu', 'w') as fout:
    fout.writelines(data[1:])

# move res.0 to res
# 2X BUG
res = np.loadtxt(tetn + '.res.' + tets)
# was fixed in ms to sgclust
#res *= 2
np.savetxt(tetn + '.res', res, fmt='%d')
