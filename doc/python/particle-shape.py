from PySZG import *
from math import *
import sys
import time
import random

class particle:
	def __init__(self):
		self.x = arVector3(0,0,0)
		self.v = arVector3(0,0,0)
		self.a = arVector3(0,0,0)
		self.maxDelta=0.1
		self.rotationAngle = 6.283*random.random()
		self.rotationSpeed = 1 + 2*random.random()
		self.fric = 0.8
		self.speedCutoff = 1
		self.floor = 5
		self.type = 'torus'
		self.numberBounces = 0
		self.maxBounces = 3
		self.tailPosition = 0
		self.tailLength = 100
		self.transformNode = None
		self.rotationNode = None
		self.pointsNode = None
		self.drawableNode = None
		self.parent = None
		self.root = None
		self.useTail = 1
	def attach(self, parent):
		if self.root:
			d = self.root.getOwner()
			d.eraseNode(self.root)
		self.parent = parent
		self.root = parent.new("name")
		m = gcast(self.root.new("material"))
		mat = arMaterial()
		mat.diffuse = arVector3(random.random(),random.random(),random.random())
		mat.specular = arVector3(0.2, 0.2, 0.2)
		mat.emissive = arVector3(0,0,0.1)
		mat.exponent = 100
		m.set(mat)
		t = m.new("transform")
		self.transformNode = t
		r = t.new("transform")
		self.rotationNode = r
		s = r.new("transform")
		choices = [ 'sphere', 'torus', 'cube', 'cylinder' ]
		self.type = random.choice(choices)
		if self.type == 'sphere':
			s.set(ar_SM(0.5))
			sph = arSphereMesh()
			sph.setAttributes(15)
			sph.attachMesh(s)
		elif self.type == 'torus':
			s.set(ar_SM(1))
			sph = arTorusMesh(20,20,0.5, 0.1)
			sph.attachMesh(s)
		elif self.type == 'cube':
			s.set(ar_SM(0.5))
			sph = arCubeMesh()
			sph.attachMesh(s)
		elif self.type == 'cylinder':
			s.set(ar_SM(0.5))
			sph = arCylinderMesh()
			sph.setTransform(ar_SM(0.3, 0.3, 1))
			sph.setAttributes(20,1,1)
			sph.toggleEnds(1)
			sph.attachMesh(s)
		if self.useTail:
			points = m.new("points")
			self.pointsNode = points
			state = points.new("graphics state")
			state.set(("line_width",1+4*random.random()))
			draw = state.new("drawable")
			self.drawableNode = draw
			draw.set(("lines",self.tailLength))	
	def reset(self):
		self.x = arVector3(0,self.floor,-5)
		angle = random.random()*6.283
		self.v = arVector3(cos(angle),1+4*random.random(),sin(angle))
		self.numberBounces = 0
		self.tail = []
		self.attach(self.parent)
	def adv(self, delta):
		# self.oldX is set to the self.x object. These are now essentially pointers to the same object.
		# However, self.x is set to a (new) object in the math below, so this is OK.
		self.oldX = self.x
		self.rotationAngle += delta * self.rotationSpeed
		iters = -1
		if delta > self.maxDelta:
			iters = floor(delta/self.maxDelta)
			rmdr = delta - iters*self.maxDelta
		if iters < 1:
			self.x = self.x + self.v * delta
			self.v = self.v + self.a * delta
		else:
			for i in range(iters+1):
				if i == 1:
					rmdr = self.maxDelta
				self.x = self.x + self.v * rmdr
				self.v = self.v + self.a * rmdr
		self.resetFlag = 0
		if self.boundary():
			self.reset()
			self.resetFlag = 1
		if self.useTail:
			self.tailPosition  += 1
			if self.tailPosition >= self.tailLength:
				self.tailPosition = 0
			if self.pointsNode and not self.resetFlag:
				self.pointsNode.set((self.x, self.oldX), (2*self.tailPosition, 2*self.tailPosition+1))
		self.post()
	def boundary(self):
		if self.x[1] < self.floor:
			self.x[1] = self.floor
			tmp = ar_reflect(self.v, arVector3(0,1,0))
			self.v = arVector3(self.fric*tmp[0], self.fric*tmp[1], self.fric*tmp[2])
			self.numberBounces = self.numberBounces + 1
			if self.v.magnitude() < self.speedCutoff or self.numberBounces >= self.maxBounces:
				return 1
		return 0
	def post(self):
		if self.transformNode != None:
			self.transformNode.set(ar_TM(self.x))
		if self.rotationNode != None:
			self.rotationNode.set(ar_RM('y',self.rotationAngle))
		

def addLights(g):
	r = g.getRoot()
	l = gcast(r.new("light"))
	light = arLight()
	light.lightID = 0
	light.position = arVector4(0,0,-1,0)
        light.ambient = arVector3(0,0,0)
        light.diffuse = arVector3(0.5, 0.5, 0.5)
	l.setLight(light)
	l = gcast(r.new("light"))
	light = arLight()
	light.lightID = 1
	light.position = arVector4(0,0,1,0)
        light.ambient = arVector3(0,0,0)
        light.diffuse = arVector3(0.5, 0.5, 0.5)
	l.setLight(light)

if len(sys.argv) > 1:
	number = int(sys.argv[1])
else:
	number = 10
if len(sys.argv) > 2 and int(sys.argv[2]) > 0:
	useTail = 1
else:
	useTail = 0

f = arDistSceneGraphFramework()
f.setAutoBufferSwap(0)
if f.init(sys.argv) != 1:
	sys.exit()

f.setNavTransSpeed(0.2)
g = f.getDatabase()
addLights(g)
r = f.getNavNode()

grav = arVector3(0,-1,0)
plist = []
for i in range(number):
	p = particle()
	p.useTail = useTail
	p.parent = r
	p.reset()
	p.a = grav
	plist.append(p)

if f.start() != 1:
	sys.exit()
now = time.clock()
change = 0
while 1:
	f.setViewer()
	f.navUpdate()
        f.loadNavMatrix()
	for p in plist:
		p.adv(change)
	temp = time.clock()
	change = temp-now
	now = temp
	f.swapBuffers()
