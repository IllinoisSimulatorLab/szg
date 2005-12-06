################################################################################################################
# An example of the Myriad scene graph in action.
# Shows how to do a particle system plus simple physics (trajectory under gravity plus bouncing on a surface).
# The physics is framerate independent, with time step depending on wall clock time. A good example of using
# different shapes and materials in the scene graph. Also shows how to erase scene graph nodes (the particles
# can change shape).
################################################################################################################

################################################################################################################
# Exercises:
#  1. Add nodes to the scene graph for a floor.
#  2. Add nodes to create a fountain (from which the objects will emerge).
################################################################################################################

from PySZG import *
from math import *
import sys
import time
import random

class particle:
	# The constructor. A number of behaviors can be tweaked by adjusting the parameters below.
	def __init__(self):
		# By default, the particle is at the origin.
		self.x = arVector3(0,0,0)
		# By default, the particle is not moving.
		self.v = arVector3(0,0,0)
		# By default, the particle is not accelerating.
		self.a = arVector3(0,0,0)
		# The maximum time step for a single ODE iteration. Used to maintain stability. If the
		# user gives larger time steps to adv(...), that method will execute multiple steps of
		# length maxDelta, up to the user's requested time step.
		self.maxDelta=0.1
		# The angle around the y-axis that the particle is rotated. Random in order to give a cool effect.
		self.rotationAngle = 6.283*random.random()
		# The speed at which the particle will rotate around its y-axis. Again, this varies to prevent
		# uniformities among the particles (and too much uniformity looks weird).
		self.rotationSpeed = 1 + 2*random.random()
		# The fraction of speed the particle retains at each bounce.
		self.fric = 0.8
		# If the particle is going slower than this, we'll disappear it at the next bounce and
		# launch it again.
		self.speedCutoff = 1
		# The y coordinate of the floor (surface on which the particles bounce).
		self.floor = 0
		# What shape will the particle have?
		self.type = 'torus'
		# How many times has this particle bounced since the last reset?
		self.numberBounces = 0
		# What is the maximum number of bounces until we reset the particle?
		self.maxBounces = 3
		# If the particle has a tail, which segment is next to be replaced?
		self.tailPosition = 0
		# If the particle has a tail, what is the maximum number of line segments in it?
		self.tailLength = 100
		# Scene graph node that holds the particle's position. Will be initialized in attach(...).
		self.transformNode = None
		# Scene graph node that holds the particle's rotation. Will be initialized in attach(...).
		self.rotationNode = None
		# Scene graph node that holds the points defining the particle's tail. Only defined if we
		# are using a tail, and then only in attach(...). 
		self.pointsNode = None
		# Scene graph node that draws the particle's tail. Only defined if we are using a tail, and then
		# only in attach(...).
		self.drawableNode = None
		# This is the scene graph node to which all the particle's geometry attaches.
		self.parent = None
		# All the particle's geometry is attached to this scene graph node (which is an immediate child
		# of self.parent). Will be initialized in attach(...).
		self.root = None
		# By default, the particle has a tail.
		self.useTail = 1

	# Used to attach particle geometry to the scene graph below the parent node.
	def attach(self, parent):
		# If we've already been attached to something, delete the old geometry in preparation
		# for creating new geometry.
		if self.root:
			# The database itself is the only object empowered to delete nodes.
			# Every node is owned by a database, which we can retrieve as follows. 
			d = self.root.getOwner()
			# Delete the node and all its children like so. The scene graph nodes are reference
			# counted, so self.root will not actually get deleted until its last reference goes
			# away (look below a few lines where the name self.root gets a new object assigned).
			d.eraseNode(self.root)
		# Save the place we were attached, for future use by the reset(...) method.
		self.parent = parent
		# Create a child node (really just a "name" node placeholder) to which we'll attach everything.
		self.root = parent.new("name")
		# Attach a child node to the particle's root that stores a material. Because properties 
		# inherit DOWN the scene graph tree (from a node to its descendants) this means that nodes
		# that are in m's subtree will use this color (like the particle shape and the trail, if used).
		m = self.root.new("material")
		# The information for the material is stored in an arMaterial object. Its defaults are the
		# OpenGL defaults.
		mat = arMaterial()
		# Random color is used to give a non-uniform appearance for our particles.
		mat.diffuse = arVector3(random.random(),random.random(),random.random())
		# Slightly shiny (white). Experiment with making it more so!
		mat.specular = arVector3(0.2, 0.2, 0.2)
		# A very slight green glow.
		mat.emissive = arVector3(0,0,0.1)
		mat.exponent = 100
		# Configure the material node with the material we've defined above.
		m.set(mat)
		# The transform node is a child of the material node. It controls the position of the particle geometry.
		t = m.new("transform")
		# Keep track of the node so we can refer to it later (when advancing the ODE).
		self.transformNode = t
		# Another transform (child of the position transform). This lets us rotate the object around the y axis.
		r = t.new("transform")
		# Again, keep track of the node, so we can alter it later.
		self.rotationNode = r
		# Another transform (child of the rotation transform). This lets us scale the object.
		# Intuitively, a path of transformation nodes in the scene graph tree is like a sequence of matrices
		# on the OpenGL modelview stack.
		s = r.new("transform")
		# Pick a random shape.
		choices = [ 'sphere', 'torus', 'cube', 'cylinder' ]
		self.type = random.choice(choices)
		if self.type == 'sphere':
			# ar_SM is an abbreviation for ar_scaleMatrix and returns an arMatrix4 with appropriate uniform scaling.
			# Set the scale node with this.
			s.set(ar_SM(0.5))
			# Create a sphere object.
			sph = arSphereMesh()
			# The sphere will have 15 longitudinal and latitudinal divisions. About 2*divisions^2 triangles.
			sph.setAttributes(15)
			# Attach the sphere geometry in the scene graph below the node s.
			# NOTE: conceptually, the sphere object is like a "factory". The geometry created in the scene
			# graph does NOT refer to anything held in the mesh object.
			sph.attachMesh(s)
		elif self.type == 'torus':
			# ar_SM is an abbreviation for ar_scaleMatrix and returns an arMatrix4 with appropriate uniform scaling.
			# Set the scale node with this.
			s.set(ar_SM(1))
			# Create a new torus object.
			#  1st parameter: number of divisions around the "big radius", i.e. around the hole.
			#  2nd parameter: number of divisions around the "small radius", i.e. around the tube.
			#  3rd parameter: the "big radius", i.e. the distance from the center to the tube's middle.
			#  4th parameter: the "small radius", i.e. the radius of the tube.
			sph = arTorusMesh(20,20,0.5, 0.1)
			# Attach the sphere geometry in the scene graph below the node s.
			# NOTE: conceptually, the sphere object is like a "factory". The geometry created in the scene
			# graph does NOT refer to anything held in the mesh object.
			sph.attachMesh(s)
		elif self.type == 'cube':
			# ar_SM is an abbreviation for ar_scaleMatrix and returns an arMatrix4 with appropriate uniform scaling.
			# Set the scale node with this.
			s.set(ar_SM(0.5))
			# Creates a new cube object.
			sph = arCubeMesh()
			# Attach the sphere geometry in the scene graph below the node s.
			# NOTE: conceptually, the sphere object is like a "factory". The geometry created in the scene
			# graph does NOT refer to anything held in the mesh object.
			sph.attachMesh(s)
		elif self.type == 'cylinder':
			# ar_SM is an abbreviation for ar_scaleMatrix and returns an arMatrix4 with appropriate uniform scaling.
			# Set the scale node with this.
			s.set(ar_SM(0.5))
			sph = arCylinderMesh()
			# When the geometry is attached to the scene graph, the points and normals will get modified by this
			# transformation. In this case, we want to squash the cylinder a little bit so it is long and skinny
			# (the cylinder's axis is along z).
			sph.setTransform(ar_SM(0.3, 0.3, 1))
			# Configure the cylinder. We divide the radius into 20 divisions for polygonalization. Also, the front
			# and the back radius are set at 1 (the default). NOTE: setting one of these to 0 would create a cone!
			sph.setAttributes(20,1,1)
			# Put caps on the cylinder's ends (using 0 would mean no caps).
			sph.toggleEnds(1)
			# Attach the sphere geometry in the scene graph below the node s.
			# NOTE: conceptually, the sphere object is like a "factory". The geometry created in the scene
			# graph does NOT refer to anything held in the mesh object.
			sph.attachMesh(s)
		# Create the scene graph nodes for a tail if requested. The tail will get its color from its ancestor
		# material node (see m above). The tail will be the same color as the particle's geometry (both have
		# the same ancestor m).
		if self.useTail:
			# Create a child node of m to store the coordinates of the points defining the tail.
			points = m.new("points")
			# Save the node, since we'll be modifying it later.
			self.pointsNode = points
			# Create a state node so we can alter the line width of the tail from the default 1.0.
			state = points.new("graphics state")
			state.set(("line_width",1+4*random.random()))
			# Put the node that actually draws the tail as a child of the state node.
			draw = state.new("drawable")
			self.drawableNode = draw
			# The points will be consumed as in GL_LINES (i.e. "lines"). There are self.tailLength line segments
			# (and twice that many points).
			draw.set(("lines",self.tailLength))

	# Resets the position, velocity, and shape of the particle. Called when the particle's lifetime has expired.	
	def reset(self):
		# The origin of the particle fountain.
		self.x = arVector3(0,self.floor,0)
		# What's the velocity direction in the xz plane?
		angle = random.random()*6.283
		# Random speed in the given direction.
		self.v = arVector3(cos(angle),1+4*random.random(),sin(angle))
		# Starting over on number of bounces.
		self.numberBounces = 0
		# Reattach the particle (potentially giving it a new shape and color).
		self.attach(self.parent)

	# Advances the particle position according to an ODE. To avoid taking too large a time step (and compromising stability
	# of the numerics), the code breaks up large user-requested time steps into more manageable chunks (the object defines
	# a maxDelta).
	def adv(self, delta):
		# self.oldX is set to the self.x object. These are now essentially pointers to the same object.
		# Later, we will want to use (self.x, self.oldX) as a new line segment for the tail, which would be
		# pointless if they were the same. However, self.x is set to a (new) object in the math below, so this is OK.
		# See lorenz.py for a related potential surprise!
		self.oldX = self.x
		# Change the object's rotation by the time step.
		self.rotationAngle += delta * self.rotationSpeed
		iters = -1
		# Remember, we must break up the user-requested time step into pieces of size at most maxDelta 
		# (with the last chunk being the remainder (rmdr) ).
		if delta > self.maxDelta:
			iters = floor(delta/self.maxDelta)
			rmdr = delta - iters*self.maxDelta
		if iters < 1:
			# Standard naive ODE time step advance.
			self.x = self.x + self.v * delta
			self.v = self.v + self.a * delta
		else:
			for i in range(iters+1):
				# On the first iteration, we do the remainder. Subsequently, maxDelta. 
				# The total time step will thus equal the requested delta.
				if i == 1:
					rmdr = self.maxDelta
				# Standard naive ODE time step advance.
				self.x = self.x + self.v * rmdr
				self.v = self.v + self.a * rmdr
		# Check if the particle has collided with the floor. If so, relaunch.
		self.resetFlag = 0
		if self.boundary():
			self.reset()
			self.resetFlag = 1
		# If we're using the tail, go ahead and update.
		if self.useTail:
			# Do not change a line segment if the particle has been reset. This would create
			# a line from the last hit to the origin.
			if self.pointsNode and not self.resetFlag:
				# Alters the self.tailPosition'th line segment by changing its endpoints in
				# the points node.
				self.pointsNode.set((self.x, self.oldX), (2*self.tailPosition, 2*self.tailPosition+1))
			self.tailPosition  += 1
			if self.tailPosition >= self.tailLength:
				self.tailPosition = 0
		# Push the new particle position into the scene graph. See method definition below.
		self.post()

	# Check to see if the particle has hit the floor. If so, reduce its speed and determine if it should be reset
	# (if it is now too slow or if it has bounced too many times).
	def boundary(self):
		if self.x[1] < self.floor:
			self.x[1] = self.floor
			# Reflect the vector around the floor's normal.
			tmp = ar_reflect(self.v, arVector3(0,1,0))
			# Reduce the speed.
			self.v = arVector3(self.fric*tmp[0], self.fric*tmp[1], self.fric*tmp[2])
			# Increment number of bounces.
			self.numberBounces = self.numberBounces + 1
			# If the particle should be reset, return 1.
			if self.v.magnitude() < self.speedCutoff or self.numberBounces >= self.maxBounces:
				return 1
		return 0

	# Push particle position and rotation into the scene graph.
	def post(self):
		if self.transformNode != None:
			self.transformNode.set(ar_TM(self.x))
		if self.rotationNode != None:
			self.rotationNode.set(ar_RM('y',self.rotationAngle))
		

# Our scene is lit. Consequently, there must be lights! Otherwise everything would be black!
def addLights(g):
	# The lights should be attached to the scene graph's "root". This is the tree node from which all other
	# nodes descend.
	r = g.getRoot()
	# Creates a new light node that is a child of the scene graph root.
	l = r.new("light")
	# Creates a light object. This will later be passed into the light node.
	light = arLight()
	# Each light needs a unique ID (0-7). If you have another light with the same ID, the light that was
	# defined first will be ignored.
	light.lightID = 0
	# Since the last position is 0, this is an OpenGL directional light. If the last position were 1,
	# it would be a positional light.
	light.position = arVector4(0,0,-1,0)
	# No ambient component to the light.
        light.ambient = arVector3(0,0,0)
	# Relatively dim white light (this is the directional component).
        light.diffuse = arVector3(0.5, 0.5, 0.5)
	# Pass the light object into the light node.
	l.setLight(light)
	# Creates a new light object that is a child of the scene graph root.
	l = r.new("light")
	light = arLight()
	# Each light needs a unique ID (0-7).
	light.lightID = 1
	light.position = arVector4(0,0,1,0)
        light.ambient = arVector3(0,0,0)
        light.diffuse = arVector3(0.5, 0.5, 0.5)
	# Set the light node with our defined light.
	l.setLight(light)

##############################################################################
#                           Main Program                                     #
##############################################################################

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

# Argument parsing. The following are valid invocations.
#  python particle-shape.py
#    10 particles at once, no tails.
#  python particle-shape.py 100
#    100 particles at once, no tails.
#  python particle-shape.py 200 1
#    200 paricles at once, tails are present.
if len(sys.argv) > 1:
	number = int(sys.argv[1])
else:
	number = 10
if len(sys.argv) > 2 and int(sys.argv[2]) > 0:
	useTail = 1
else:
	useTail = 0

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
# Create a child of the nav node to place our whole scene.
w = r.new("transform")
# ar_TM is an abbreviation for ar_translationMatrix. It returns an arMatrix4 representing the given translation.
# The default camera for Syzygy programs shows the front CAVE wall (center is (0,5,-5)). We choose w's transform
# with this in mind.
w.set(ar_TM(0,2,-5))

# The acceleration due to gravity in our simulation.
grav = arVector3(0,-1,0)
# Make a list of the particles.
plist = []
for i in range(number):
	p = particle()
	# Will our particle have a tail?
	p.useTail = useTail
	# The particle will be attached to the scene graph below the w node. Since this is below the framework's
	# navigation node, it will be affected by user navigation.
	p.parent = w
	# Attaches the particle to the scene graph.
	p.reset()
	# Set the acceleration of the particle (gravity is the only force).
	p.a = grav
	plist.append(p)

# Starts all the services, windows, etc. NOTE: it makes the most sense to do
# this AFTER lengthy initializations of the scene graph (though you can issue
# this statement any time after initializing the framework).
if f.start() != 1:
	sys.exit()

now = time.clock()
change = 0
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
	for p in plist:
		p.adv(change)
	temp = time.clock()
	change = temp-now
	now = temp
	# We have chosen the manual swap mode of operation for the scene graph framework.
	# Consequently, we must issue the buffer swap command ourselves.
	f.swapBuffers()
