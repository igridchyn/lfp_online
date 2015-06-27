from sys import argv

conf = open(argv[1], 'w')

# staring electrode
el = 0

# starting pos, 70 for 32 channels, 300 for 16 channels per screen
ypos = 100
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
24
""")

col = 0

# LEFT
# channels = range(0,16)
# channels.extend(range(48, 56))

# RIGHT
channels = range(0, 32)
# channels.extend(range(40, 48))

for i in channels:
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
