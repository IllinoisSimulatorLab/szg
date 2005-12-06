###################################################################################################
# Based on the szglorenz.cpp program of Wayne City High School (Illinois). Thanks Stan Blank!
# This program demonstrates using the Myriad scene graph, including some of the built-in shapes
# and line strips. It also shows how to use materials and lighting.
###################################################################################################

###################################################################################################
# Exercises:
#  1. Add the ability to grab the attractor and rotate it around in response to the wand.
#  2. Make it possible to reset the attractor without restarting the application.
#  3. Make the particles nonuniform! Maybe some will have tails and some not. Maybe they will
#     have different shapes.
###################################################################################################

# Import the Syzygy shared libraries.
from PySZG import *
# Need the following module for sys.argv, both to parse args AND to pass sys.argv to the framework init.
import sys
# Need the following module so we can make the animation framerate independent.
import time
# If the user wants cubes as the particle shape, we give them a random angle.
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
		# The scene graph node that says how many line segments to draw for the tail.
		self.drawable = None
		# Default material for drawing the object.
		self.mat = arMaterial()
		# The default color (for diffuse lighting) is pure red.
		self.mat.diffuse = arVector3(1,0,0)
		# By default, a faint white color for specular highlights.
		self.mat.specular = arVector3(0.2, 0.2, 0.2)
		# The object, by default, has a very slight green glow.
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
			self.drawable = state.new("drawable")
			self.drawable.set(("lines",self.tailLength))
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
			self.pointsNode.set((self.x, self.oldX), (2*self.tailPosition, 2*self.tailPosition+1))
			self.tailPosition  += 1
			if self.tailPosition >= self.tailLength:
				self.tailPosition = 0

# The scene graph needs lights since we are using lighting.
def addLights(g):
	# The lights will be attached directly to the unique root node of the database.
	r = g.getRoot()
	# Create a new light node for the scene graph.
	l = r.new("light")
	# Create a new light description. This will be used to configure the scene graph node.
	light = arLight()
	# It is important to give each light a unique ID (0-7).
	light.lightID = 0
	# The fourth position being 0 means this is a directional light (as in OpenGL).
	# If it were 1, this would be a positional light.
	light.position = arVector4(0,0,-1,0)
	# Non-directional component of the light (with RGB color components), as in OpenGL.
        light.ambient = arVector3(0,0,0)
	# The directional component of the light (with RGB color components), as in OpenGL.
        light.diffuse = arVector3(0.5, 0.5, 0.5)
	# The scene graph node is now configured with the light description.
	l.setLight(light)
	l = r.new("light")
	light = arLight()
	# It is important to give each light a unique ID (0-7). If we repeated an ID,
	# one of the lights with the same ID would be lost.
	light.lightID = 1
	# The fourth position being 0 means this is a directional light (as in OpenGL).
	# If it were 1, this would be a positional light.
	light.position = arVector4(0,0,1,0)
        light.ambient = arVector3(0,0,0)
        light.diffuse = arVector3(0.5, 0.5, 0.5)
	l.setLight(light)
	
##########################################################################
#                           Main Program                                 #
##########################################################################

# IMPORTANT NOTE ABOUT SYZYGY SCENE GRAPH PROGRAMS: 
# In "phleet mode", these programs do not have a display themselves, but 
# instead depend on an external szgrender program.
# In "standalone mode", these programs do have a display.

# This is a Syzygy "distributed scene graph framework" program.
# We need a framework object.
f = arDistSceneGraphFramework()

# Distributed scene graph programs can run in one of two modes:
#  Automatic graphics buffer swap: In this case, the library
#   itself decides when the buffer swap should occur.
#   This is the default.
#  Manual graphics buffer swap: The programmer is responsible
#   for requesting a buffer swap. This is better for times
#   when precise control over the contents of successive frames
#   is desired.
# We choose manual swap (by setting auto swap to false). 
# This must occur BEFORE f.init(...).
f.setAutoBufferSwap(0)

# This initializes the framework, setting mode of operation and configuration.
# Configuration information comes from the Phleet (the Syzygy distributed OS).
# PLEASE NOTE: All Syzygy programs accept a collection of "special" Phleet args
# (of the form -szg foo=bar). These are stripped from the command line by init.
# Consequently, the framework init must come BEFORE argument parsing!
if f.init(sys.argv) != 1:
	sys.exit()

# Argument parsing. The following are valid invocations of the program.
#  python lorenz.py
#    80 particles, no tails, cube shapes
#  python lorenz.py 200
#    200 particles, no tails, cube shapes
#  python lorenz.py 100 1
#    100 particles. tails, cube shapes
#  python lorenz.py 500 0 sphere
#    500 particles, no tails, sphere shapes

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

# The framework includes a default navigator (implemented by the Syzygy interaction framework).
# It essentially allows the user to fly around the world along (or transverse) the direction of
# the wand. The speed is set to 0.2 feet per second.
f.setNavTransSpeed(0.2)
# The addLights(...) function expects to be handed an arGraphicsDatabase. We get the
# database that the framework object holds.
g = f.getDatabase()
# The Myriad scene graph uses lighing by default. Consequently, it is VERY important to
# add lights to it! Otherwise you won't see anything. NOTE: it is actually possible to
# turn off lighting in a scene graph (or portions thereof). Please see the documentation.
addLights(g)
# The navigation node is a special scene graph node created by the framework itself. This
# node is altered by the framework navigation calls and all scene graph geometry that
# should be affected by navigation must go below it in the scene graph tree.
# NOTE: sometimes one does NOT want objects to navigate (like tools that always follow the
# user). These could be attached, for instance, to the scene graph's root node.
r = f.getNavNode()
# We want to put all our objects in the default Syzygy viewing position (center of front
# CAVE wall, (0,5,-5)) and scale things so that the lorenz attractor is of reasonable size.
# Create a new node w as a child of the nav node.
w = r.new("transform")
# ar_TM is an abbreviation for ar_translationMatrix and gives an arMatrix4 with the
# appropriate translation. ar_SM is an abbreviation for ar_scaleMatrix and gives an 
# arMatrix4 with the appropriate uniform scaling.
w.set(ar_TM(0,5,-5)*ar_SM(0.1))
# Populate the list of particles.
plist = []
# The number of particles is determined by the command line args.
for i in range(number):
	p = particle()
	# Set the particle shape based on the command line args.
	p.type = shape
	# Whether or not the particle has a tail is determined by the command line args,
	p.useTail = useTail
	# The particles are staggered slightly from the default initial position. 
	# This demonstrated sensitive dependence on initial conditions (one form of chaos).
	p.x[2] = p.x[2] + i*0.03
	# It's a better visualization if the particles start out as a rainbow hue.
	# This is especially good for observing the *mixing* that is another form of chaos.
	p.mat.diffuse = arVector3(1 - (i+1)/(1.0*(number)), 0, (i+1)/(1.0*(number)))
	# Add nodes to the scene graph. Recall that the scene graph has a tree structure.
	# In this case, we are adding the particle under the node w.
	p.attach(w)
	plist.append(p)

# Starts all the services, windows, etc. NOTE: it makes the most sense to do
# this AFTER lengthy initializations of the scene graph (though you can issue
# this statement any time after initializing the framework).
if f.start() != 1:
	sys.exit()

# Main loop.
while 1:
	# Get the user head position/orientation from control device and post to
	# the framework. By making this step explicit, we allow greater user control.
	# Without this statement, the scene will not change in response to head
	# position changes!
	f.setViewer()
	# Update the framework's navigation matrix. This processes user input and
	# changes the navigation matrix held inside the framework (but does not
	# update the scene graph yet). Without this call, no navigation will occur!
	f.navUpdate()
	# This actually loads the navigation matrix into the scene graph. Without this
	# call, no navigation will occur!
        f.loadNavMatrix()
	# Go through the list of particles, updating their positions, trails (if selected),
	# and graphical representation in the scene graph.
	for i in plist:
		i.advance()
	# We have chosen the manual swap mode of operation for the scene graph framework.
	# Consequently, we must issue the buffer swap command ourselves.
	f.swapBuffers()
