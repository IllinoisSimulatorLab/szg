# Based on the szglorenz.cpp program of Wayne City High School

from PySZG import *
import sys
import time
import random

class particle:
	# The constructor.
	def __init__(self):
		# Default initial position of the object. Chosen so that the object is inside the lorenz attractor
		# at the start.
		self.x = arVector3(8,8,23)
		# A place-holder used to draw the trails, if such are used.
		self.oldX = arVector3(0,0,0)
		# Should this object have a tail or not?
		self.useTail = 1
		# How wide should the tail be? Can be any floating point number.
		self.lineWidth = 5.0
		# Which line segment in the tail should be replaced next? The storage is like a circular buffer.
		self.tailPosition = 0
		# How many line segments in the tail? You can change this value for different visual effects.
		self.tailLength = 100
		# The scene graph node that holds the positions used to draw the tail line segments. Undefined
		# upon object creation (see attach method).
		self.pointsNode = None
		# Default material for drawing the object.
		self.mat = arMaterial()
		self.mat.diffuse = arVector3(1,0,0)
		self.mat.specular = arVector3(0.2, 0.2, 0.2)
		self.mat.emissive = arVector3(0,0,0.1)
		self.mat.exponent = 100
		# What shape should we use ('cube' and 'sphere' are valid)?
		self.type = 'cube'
		# The transform node (of the scene graph) used to place the object. 
		# Undefined upon object creation (see attach method).
		self.trans = None

	# Puts geometry in the scene graph associated with this object. This should only be called once.
	# Note how the object stores references to particular scene graph nodes (which will be used later
	# to manipulate the object's visual representation). In the scene graph tree, the nodes for this
	# object will be attached to node n.
	def attach(self, n):
		# Create a material node using the material info stored in this object. All other nodes for 
		# this object are attached to node m.
		m = n.new("material")
		m.set(self.mat)
		# If we are using a tail, this adds the appropriate scene graph nodes.
		if self.useTail:
			# The points (arVector3's) stored in the points node are the endpoints of the
			# line segments in the object's tail.
			self.pointsNode = m.new("points")
			# We want to be able to vary the line width from the default value of 1 to get a fatter tail.
			state = self.pointsNode.new("graphics state")
			state.set(("line_width", self.lineWidth))
			# This node is what actually draws the line segments for the tail. It inherits the points from
			# its pointsNode ancestor and the line width from its state ancestor node.
			draw = state.new("drawable")
			draw.set(("lines",self.tailLength))
		# The transform node is a child of node m and positions the object.
		self.trans = m.new("transform")
		# Set the value of the transform node. ar_TM is an abbreviation for ar_translationMatrix and
		# converts the arVector3 into a corresponding translation (an arMatrix4).
		self.trans.set(ar_TM(self.x))
		# Also want to change the size and orientation of the attached object. Changing the orientation is
		# especially important for the cube shapes (otherwise it looks too uniform).
		scl = self.trans.new("transform")
		# ar_RM is an abbreviation for ar_rotationMatrix. We could also rotate around axis 'x' or 'z'.
		# The angle is in radians.
		# ar_SM is an abbreviation for ar_scaleMatrix. It returns an arMatrix4 with uniform scaling.
		scl.set(ar_RM('y', 6.283*random.random())*ar_SM(2))
		# What shape should we attach? The geometry nodes will be descendants of node scl in the scene graph
		# tree.
		if self.type == 'cube':
			s = arCubeMesh()
			s.attachMesh(scl)
		elif self.type == 'sphere':
			s = arSphereMesh()
			# The number of divisions in the sphere. It will have about divisions*divisions*2 triangles.
			s.setAttributes(15)
			s.attachMesh(scl)

	# Advance the position of the object (and update its trail if such has been enabled) according to the
	# lorenz ODE.
	def advance(self):
		# Marshal the position for efficient python math.
		x = self.x[0]
		y = self.x[1]
		z = self.x[2]
		# NOTE: self.oldX = self.x WILL NOT WORK here. Why? Because, under this scenario, each object
		# will point to the same vector. BUT we don't actually reassign either object in the math below.
		self.oldX[0] = x
		self.oldX[1] = y
		self.oldX[2] = z
		dt = 0.004
		# Do five iterations of the lorenz attractor ODE (multiple iterations make the objects go faster)
		for i in range(5):
			# These three lines are actually much faster than the subsequent 3 (which are included for
			# comparison purposes. Could it be that self.x[0] takes significant time in and of itself?
			x = (10*y - 10*x)*dt + x
			y = (28*x - y - x*z)*dt + y
			z = (-(2.66667)*z + x*y)*dt + z
			#self.x[0] = (10*self.x[1]-10*self.x[0])*self.dt + self.x[0]
			#self.x[1] = (28*self.x[0]-self.x[1] - self.x[0]*self.x[2])*self.dt + self.x[1]
			#self.x[2] = (-(2.66667)*self.x[2] + self.x[0]*self.x[1])*self.dt + self.x[2]
		# Demarshall the position into object storage.
		self.x[0] = x
		self.x[1] = y
		self.x[2] = z
		# Change the position of the sphere/cube.
		self.trans.set(ar_TM(self.x))
		# Update the points used to plot the lines in the trail. The trail is stored in the scene graph
		# as a line set. This means that every even indexed point is the beginning of a line segment, with the
		# following odd indexed point being the end of that segment. Consequently, at each stage we just replace
		# the oldest line segment, giving the effect of a trail of finite length.
		if self.pointsNode:
			self.tailPosition  += 1
			if self.tailPosition >= self.tailLength:
				self.tailPosition = 0
			self.pointsNode.set((self.x, self.oldX), (2*self.tailPosition, 2*self.tailPosition+1))

# The scene graph needs lights since we are using lighting.
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
	number = 80
if len(sys.argv) > 2 and int(sys.argv[2]) >= 1:
	useTail = 1
else:
	useTail = 0
if len(sys.argv) > 3 and sys.argv[3] == "sphere":
	shape = "sphere"
else:
	shape = "cube"

f = arDistSceneGraphFramework()
f.setAutoBufferSwap(0)
if f.init(sys.argv) != 1:
	sys.exit()
if f.start() != 1:
	sys.exit()

f.setNavTransSpeed(0.2)
g = f.getDatabase()
addLights(g)
r = f.getNavNode()
w = gcast(r.new("transform"))
w.set(ar_TM(0,5,-5)*ar_SM(0.1))
plist = []
for i in range(number):
	p = particle()
	p.type = shape
	p.useTail = useTail
	p.x[2] = p.x[2] + i*0.03
	p.mat.diffuse = arVector3(1 - (i+1)/(1.0*(number)), 0, (i+1)/(1.0*(number)))
	p.attach(w)
	plist.append(p)

while 1:
	f.setViewer()
	f.navUpdate()
        f.loadNavMatrix()
	for i in plist:
		i.advance()
	f.swapBuffers()
