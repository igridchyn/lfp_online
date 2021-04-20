#!/usr/bin/env python

from sys import argv
import numpy as np
from matplotlib import pyplot
import os
from iutils import *
from call_log import *

# extract session-wise clu and res from the medged one
if len(argv) < 4:
	print 'USAGE: (1)<base> (2)<shift> (3)<base output>'
	exit(0)

base = argv[1]
shift = int(argv[2])
baseo = argv[3]

clu = read_int_list(base + 'clu')
res = read_int_list(base + 'res')

i = 0
while i < len(res) and res[i] < shift:
	i += 1

if os.path.isfile(baseo + 'res') or os.path.isfile(baseo + 'clu'):
	print 'ERROR: res or clu output file exists'
	exit(1)

freso = open(baseo + 'res', 'w')
fcluo = open(baseo + 'clu', 'w')

while i < len(res):
	fcluo.write('%d\n' % clu[i])
	freso.write('%d\n' % (res[i] - shift))
	i += 1
