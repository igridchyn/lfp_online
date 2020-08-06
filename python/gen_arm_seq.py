from random import randint
from sys import argv

n = int(argv[1])
excl = int(argv[2])

seq = []
while len(seq) < n:
	r = randint(1, 8)
	if r == excl or len(seq) > 0 and r == seq[-1]:
		continue
	seq.append(r)

print seq
