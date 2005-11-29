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
		self.rotationAngle = 0
		self.rotationSpeed = 3
		self.fric = 0.8
		self.speedCutoff = 1
		self.floor = 5
		self.type = 'torus'
		self.tail = []
		self.tailLength = 100
		self.transformNode = None
		self.rotationNode = None
		self.pointsNode = None
		self.drawableNode = None
		self.parent = None
		self.root = None
		self.useTail = 0
	def attach(self, parent):
		if self.root:
			d = self.root.getOwner()
			d.eraseNode(self.root)
		self.parent = parent
		self.root = parent.new("name")
		m = gcast(self.root.new("material"))
		mat = arMaterial()
		mat.diffuse = arVector3(1,0,0)
		mat.specular = arVector3(0.2, 0.2, 0.2)
		mat.emissive = arVector3(0,0,0.1)
		mat.exponent = 100
		m.set(mat)
		t = m.new("transform","sphere_trans")
		self.transformNode = t
		r = t.new("transform")
		self.rotationNode = r
		s = r.new("transform","sphere_scl_"+str(self))
		s.set(ar_SM(1))
		if self.type == 'sphere':
			sph = arSphereMesh()
			sph.setAttributes(15)
			sph.attachMesh("sphere_"+str(self),"sphere_scl_"+str(self))
		elif self.type == 'torus':
			sph = arTorusMesh(20,20,0.5, 0.1)
			sph.attachMesh("sphere_"+str(self),"sphere_scl_"+str(self))
		elif self.type == 'cube':
			sph = arCubeMesh()
			sph.attachMesh("sphere_"+str(self),"sphere_scl_"+str(self))
		elif self.type == 'cylinder':
			sph = arCylinderMesh()
			sph.setAttributes(20,1,1)
			sph.toggleEnds(1)
			sph.attachMesh("sphere_"+str(self),"sphere_scl_"+str(self))
		if self.useTail:
			points = m.new("points")
			self.pointsNode = points
			points.set(p.tail)
			state = points.new("graphics state")
			state.set(("line_width",5))
			draw = state.new("drawable")
			self.drawableNode = draw
			draw.set(("line_strip",1000))	
	def reset(self):
		self.x = arVector3(0,self.floor,-5)
		angle = random.random()*6.283
		self.v = arVector3(cos(angle),1+4*random.random(),sin(angle))
		self.tail = []
		self.attach(self.parent)
	def adv(self, delta):
		self.rotationAngle += delta * self.rotationSpeed
		iters = -1
		if delta > self.maxDelta:
			iters = floor(delta/self.maxDelta)
			rmdr = delta - iters*self.maxDelta
		if iters < 1:
			self.x = self.x + self.v * delta
			self.v = self.v + self.a * delta
			#self.x = self.x + arVector3(self.v[0]*delta,self.v[1]*delta,self.v[2]*delta)
			#self.v = self.v + arVector3(self.a[0]*delta,self.a[1]*delta,self.a[2]*delta)
		else:
			for i in range(iters+1):
				if i == 1:
					rmdr = self.maxDelta
				self.x = self.x + self.v * rmdr
				self.v = self.v + self.a * rmdr
				#self.x = self.x + arVector3(self.v[0]*rmdr,self.v[1]*rmdr,self.v[2]*rmdr)
				#self.v = self.v + arVector3(self.a[0]*rmdr,self.a[1]*rmdr,self.a[2]*rmdr)
		if self.boundary():
			self.reset()
		if self.useTail:
			self.tail.append(self.x)
			if len(self.tail) > self.tailLength:
				self.tail = self.tail[1:]
		self.post()
	def boundary(self):
		if self.x[1] < self.floor:
			self.x[1] = self.floor
			tmp = ar_reflect(self.v, arVector3(0,1,0))
			self.v = arVector3(self.fric*tmp[0], self.fric*tmp[1], self.fric*tmp[2])
			if self.v.magnitude() < self.speedCutoff:
				return 1
		return 0
	def post(self):
		if self.transformNode != None:
			self.transformNode.set(ar_TM(self.x))
		if self.rotationNode != None:
			self.rotationNode.set(ar_RM('y',self.rotationAngle))
		if self.pointsNode != None:
			self.pointsNode.set(self.tail)
		if self.drawableNode != None:
			self.drawableNode.set(('line_strip',len(self.tail)-1))
		

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
	#time.sleep(0.02)
	temp = time.clock()
	change = temp-now
	now = temp
	f.swapBuffers()
