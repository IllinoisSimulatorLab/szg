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

f = arDistSceneGraphFramework()
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
tlist = []
for i in range(8):
	for j in range(8):
		t = m.new("transform","sphere_trans"+str(i)+str(j))
		tlist.append(t)
		t.set(ar_TM(-3.5+i, -3.5+j + 5, -4))
		s = t.new("transform","sphere_scl"+str(i)+str(j))
		s.set(ar_SM(random.random()))
		s = arSphereMesh()
		s.setAttributes(15)
		s.attachMesh("sphere"+str(i)+str(j),"sphere_scl"+str(i)+str(j))
print(len(tlist))
while 1:
	f.setViewer()
	f.navUpdate()
        f.loadNavMatrix()
	time.sleep(0.02)
