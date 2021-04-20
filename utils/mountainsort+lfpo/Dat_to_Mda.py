#!/usr/bin/env python3

import numpy as np 
from mountainlab_pytools import mdaio
import sys
import os

if len(sys.argv)<5:
    print('USAGE: (1)< base name> (2)<tetrode number> (3)<TOTAL number of dat files> (4)<TOT CHANS: 64/128>')
    exit(1)

base = sys.argv[1]
tet = int(sys.argv[2])
tot_dats=int(sys.argv[3])
TOT_CHANS = int(sys.argv[4])

# read par file - need to know how many channel per tetrode and number of dats
parf = base+'.par' 
with open(parf) as f: 
    par = f.readlines() 
par = [x.strip() for x in par]

chan = [] # channel of each tetrode from par 

ntet = int(par[2].split()[0])

print(par)

for p in par[3:3+ntet]: 
    l = p.split() 
    chan.append([int(c) for c in l[1:]]) 

MAX_SIZE = (5000000000 // 128) * 128
#MAX_SIZE = (1000000000 // 128) * 128
dats_tet=[]
for datf in par[-tot_dats:]: 
    print(datf)
    fname = datf+'.dat'

    # IF SIZE > MAX ALLOWED => READ IN CHUNKS!
    fsize = os.stat(fname).st_size
    if (fsize < MAX_SIZE):
        d = np.fromfile(fname,dtype=np.int16).reshape(-1, TOT_CHANS)
        dats_tet.append(d[:,chan[tet]])
    else:
        print('File too large, read in batches')
        for b in range(fsize // MAX_SIZE + 1):
            count = -1 if b==fsize//MAX_SIZE else MAX_SIZE
            offset = b*MAX_SIZE
            #print('COUNT/OFFSET', count, offset)
            dp = np.fromfile(fname,dtype=np.int16,count=count//2, offset=offset).reshape(-1, TOT_CHANS)[:, chan[tet]]
            d = dp if b==0 else np.vstack((d, dp))
            #print('NEW SHAPE', d.shape)
        dats_tet.append(d)

# this is what you'll write to disk - the dat relative to that tetrode basically
mdao = np.concatenate(dats_tet)

mdaio.writemda16i(mdao.T,'tet'+str(tet)+'/tet'+str(tet)+'raw.mda') 


