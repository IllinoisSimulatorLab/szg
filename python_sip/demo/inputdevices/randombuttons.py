import sys
import time
import thread
import math
import random
import szg


class MyFilter( szg.arIOFilter ):
  def onInputEvent( self, event ):
    if event.getType() == szg.AR_EVENT_BUTTON:
      print 'Before filter:',event
      if event.getButton() == 1:
        self.lastIndex = random.choice( range(8) )
        event.setIndex( self.lastIndex )
      else:
        event.setIndex( self.lastIndex )
      print 'After  filter:',event
      print '------------------------------------------'
    return True


class MyDriverFramework( szg.arPyDeviceServerFramework ):
  def configureInputNode(self):
    self.driver = szg.arGenericDriver()
    self.driver.setSignature( 8,0,2 )
    self.getInputNode().addInputSourceMine( self.driver )
    self.filter = MyFilter()
    self.getInputNode().addFilterMine( self.filter )
    return self.addNetInput()
  def start(self):
    thread.start_new_thread( self.driverFunc, () )
  def driverFunc(self):
    startTime = time.clock()
    while True:
      time.sleep( 1. )
      buttonIndex = random.choice( range(8) )
      self.driver.sendButton( buttonIndex, 1 )
      time.sleep( .1 )
      self.driver.queueButton( buttonIndex, 0 )
      self.driver.queueMatrix( 0, szg.ar_translationMatrix(0,5.5,0) )
      self.driver.queueMatrix( 1, szg.ar_translationMatrix(1,3.5,-1) )
      self.driver.sendQueue()



if __name__=='__main__':
  app = MyDriverFramework()
  if not app.init( sys.argv ):
    print sys.argv[0], "failed to init framework."
    app.getSZGClient().failStandalone(False)
    sys.exit(1)
  app.start()
  app.messageLoop()
