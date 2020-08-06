# from numpy import *
import numpy
from sys import argv

# f=open(argv[1])

a=numpy.loadtxt(argv[1])
plow = numpy.percentile(a, float(argv[2]))
phigh = numpy.percentile(a, float(argv[3]))
print 'Mean: %f' % numpy.mean(a)
print '%f percentile: %f' % (float(argv[2]), plow)
print '%f percentile: %f' % (float(argv[3]), phigh)

print 'Config params:\n'
print 'lpt.trigger.confidence.average=%f'%numpy.mean(a)
print 'lpt.trigger.confidence.high.left=%f'%plow
print 'lpt.trigger.confidence.high.right=%f'%phigh
