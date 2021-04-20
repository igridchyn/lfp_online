#!/usr/bin/env python3

from sys import argv
import numpy as np
from matplotlib import pyplot as plt
import os
from scipy import stats
from collections import defaultdict
import shutil

if len(argv) < 4:
	print('USAGE: (1)<base for sgclust files: $DIR/$ANIMAL_$DAY> (2)<directory containing tet* dirs> (3)<0/1 - downsample to 20 kHz>')
	exit(0)

SGBASE = argv[1]
TBASE = argv[2]
DOWNS = bool(int(argv[3]))

if TBASE[-1] != '/': TBASE += '/'
rows, columns = os.popen('stty size', 'r').read().split()
lwid = (int(columns) - 10) // 2

if not os.path.isfile(TBASE + 'TEMPLATE.par') or not os.path.isfile(TBASE + 'TEMPLATE.par.tet'):
    print('ERROR: Either TEMPLATE.par or TEMPLATE.par.tet are missing')
    exit(1)
#tpar = open(TBASE + 'TEMPLATE.par').readlines()
tpar = open(TBASE + os.path.basename(SGBASE) + '.par').readlines()
tpartet = open(TBASE + 'TEMPLATE.par.tet').readlines()

ntet = int(tpar[2].split()[0])
nses = int(tpar[3+ntet])
sesnums = [int(s.split('_')[-1]) for s in tpar[ntet+4:]]

tet = 0
while os.path.isdir('%stet%d' % (TBASE, tet)):
    print('='*lwid + '[ TET %02d ]'%tet + '='*lwid)

    tdir = '%stet%d/' % (TBASE , tet)
    shifts = np.loadtxt(tdir + 'session_shifts.txt', dtype=int)

    clu = np.loadtxt(tdir + 'tet%d.clu'%tet, dtype=int)
    nclu = max(clu)
    res = np.loadtxt(tdir + 'tet%d.res'%tet, dtype=int)
    fet = np.loadtxt(tdir + 'tet%d.fet.0'%tet, dtype=int, skiprows=2)
    nchan = (fet.shape[1] - 5) // 2

    spkname = tdir + 'tet0.spk.0.ORIG' if tet == 0 else tdir + 'tet%d.spk.%d'%(tet, tet)
    #fspk = open(tdir + 'tet%d.spk.0'%tet, 'rb')
    fspk = open(spkname, 'rb')

    # 24 kHz -> 20 kHz
    if DOWNS:
        fet[:,-1] = np.round(fet[:,-1]/6*5).astype(int)
        res = np.round(res/6*5).astype(int)
        shifts = np.round(shifts/6*5).astype(int)

    # clu/res: first assign 1 to unclustered!
    dres = dict(zip(res, clu))
    clu_upd = np.array([dres.get(k) if k in dres else 1 for k in fet[:,-1]])
    res_upd = fet[:,-1]

    shiftb = 0
    for (ishift, shift) in enumerate(shifts):
        #? does every entry in fet need to have entry in res? - see Philipp's dataset
        # write clu/res/fet

        istart = np.argmax(res_upd > shiftb)
        iend = np.argmax(res_upd > shift)
        if iend == 0: iend = len(res_upd)

        #nameset = (SGBASE, ishift + 1, tet)
        nameset = (SGBASE, sesnums[ishift], tet + 1)

        np.savetxt('%s_%02d.clu.%d' % nameset, [nclu] + list(clu_upd[istart:iend]), fmt='%d')
        np.savetxt('%s_%02d.res.%d' % nameset, res_upd[istart:iend] - shiftb, fmt='%d')
        subfet = fet[istart:iend]
        subfet[:,-1] -= shiftb
        np.savetxt('%s_%02d.fet.%d' % nameset, subfet, fmt = '%d', header=str(fet.shape[1]) + '\n')

        shiftb = shift
        # 128 samples per channel, 2 bytes per sample
        chunk = fspk.read(len(subfet) * nchan * 2 * 32) # 128 for interpolated!
        open('%s_%02d.spk.%d' % nameset, 'wb').write(chunk)

        # par file 
        # TODO check all content of par file
        open('%s_%02d.par.%d' % nameset, 'w').write('64 %d 50\n%s%s' % (nchan, tpar[3+tet], ''.join(tpartet)))
        shutil.copy(tdir + 'tet%d.mm.%d' % (tet, tet), '%s_%02d.mm.%d' % nameset)

    #namesetd = (SGBASE, len(shifts), tet)
    namesetd = (SGBASE, sesnums[0], sesnums[-1], tet + 1)

    # write merged (for all sessions) clu/res files
    nclu = max(clu_upd)
    np.savetxt('%s-%02d%02d.clu.%d' % namesetd, [nclu] + list(clu_upd), fmt='%d')
    np.savetxt('%s-%02d%02d.res.%d' % namesetd, res_upd, fmt='%d')
    open('%s-%02d%02d.par.%d' % namesetd, 'w').write('64 %d 50\n%s%s' % (nchan, tpar[3+tet], ''.join(tpartet)))
    sgtotd = os.path.relpath(tdir, start = os.path.dirname(SGBASE))
    os.symlink(sgtotd + '/tet%d.spk.0' % tet, '%s-%02d%02d.spk.%d' % namesetd)
    os.symlink(sgtotd + '/tet%d.fet.0' % tet, '%s-%02d%02d.fet.%d' % namesetd)

    tet += 1
