#!/usr/bin/env python3

import numpy as np 
from mountainlab_pytools import mdaio
import sys
from scipy.interpolate import interp1d

if len(sys.argv)<2:
    print('Not enough input. I need the tetrode number #t (folder tet+#t where to find the files firings.mda and raw_filt.mda')
    exit()

tet= sys.argv[1]
fold='tet'+tet

fpar = open(fold + '/' + 'tet' + tet + '.par.' + tet)
nchan = int(fpar.readline().split()[1])
print('Number of channels:',nchan)

a = mdaio.readmda(fold+'/raw_filt.mda')
b = mdaio.readmda(fold+'/firings.mda')

nC, L = a.shape # number of channels, length of recording

res = b[1,:].astype(int)
clu = b[2,:].astype(int)

print(res[-1])

c = np.zeros([len(res),nchan,32])  # was 32
ce = np.zeros([len(res),nchan,33])

for ir, r in enumerate(res):
    if r > 15 and r < L-18:
        c[ir,:,:] = a[:,r-15:r+17] # was 17
        ce[ir,:,:] = a[:,r-15:r+18] # was 17

# WOAH! THIS SUCKS - REASON IS THAT in Dat_to_mda RESHAPING WAS WRONG - 128 INSTEAD OF 64!
#res *= 2

# do PCA on the 4 channel separately
npc = 2
# TODO: calculate and write 4 special features!
d = np.zeros([len(res),nchan*npc+4+1])
d[:,-1] = res
for i in range(nchan):
    C = c[:,i,:].T@c[:,i,:]
    lams, us = np.linalg.eig(C)
    print('EVals', lams)
    T = c[:,i,:]@us
    d[:,i*npc:(i+1)*npc] = T[:,:npc]
    print(i*npc,(i+1)*npc)

# save d as plain text, toghether with clu and res
np.savetxt(fold+'/'+fold+'.res.'+tet,res,fmt='%i')
np.savetxt(fold+'/'+fold+'.clu.'+tet,[max(clu)+1]+list(clu+1),fmt='%i')
# TODO: HEADER DEPENDING ON NUMBER OF FEATURES !
np.savetxt(fold+'/'+fold+'.fet.'+tet,d,fmt='%i',header = str(nchan*npc+4+1),comments='')
np.savetxt(fold+'/'+fold+'.mm.'+tet,[0,0,0,0,0,0,0,0])

#print('WARNING: NOT SAVING SPK')
#exit(0)

# now save the file spk to have the spike shape in sgclust

# c[SPIKE, CHAN, TIME]
c128 = np.zeros([len(res),nchan,128]) 

print(type(c128), c128.shape)

#for s in range(len(c)):
    # interpolate each channel
#    for ch in range(nchan):
#        f = interp1d(range(0,128+1,4), ce[s,ch,:], kind='linear')
#        c128[s,ch,:] = f(range(0,128))
#        c128[s,ch,::4] = c[s,ch,:]
#        c128[s,ch,1::4] = c[s,ch,:]
#        c128[s,ch,2::4] = c[s,ch,:]
#        c128[s,ch,3::4] = c[s,ch,:]

#c128[:,:,::4] = c[:,:,:]
#c128[:,:,1::4] = c[:,:,:]
#c128[:,:,2::4] = c[:,:,:]
#c128[:,:,3::4] = c[:,:,:]

#c=np.swapaxes(c,1,2)
#c128=np.swapaxes(c128,1,2)

c = c / np.max(np.abs(c)) * 1024
c = c.astype(np.int16)

#c128 = c128 / np.max(np.abs(c128)) * 1024
#c128 = c128.astype(np.int16)

with open(fold+'/'+fold+'.spk.'+tet, "wb") as binary_file:
#    # Write text or bytes to the file
#    for byte in c.flatten():
#        binary_file.write(int(byte).to_bytes(2, byteorder='big',signed=True))
    np.array(c.flatten()).tofile(binary_file)

#with open(fold+'/'+fold+'.spkb.'+tet, "wb") as binary_file:
    # Write text or bytes to the file
    #for byte in c128.flatten():
    #    binary_file.write(int(byte).to_bytes(2, byteorder='little',signed=True))
#    np.array(c128.flatten()).tofile(binary_file)
