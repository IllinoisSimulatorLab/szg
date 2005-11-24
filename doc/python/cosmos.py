from PySZG import *
import sys
import time
import random

trans1 = None
trans2 = None
trans3 = None
trans4 = None
points = None
viz = None
billboardTrans = None
percentage = 1.0

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

def attachLineSet(t):
	global points
	plist = []
	for i in range(150):
		p = arVector3(-5 + 10 * random.random(),
                              -5 + 10 * random.random(),
                              -5 + 10 * random.random())
		if p.magnitude() == 0:
			p = arVector3(0,0,1)
		p.normalize()
		plist.append(p)
		plist.append(arVector3(0,0,0))
	points = t.new("points")
	points.set(plist)
	clist = []
	for i in range(150):
		c = arVector4(random.random(),
                              random.random(),
                              random.random(),
                              1)
		clist.append(c)
		clist.append(arVector4(0,0,0,0))
	colors = points.new("color4")
	colors.set(clist)
	draw = colors.new("drawable")
	draw.set(("lines",150))

def lineChange(percentage):
	plist = points.get()
	for i in range(150):
		plist[2*i+1] = plist[2*i]*percentage
	points.set(plist)

def worldInit(t):
	global trans1
	global trans2
	global trans3
	global trans4
	global viz
	global billboardTrans
 	trans1 = t.new("transform")
 	tex = trans1.new("texture")
 	tex.set("WallTexture1.ppm")
 	m = tex.new("material")
 	mat = arMaterial()
 	mat.diffuse = arVector3(1,0.6,0.6)
 	m.set(mat)
  	torus = arTorusMesh(60,30,4,0.5)
	torus.attachMesh(m)

	trans2 = t.new("transform")
 	tex = trans2.new("texture")
 	tex.set("WallTexture2.ppm")
  	torus.reset(60,30,2,0.5)
	torus.attachMesh(tex)

	trans3 = t.new("transform")
 	tex = trans3.new("texture")
 	tex.set("WallTexture3.ppm")
  	torus.reset(60,30,1,0.5)
	torus.attachMesh(tex)

	trans4 = t.new("transform")
 	tex = trans4.new("texture")
 	tex.set("WallTexture4.ppm")
  	torus.reset(60,30,3,0.5)
	torus.attachMesh(tex)

	viz = trans1.new("visibility")
	viz.set(1)
	billboardTrans = viz.new("transform")
	billboardTrans.set(ar_TM(4.55,0,0)*ar_RM('x', -1.571)*ar_RM('y', -1.571)*ar_SM(0.1))
	billboard = billboardTrans.new("billboard")
	billboard.set(" myriad scene graph ")

def worldAlter(elapsedTime):
	global trans1
	global trans2
	global trans3
	global trans4
	global percentage
        trans1.set(ar_RM('x', elapsedTime *  1.6))
	trans2.set(ar_RM('y', elapsedTime *  3.1))
	trans3.set(ar_RM('z', elapsedTime *  2.1))
	trans4.set(ar_RM('z', elapsedTime * -5.5))
	billboardTrans.set(ar_RM('z', elapsedTime*0.02)*billboardTrans.get())
	percentage -= 0.05
	if percentage < 0:
		percentage = 1
	lineChange(percentage)
	
f = arDistSceneGraphFramework()
if f.init(sys.argv) != 1:
	sys.exit()
f.setAutoBufferSwap(0)
if f.start() != 1:
	sys.exit()

f.setNavTransSpeed(0.2)
g = f.getDatabase()
addLights(g)
r = f.getNavNode()
worldTrans = gcast(r.new("transform"))
worldTrans.set(ar_TM(0,5,-5))
worldInit(worldTrans)
attachLineSet(worldTrans)
counter = 0
while 1:
	f.setViewer()
	f.navUpdate()
        f.loadNavMatrix()
	worldAlter(counter)
	counter += 0.02
	f.swapBuffers()
