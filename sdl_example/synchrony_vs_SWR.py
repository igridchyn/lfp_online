from sys import argv
import numpy as np
import pylab as P

psyn = argv[1]
pswr = argv[2]

syns = []
swrs = []

for line in open(psyn):
	syns.append(int(line))

for line in open(pswr):
	swrs.append([int(s) for s in line.split()])

errs = []

# 30 ms
max_dist = 960
swrp = 1
SWI = int(argv[3])
for syn in syns:
	swrpstart = swrp

	if syn < swrs[swrp][SWI] - max_dist:
		continue

	# find the first after syn
	while swrp < len(swrs) and swrs[swrp][SWI] < syn:
		swrp += 1

	if swrp >= len(swrs):
		break

	err = syn - swrs[swrp - 1][SWI]
	err2 = swrs[swrp][SWI] - syn

	if err2 < err and err2 < max_dist:
		errs.append(-err2)
	elif err < max_dist:
		errs.append(err)

	# DEBUG
	# print swrp, err2, err
	# break
		
	if swrp > swrpstart:
		swrp -= 1

print errs
print len(errs), ' coinciding events within ', '%.2f ms' % (max_dist/24.0), ' from ', 'BEG' if SWI==0 else 'PEAK' if SWI==1 else 'END'
print 'Median error: %.2f +- %.2f'% (np.median(errs)/24.0, np.std(errs)/24.0)

n, bins, patches = P.hist([err/24.0 for err in errs], 30, normed=0, histtype='stepfilled')
P.show()
