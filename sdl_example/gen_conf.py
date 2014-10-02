conf = open('regaa_jc118_all.conf', 'w')

el = 0

ypos = 70
# between channels
dy = 25
# between tetrodes
dyt = 40
skip = [1]
nel = 4

conf.write("""regaa2.0
0
1820
1014
30
50
10
1
31
""")

col = 0

for i in range(el, el + 32):
	if i in skip:
		continue
	conf.write(str(i) + ' 0 ' + str(ypos) + ' 0.005 ' + str(col) + '\n')

	if not ((i+1) % nel):
		ypos += dyt
		col += 1
	else:
		ypos += dy

conf.write("""0
0
0
0
0
0""")
