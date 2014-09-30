conf = open('regaa_jc118_all.conf', 'w')

el = 0

ypos = 70
# between channels
dy = 25
# between tetrodes
dyt = 40
skip = [5]
nel = 4

conf.write("""regaa2.0
0
1820
1014
30
50
10
1
28
""")

col = 0

for i in range(5,  61, 2):
	if i in skip:
		j = i + 1
	else:
		j = i

	col= (j - 1) / 4

	conf.write(str(j) + ' 0 ' + str(ypos) + ' 0.005 ' + str(col) + '\n')

	if not ((j+1) % nel):
		ypos += dyt
	else:
		ypos += dy

conf.write("""0
0
0
0
0
0""")
