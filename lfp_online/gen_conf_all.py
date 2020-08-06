conf = open('regaa_jc140_all.conf', 'w')

el = 0

ypos = 70
# between channels
dy = 25
# between tetrodes
dyt = 40
skip = []
nel = 4

conf.write("""regaa2.0
0
1820
1014
30
50
10
1
24
""")

col = 0

chlist = range(0,  32, 2)
chlist.extend(range(40, 56, 2))

for i in chlist:
	if i in skip:
		j = i + 1
	else:
		j = i

	col = j / 4

	conf.write(str(j) + ' 0 ' + str(ypos) + ' 0.005 ' + str(col) + '\n')

	if not ((j-2) % nel):
		ypos += dyt
	else:
		ypos += dy

conf.write("""0
0
0
0
0
0""")
