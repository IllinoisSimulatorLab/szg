from PySZG import * 
import random
import time

useRemotePeer = 0

f = arDistSceneGraphFramework();
if f.init(sys.argv) != 1:
	sys.exit()
if len(sys.argv) < 2:
	print("First argument must be local peer name")
	sys.exit()
if len(sys.argv) >= 3:
	print("Will connect to remote peer, which must already be running.")
	useRemotePeer = 1
if f.getStandalone():
	print("This program only works in cluster mode. Please log in.")
	sys.exit()
peer = arGraphicsPeer()
peer.setName(sys.argv[1])
peer.queueData(0)
peer.initSZG(f.getSZGClient())
peer.setTexturePath(f.getSZGClient().getAttribute("SZG_RENDER","texture_path"))
peer.loadAlphabet(f.getSZGClient().getAttribute("SZG_RENDER","text_path"))
peer.setBridge(f.getDatabase())
peer.setBridgeRootMapNode(f.getNavNode())

if peer.start() != 1:
	sys.exit()
if f.start() != 1:
	sys.exit()

if useRemotePeer:
	if peer.connectToPeer(sys.argv[2]) < 0:
		print("Failed to connect to specified peer.")
		sys.exit()
	else:
		print("Downloading from peer.")
		if peer.pullSerial(sys.argv[2], 0, 0, AR_TRANSIENT_NODE, AR_TRANSIENT_NODE, AR_TRANSIENT_NODE) == 0:
			print("Error in downloading")
else:
	n = peer.getRoot()
	for i in range(100):
		world = n.new("transform", "world"+str(i))

worldRoot = peer.find(sys.argv[1])
if worldRoot == None:
	print("Invalid world name. Must be worldN, where n=0-99.")
	peer.stop()
	sys.exit()
l = worldRoot.new("light")
light = arLight()
light.position = arVector4(0,0,1,0)
l.set(light)	
t = worldRoot.new("transform")
t.set(ar_TM(-3 + random.random()*6, 5, -5))
m = t.new("material")
s = arSphereMesh(50)
s.attachMesh(m)
while 1:
	f.setViewer()
	f.setPlayer()
	f.navUpdate()
	mat = arMaterial();
	mat.diffuse = arVector3(random.random(), random.random(), random.random())
	m.set(mat)
	f.loadNavMatrix()
	time.sleep(0.01)
