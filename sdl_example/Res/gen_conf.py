f = open('tetr_jc84.conf', 'w')
f.write('15\n')

for i in range(0, 12)+range(13,16):
	f.write('4\n')
	for a in range(0, 4):
		f.write(str(4*i + a) + ' ')
	f.write('\n')
