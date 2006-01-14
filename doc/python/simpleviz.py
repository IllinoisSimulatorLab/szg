########################################################################################
# The absolute minimum scene graph app. No navigation, but head movement is supported.
########################################################################################

############################################################################################
# Exercise: Can you move the sphere? Give it a color or texture? Can you have more of them?
############################################################################################

from PySZG import *
import time

f = arDistSceneGraphFramework()
if f.init(sys.argv) != 1:
	sys.exit()
g = f.getDatabase()
root = g.getRoot()
l = root.new("light")
light = arLight()
light.position = arVector4(0,0,1,0)
l.set(light)	
s = arSphereMesh(30)
s.setTransform(ar_TM(0,5,-5)*ar_SM(3))
s.attachMesh(l)
if f.start() != 1:
	sys.exit()
while 1:
	f.setViewer()
	time.sleep(0.02)
