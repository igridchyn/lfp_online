from PIL import Image, ImageDraw, ImageFont
from math import cos, sin, pi, sqrt
import random
import itertools
from sys import argv

class EnvTransform:
	def __init__(self, sc, rot, x, y):
		self.scale = sc
		self.rotation = rot
		self.dx = x
		self.dy = y

def rotate(point, origin, angle):
	diff = [point[0]-origin[0], point[1]-origin[1]]
	ndiff = [diff[0] * cos(angle) + diff[1] * sin(angle), - diff[0] * sin(angle) + diff[1] * cos(angle)]
	return [ndiff[0] + origin[0], ndiff[1] + origin[1]]
#========================================================================================================================
def dist(p1, p2):
	return (p1[0] - p2[0])**2 + (p1[1] - p2[1])**2
#========================================================================================================================
def generate_new_goals(goals, wells, thold):
# geenrate new goals
	rfailed = [0] * 10
	niter = 0
	# print 'Start goal generation with threshold %d' % thold
	# print 'With goals at ', goals
	# print 'And wells: ', wells
	# print '\t (%d wells)' % len(wells)
	max_iter = 100000
	# print 'With max iterations: %d' % max_iter
	while True:
		failed = False
		niter += 1

		if niter % 50000 == 0:
			print str(niter) + ' iterations tried, failed: ' + str([r / float(niter) for r in rfailed])

		if niter >= max_iter:
			return []

		inds = []
		for i in range(0,3):
			inds.append(wells[random.randint(0, len(wells) - 1)])

		# 1) too close to each other
		if dist(inds[0], inds[1]) < thold or dist(inds[1], inds[2]) < thold or dist(inds[0], inds[2]) < thold:
			rfailed[0] += 1
			failed = True

		# 2) too close to other goals
		failed2 = False
		for i in range(0,3):
			if failed2:
				break
			for j in range(0, len(goals)):
				if dist(inds[i], goals[j]) < thold:
					rfailed[1] += 1
					failed = True
					failed2 = True
					break

		# too close to the trajectory
		#for i in range(0, len(whl), 1):
		#	for j in range(0,3):
		#		pnt = [inds[j][0]*30 +25, inds[j][1]*30 + 25]
		#		if dist(pnt, whl[i]) < 1600:
		#			failed = True
		#			break
		#	if failed:
		#		break

		# 4) too close to the SB
		for i in range(0,3):
			if inds[i][1] > 10 and inds[i][0] > 6:
				rfailed[3] += 1
				failed = True
				break

		center = False
		# 5) none in the center
		crad = 5
		for i in range(0,3):
			if inds[i][0] in range(crad,15-crad) and inds[i][1] in range(crad,15-crad):
				center = True
		if not center:
			rfailed[4] += 1
			failed = True

		# 6) goals should be  in both left third and right third, and top third
		left_third = False
		right_third = False
		top = False
		for i in range(0,3):
			if inds[i][0] < 5:
				left_third = True
			if inds[i][0] > 9:
				right_third = True			
			if inds[i][1] < 5:
				top = True
		if not left_third or not right_third or not top:
			rfailed[5] += 1
			failed = True
		
		# 8) no more than one s.t. disatnce to border is < 3
		nclose = 0
		bordprox = 3
		for i in range(0, 3):
			for (x,y) in [(-bordprox, 0), (bordprox, 0), (0, -bordprox), (0, bordprox)]:
				shiftp = [inds[i][0]+x, inds[i][1]+y]
				if not (shiftp in wells):
					# print shiftp
					nclose += 1
					break
		if nclose > 1:
			rfailed[7] += 1
			failed = True

		# are all conditions satisfied ?
		if failed:
			continue

	        break

	return inds
#========================================================================================================================
def draw_whl(whl):
	for i in range(0,len(whl),1):
		draw.point([whl[i][0], whl[i][1]], fill='blue')
#========================================================================================================================
def well_to_coords(well):
	return [well[0] * 30 + 25, well[1] * 30 + 25]
#========================================================================================================================


# Env 2
# tr = EnvTransform(1.05, pi * 0.205, -65, -10)
# Evn 1
# tr = EnvTransform(1.03, -pi * 0.027, -6, -54)
tr = EnvTransform(1.02, -pi * 0.023, -6, -52)

excl = [[0,4], [0,3], [0,2], [0,1], [0,0], [1,2], [1,1], [1,0], [2,1], [2,0], [3,0], [4,0]]
excl.extend([[0,4], [0,3], [0,2], [0,1], [0,0], [1,2], [1,1], [1,0], [2,1], [2,0], [3,0], [4,0]])
excl.extend([[0,4], [0,3], [0,2], [0,1], [0,0], [1,2], [1,1], [1,0], [2,1], [2,0], [3,0], [4,0]])
excl.extend([[0,4], [0,3], [0,2], [0,1], [0,0], [1,2], [1,1], [1,0], [2,1], [2,0], [3,0], [4,0]])
ncorn = 12
# excl = excl * 4

# modify excl to represent 4 quarters add other corners
for i in range(ncorn, 3*ncorn):
	excl[i][0] = 14 - excl[i][0]
for i in range(2*ncorn, 4*ncorn):
	excl[i][1] = 14 - excl[i][1]

wells = []
# make list of wells
for x in range(0,15):
	for y in range(0,15):
		# check if corner
		if [x,y] in excl:
			# print 'Skip'
			# print [x,y]
			continue
		
		wells.append([x,y])

# excluding the outer circle
wells_inner = wells
# for w in wells:
#	nin = 0
#	for d in [[-1,0], [0, -1], [0, 1], [1, 0]]:
#		if [w[0] + d[0], w[1] + d[1]] in wells:
#			nin += 1
#
#	if nin == 4:
#		wells_inner.append(w)
		
# print len(excl)
# print excl
# print len(wells)
# print wells

# Env2 - 0402
# goals = [[13,7], [4,7], [5,3]]
# goals = [[4,9], [8,8], [10,3]]
# goals = [[0,8], [5,6], [12,7]] if len(argv) < 3 else []
# goals = [[3,2], [5,5], [11,10]] if len(argv) < 3 else []
# goals = [[3,3], [8,5], [10, 9]]
goals = [[11,1], [5,6], [4,9]]
# goals = [[2,2], [7,5], [10,7]]
initial_goals = goals

# tracking
# ftra = open('/hd1/data/bindata/jc140/0402/jc140_0402_21calib1.axtrk')
# ftra = open('/hd1/data/bindata/jc140/0402/jc140_0402_16l2.axtrk')
ftra = open('/hd1/data/bindata/jc140/0402/jc140_0402_15l1.axtrk')
ftra.readline()
whl = []
scale = tr.scale
for line in ftra:
        tmp = line.split('\t')
        # print tmp
        whl.append([float(tmp[3])*scale, float(tmp[4])*scale])

for i in range(0, len(whl)):
        nwhl = rotate(whl[i], [225, 225], tr.rotation)
        whl[i][0] = nwhl[0] + tr.dx
        whl[i][1] = nwhl[1] + tr.dy

print 'Rules used for goal generation: goals should be'
print '	1) apart from each other'
print '	2) apart from previous goals'
print '	3) apart from previous trajectory (last few trials)'
print '	4) not too close to the S.B.'
print '	5) in the central 5 X 5 square'
print '	6) present in left and right thirds of the S.B. and top third'
print '	7) not in the outer circle'
print '	8) no more than one s.t. the distance to border < 3'

thold = 8 if len(argv) < 2 else float(argv[1])

# geenrate sets new goals, thold = 10
goal_shift = 0
d = 0
nreset = 0
while d < 10:
	d += 1

	im = Image.new('RGBA', (470, 470), color = (255,255,255,0))
	draw = ImageDraw.Draw(im)

	inds = generate_new_goals(goals, wells_inner, thold)

	# if failed: restart
	if len(inds) == 0:
		d = 0
		goals = initial_goals
		nreset += 1
		print 'Failed to find sequence (for %d-th time), start from beginning...' % nreset
		continue

	for [x,y] in wells:
		# print [x,y]
		r = 15 if [x,y] in goals or [x,y] in inds else 7
		xe = x * 30 + 25
		ye = y * 30 + 25
		draw.ellipse((xe - r, ye - r, xe + r, ye + r), fill='red' if [x,y] in goals else ('green' if [x,y] in inds else 'blue'))

	# draw_whl(whl)

	font = ImageFont.truetype("/usr/share/fonts/liberation/LiberationSerif-Bold.ttf", 20)
	draw.text((10, 0), 'Day ' + str(d+goal_shift), (0,0,0), font=font)
	
	
	# find shortest path between goals
	if len(goals) > 0:
		shortest_dist = 10000000
		shortest_perm = []
		for perm in itertools.permutations(range(0,3)):
			pathlen = sqrt(dist([7, 14], goals[perm[0]]))
			for i in range(1,3):
				pathlen += sqrt(dist(goals[perm[i-1]], goals[perm[i]]))
			pathlen += sqrt(dist(goals[perm[2]], [7, 14]))
			if pathlen < shortest_dist:
				shortest_dist = pathlen
				shortest_perm = perm

		# draw shortest path
		shortest_goals = [[7, 14]]
		for i in range(0, 3):
			shortest_goals.append(goals[shortest_perm[i]])
		shortest_goals.append([7, 14])
		for i in range(1,5):
			start = well_to_coords(shortest_goals[i-1])
			end = well_to_coords(shortest_goals[i])
			draw.line((start[0], start[1], end[0], end[1]), fill = 'black', width = 5)

	im.save('goals_' + str(d+goal_shift) + '.jpg')
	im.show()

	goals = inds
