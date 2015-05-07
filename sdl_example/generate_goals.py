from PIL import Image, ImageDraw, ImageFont
from math import cos, sin, pi, sqrt
import random
import itertools

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
	while True:
		inds = []
		for i in range(0,3):
			inds.append(wells[random.randint(0, len(wells) - 1)])

		# too close to each other
		if dist(inds[0], inds[1]) < thold or dist(inds[1], inds[2]) < thold or dist(inds[0], inds[2]) < thold:
			continue

		failed = False
		# too close to other goals
		for i in range(0,3):
			for j in range(0,3):
				if dist(inds[i], goals[j]) < thold:
					failed = True

		# too close to the trajectory
		#for i in range(0, len(whl), 1):
		#	for j in range(0,3):
		#		pnt = [inds[j][0]*30 +25, inds[j][1]*30 + 25]
		#		if dist(pnt, whl[i]) < 1600:
		#			failed = True
		#			break
		#	if failed:
		#		break

		# too close to the SB
		for i in range(0,3):
			if inds[i][1] > 10:
				failed = True
				break

		center = False
		# none in the center
		for i in range(0,3):
			if inds[i][0] in range(5,10) and inds[i][1] in range(5,10):
				center = True

		# no goals in left third or right third
		left_third = False
		right_third = False
		for i in range(0,3):
			if inds[i][0] < 5:
				left_third = True
			if inds[i][0] > 9:
				right_third = True
			
		if not center or not left_third or not right_third:
			failed = True

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

# add other corners
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

# excluding the outer
wells_inner = []
for w in wells:
	nin = 0
	for d in [[-1,0], [0, -1], [0, 1], [1, 0]]:
		if [w[0] + d[0], w[1] + d[1]] in wells:
			nin += 1

	if nin == 4:
		wells_inner.append(w)
		
# print len(excl)
# print excl
# print len(wells)
# print wells

# Env2 - 0402
# goals = [[13,7], [4,7], [5,3]]
goals = [[2,3], [10,2], [11,10]]

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
print '	6) present in left and right thirds of the S.B.'
print '	7) not in the outer circle'

# geenrate sets new goals, thold = 10
for d in range(0, 7):
	im = Image.new('RGBA', (470, 600), color = (255,255,255,0))
	draw = ImageDraw.Draw(im)

	inds = generate_new_goals(goals, wells_inner, 11)

	for [x,y] in wells:
		# print [x,y]
		r = 15 if [x,y] in goals or [x,y] in inds else 7
		xe = x * 30 + 25
		ye = y * 30 + 25
		draw.ellipse((xe - r, ye - r, xe + r, ye + r), fill='red' if [x,y] in goals else ('green' if [x,y] in inds else 'blue'))

	# draw_whl(whl)

	font = ImageFont.truetype("/usr/share/fonts/liberation/LiberationSerif-Bold.ttf", 20)
	draw.text((10, 0), 'Day ' + str(d), (0,0,0), font=font)
	
	
	# find shortest path between goals
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

	im.save('goals_' + str(d) + '.jpg')
	im.show()

	goals = inds
