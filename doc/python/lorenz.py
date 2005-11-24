from PySZG import *
import sys
import time
import random

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

def advance(t):
	dt = 0.004
	v = ar_ET(t.get())
        v[0] = (10*v[1]-10*v[0])*dt + v[0]
	v[1] = (28*v[0]-v[1] - v[0]*v[2])*dt + v[1]
	v[2] = (-(2.66667)*v[2] + v[0]*v[1])*dt + v[2]
	t.set(ar_TM(v))
	

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
m = gcast(r.new("material"))
mat = arMaterial()
mat.diffuse = arVector3(1,0,0)
mat.specular = arVector3(0.2, 0.2, 0.2)
mat.emissive = arVector3(0,0,0.1)
mat.exponent = 100
m.set(mat)
w = m.new("transform")
w.set(ar_TM(0,5,-5)*ar_SM(0.1))
t = w.new("transform","sphere_trans")
t.set(ar_TM(8, 8, 23))
s = t.new("transform","sphere_scl")
s.set(ar_SM(2))
s = arSphereMesh()
s.setAttributes(15)
s.attachMesh("sphere","sphere_scl")

while 1:
	f.setViewer()
	f.navUpdate()
        f.loadNavMatrix()
	advance(t)
	f.swapBuffers()
