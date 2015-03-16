from sys import argv

conf = open(argv[1], 'w')

# staring electrode
el = 40

# starting pos, 70 for 32 channels, 300 for 16 channels per screen
ypos = 300
# between channels
dy = 25
# between tetrodes, 40 for 32 channels per screen, 80 for 16 channels
dyt = 80
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
16
""")

col = 0

for i in range(el, el + 16):
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
