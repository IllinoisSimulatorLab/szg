########################################################################################
# Demonstrates how to use the Myriad scene graph for grabber-based user interaction.
# A collection of objects is randomly dispersed throughout space.
# Here, we have mixed default user navigation with custom object manipulation code.
########################################################################################

from PySZG import *
import sys
import time
import random

########################################################################################
# Exercises:
#  1. Can you use a bounding sphere hierarchy to have faster drawing and intersection
#     testing?
########################################################################################

#########################################
#          Global variables             #
#########################################

widgetList = []

class tool:
	def __init__(self):
		self.transform = None
		self.length = 5
		self.touching = 0
	def attach(self, transformNode):
		self.transform = transformNode
		c = arCubeMesh()	
		c.setTransform(ar_TM(0,0,-self.length/2.0)*ar_SM(0.5, 0.5, self.length))
		c.attachMesh(self.transform)
	def showTouched(self, l):
		for n in l:
			m = n.findByType("material")
			if m:
				mat = m.get()
				mat.diffuse = arVector3(0,0,1)
				m.set(mat)
			
	def update(self, framework, event):
		if event.getType() == AR_EVENT_MATRIX and event.getIndex() == 1:
			self.transform.set(event.getMatrix())
			if self.touching:
				b = arBoundingSphere()
				b.radius = 0.5
				b.position = self.transform.get()*arVector3(0,0,-self.length)
				l = framework.getDatabase().intersectSphere(b)
				self.showTouched(l)
		if event.getType() == AR_EVENT_BUTTON and event.getIndex() == 0:
			self.touching = event.getButton()
			

def addLights(r):
	l = r.new("light")
	light = arLight()
	light.lightID = 0
	light.position = arVector4(0,0,-1,0)
        light.ambient = arVector3(0,0,0)
        light.diffuse = arVector3(0.5, 0.5, 0.5)
	l.set(light)
	l = r.new("light")
	light = arLight()
	light.lightID = 1
	light.position = arVector4(0,0,1,0)
        light.ambient = arVector3(0,0,0)
        light.diffuse = arVector3(0.5, 0.5, 0.5)
	l.set(light)

# The event processing callback. All it does is grab the events that have queued
# since last call and send them to the items on the widget list. A very generic function.
# NOTE: callbacks of this type must return 0/1 (i.e. bool on the C++ side).
def eventProcessing(framework, eventQueue):
	while not eventQueue.empty():
		e = eventQueue.popNextEvent()
		for w in widgetList:
			w.update(framework, e)
	return 1

f = arDistSceneGraphFramework()
# We want to do event-based interaction for the tool. See eventProcessing(...) function above.
f.setEventQueueCallback(eventProcessing)

if f.init(sys.argv) != 1:
	sys.exit()
if f.start() != 1:
	sys.exit()

f.setNavTransSpeed(0.2)
g = f.getDatabase()
root = g.getRoot()
addLights(root)
toolTransform = root.new("transform")
wand = tool()
wand.attach(toolTransform)
widgetList.append(wand)
navNode = f.getNavNode()
tlist = []
for i in range(8):
	for j in range(8):
		t = navNode.new("transform")
		t.set(ar_TM(-3.5+i, -3.5+j + 5, -4))
		tlist.append(t)
		scl = t.new("transform")
		scl.set(ar_SM(random.random()))
		bound = scl.new("bounding sphere")
		b = arBoundingSphere()
		b.position = arVector3(0,0,0)
		b.radius = 0.5
		bound.set(b)
		m = bound.new("material")
		mat = arMaterial()
		mat.diffuse = arVector3(1,0,0)
		mat.specular = arVector3(0.2, 0.2, 0.2)
		mat.emissive = arVector3(0,0,0.1)
		mat.exponent = 100
		m.set(mat)
		s = arSphereMesh()
		s.setAttributes(15)
		s.attachMesh(m)

while 1:
	f.processEventQueue()
	f.setViewer()
	f.navUpdate()
        f.loadNavMatrix()
	time.sleep(0.02)
