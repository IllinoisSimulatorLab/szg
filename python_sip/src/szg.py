print 'szg module:',__file__
from _szg import *
print 'Syzygy version:',ar_versionString()
import sys
import traceback

# Simple embedded multiline python interpreter built around raw_input().
# Interrupts the control flow at any given location with 'exec prompt'
# and gives control to the user.
# Allways runs in the current scope and can even be started from the 
# pdb prompt in debugging mode. Tested with python, jython and stackless.
# Handy for simple debugging purposes.

import thread

__szg_python_prompt_lock = thread.allocate_lock()

__szg_python_prompt = compile("""
try:
    _prompt
    _recursion = 1
except:
    _recursion = 0
if not _recursion:
    try:
      szgPromptLock = __szg_python_prompt_lock
    except NameError:
      try:
        szgPromptLock = szgprompt.__szg_python_prompt_lock
      except NameError, AttributeError:
        raise RuntimeError, 'Failed to get a reference to the Syzygy Python prompt lock.'
    from traceback import print_exc as print_exc
    from traceback import extract_stack
    _prompt = {'print_exc':print_exc, 'inp':'','inp2':'','co':''}
    _a_es, _b_es, _c_es, _d_es = extract_stack()[-2]
    if _c_es == '?':
        _c_es = '__main__'
    else:
        _c_es += '()' 
    print '\\nprompt in %s at %s:%s  -  continue with CTRL-D' % (_c_es, _a_es, _b_es)
    del _a_es, _b_es, _c_es, _d_es, _recursion, extract_stack, print_exc
    while 1:
        try:
            _prompt['inp']=raw_input('>>> ')
            if not _prompt['inp']:
                continue
            if _prompt['inp'][-1] == chr(4): 
                break
            szgPromptLock.acquire()
            exec compile(_prompt['inp'],'<prompt>','single')
            szgPromptLock.release()
        except EOFError:
            print
            szgPromptLock.release()
            break
        except SyntaxError:
            szgPromptLock.release()
            while 1:
                _prompt['inp']+=chr(10)
                try:
                    _prompt['inp2']=raw_input('... ')
                    if _prompt['inp2']:
                        if _prompt['inp2'][-1] == chr(4): 
                            print
                            break
                        _prompt['inp']=_prompt['inp']+_prompt['inp2']
                    _prompt['co']=compile(_prompt['inp'],'<prompt>','exec')
                    if not _prompt['inp2']: 
                        szgPromptLock.acquire()
                        exec _prompt['co']
                        szgPromptLock.release()
                        break
                    continue
                except EOFError:
                    print
                    break
                except:
                    if _prompt['inp2']: 
                        continue
                    _prompt['print_exc']()
                    break
        except:
            _prompt['print_exc']()
            szgPromptLock.release()
    print '--- continue ----'
    # delete the prompts stuff at the end
    del _prompt
""", '<prompt>', 'exec')



# note 'self' here refers to the framework.
def __szg_promptThread( self ):
  exec __szg_python_prompt


def ar_initPythonPrompt( framework ):
  print 'Starting up interactive Python prompt...'
  thread.start_new_thread( __szg_promptThread, (framework,) )
  __szg_python_prompt_lock.acquire()


def ar_doPythonPrompt():
  __szg_python_prompt_lock.release()
  __szg_python_prompt_lock.acquire()



class arPyMasterSlaveFramework(arMasterSlaveFramework):
  unitsPerFoot = 1.   # default to feet
  nearClipDistance = 1.
  farClipDistance = 200.
  def __init__(self):
    arMasterSlaveFramework.__init__(self)
    self.__usePrompt = '--prompt' in sys.argv
    # Tell the framework what units we're using.
    self.setUnitConversion( self.unitsPerFoot )
    # Near & far clipping planes.
    self.setClipPlanes( self.nearClipDistance, self.farClipDistance )
  def onStart( self, client ):
    if self.__usePrompt:
      if self.getMaster():
        ar_initPythonPrompt( self )
    return True
  def _onWindowStartGL( self, winInfo ):
    try:
      self.onWindowStartGL( winInfo )
    except Exception, msg:
      traceback.print_exc()
      self._stop( 'WindowStartGL', arCallbackException(str(msg)) )
      self.stop( False )
  def onWindowStartGL( self, winInfo ):
    pass
  def _onPreExchange( self ):
    try:
      if self.__usePrompt:
        ar_doPythonPrompt()
      self.onPreExchange()
    except Exception, msg:
      traceback.print_exc()
      self._stop( 'PreExchange', arCallbackException(str(msg)) )
      self.stop( False )
  def onPreExchange( self ):
    pass
  def _onPostExchange( self ):
    try:
      self.onPostExchange()
    except Exception, msg:
      traceback.print_exc()
      self._stop( 'PostExchange', arCallbackException(str(msg)) )
      self.stop( False )
  def onPostExchange( self ):
    pass
  def _onWindowInit( self ):
    try:
      self.onWindowInit()
    except Exception, msg:
      traceback.print_exc()
      self._stop( 'WindowInit', arCallbackException(str(msg)) )
      self.stop( False )
  def onWindowInit( self ):
    ar_defaultWindowInitCallback()
  def _onDraw( self, win, vp ):
    try:
      self.onDraw( win, vp )
    except Exception, msg:
      traceback.print_exc()
      self._stop( 'Draw', arCallbackException(str(msg)) )
      self.stop( False )
  def onDraw( self, win, vp ):
    pass
  def _onDisconnectDraw( self ):
    try:
      self.onDisconnectDraw()
    except Exception, msg:
      traceback.print_exc()
      self._stop( 'DisconnectDraw', arCallbackException(str(msg)) )
      self.stop( False )
  def onDisconnectDraw( self ):
    # just draw a black background
    ar_defaultDisconnectDraw()
  def _onPlay( self ):
    try:
      self.onPlay()
    except Exception, msg:
      traceback.print_exc()
      self._stop( 'Play', arCallbackException(str(msg)) )
      self.stop( False )
  def onPlay( self ):
    self.setPlayTransform()
  def _onWindowEvent( self, winInfo ):
    try:
      self.onWindowEvent( winInfo )
    except Exception, msg:
      traceback.print_exc()
      self._stop( 'WindowEvent', arCallbackException(str(msg)) )
      self.stop( False )
  def onWindowEvent( self, winInfo ):
    state = winInfo.getState()
    if state == AR_WINDOW_RESIZE:
      winInfo.getWindowManager().setWindowViewport( winInfo.getWindowID(), \
        0, 0, winInfo.getSizeX(), winInfo.getSizeY() )
    elif state == AR_WINDOW_CLOSE:
    # We will only get here if someone clicks the window close decoration.
    # This is NOT reached if we use the arGUIWindowManagers delete  method.
      self.stop( False )
  def _onCleanup( self ):
    try:
      self.onCleanup()
    except Exception, msg:
      traceback.print_exc()
      self._stop( 'Cleanup', arCallbackException(str(msg)) )
      self.stop( False )
  def onCleanup( self ):
    pass
  def _onOverlay( self ):
    try:
      self.onOverlay()
    except Exception, msg:
      traceback.print_exc()
      self._stop( 'Overlay', arCallbackException(str(msg)) )
      self.stop( False )
  def onOverlay( self ):
    pass
  def _onKey( self, keyInfo ):
    try:
      self.onKey( keyInfo )
    except Exception, msg:
      traceback.print_exc()
      self._stop( 'Key', arCallbackException(str(msg)) )
      self.stop( False )
  def onKey( self, keyInfo ):
    if keyInfo.getState() == AR_KEY_DOWN:
      self.onKeyDown( keyInfo.getKey(), keyInfo.getCtrl(), keyInfo.getAlt() )
    elif keyInfo.getState() == AR_KEY_UP:
      self.onKeyUp( keyInfo.getKey(), keyInfo.getCtrl(), keyInfo.getAlt() )
  def onKeyDown( self, key, ctrl, alt ):
    pass
  def onKeyUp( self, key, ctrl, alt ):
    pass
  # master->slave data-transfer based on cPickle module
  def initObjectTransfer(self,name): self.initStringTransfer(name)
  def setObject(self,name,obj): self.setString(name,cPickle.dumps(obj))
  def getObject(self,name): return cPickle.loads(self.getString(name))
  # master->slave data-transfer based on struct module
  def initStructTransfer( self, name ):
    self.initStringTransfer( name+'_FORMAT' )
    self.initStringTransfer( name+'_DATA' )
  def setStruct( self, name, format, obj ):
    self.setString( name+'_FORMAT', format )
    tmp = struct.pack( format, *obj )
    self.setString( name+'_DATA', tmp )
  def getStruct( self, name ):
    format = self.getString( name+'_FORMAT' )
    data = self.getString( name+'_DATA' )
    return struct.unpack( format, data )


# Utility classes

import UserDict

class arMasterSlaveDict(UserDict.IterableUserDict):
  """
  arMasterSlaveDict: An IterableUserDict subclass designed for use in Syzygy Master/slave apps.
  Provides an easy method for synchronizing its contents between master and slaves. Also
  supports easy user interaction. Most of its methods are intended for use only in the
  onPreExchange() method in the master copy of the application; in the slaves, only
  unpackState() should be called in onPostExchange() and draw() in onDraw().
  It supports most of the usual methods for insertion, deletion, and extraction, including
  d[key], d[key] = value, del d[key], and d.clear(). Note that keys MUST be Ints, Floats,
  or Strings. It also has a delValue() method for deleting objects by reference. It does
  not support d.copy() or d.popitem(). It supports iteration across either keys, values,
  or items (key/value tuples).
  Additional docs are available for:
  __init__()
  start()
  packState()
  unpackState()
  delValue()
  push()
  draw()
  processInteraction()
  """

  keyTypes = [type(''),type(0),type(0.)]
  def __init__( self, name, classData=[] ):
    """
    d = arMasterSlaveDict( name, classData=[] )
    'name' should be a string. This is used by the master/slave framework sequence data transfer
    methods, e.g. framework.initSequenceTransfer (called in the this class' start() method).

    'classData' contains information about the classes of objects that will be inserted into the
    arMasterSlaveDict: First, whenever you try to insert an object into the arMasterSlaveDict,
    it is checked against the set of classes that you have provided; if not present, TypeError
    is raised. Second, if you insert an object into the dictionary in the master instance of
    your application, it is used to construct an instance of the same class in the slaves.
    You have to provide an entry for each class of object that you intent to insert
    in this container.  classData can be either a list or a tuple. Each item must be either
    (1) an unambiguous reference to the class itself, or (2) a tuple containing two items,
    the class reference and a callable class factory. The class factory should be a reference to a
    callable object that takes no parameters and returns an object of the specified class
    If the class factory item is not presented, then the class itself will
    be used as the factory; it will be called without arguments (again, see below), so the class
    must provide a zero-argument constructor (i.e. __init__(self)). Take three examples:

    1) You've defined class Foo, either in the current file or in a module Bar that youve imported
    using 'from Bar import *'. Now  the class object Foo is in the global namespace. So you could call
    self.dict = arMasterSlaveDict( 'mydict', [Foo] ).
    In this case, inserted objects are type-checked against Foo and the class itself is
    used as the factory, i.e. Foo() is called to generate new instances in the slaves.

    2) You've defined class Foo in module Bar and imported it using 'import Bar'. Now Foo is not in the
    global namespace, it's in the Bar namespace. So you would call
    self.dict = arMasterSlaveDict( 'mydict', [Bar.Foo] ).
    Now, objects you insert are type-checked agains Bar.Foo and Bar.Foo() is called to create new
    instances in the slaves.

    3) You want to pass a parameter to the constructor of Foo, e.g. a reference to the framework object.
    So you define a framework method newFoo():

    def newFoo(self): return Foo(self)

    and call self.dict = arMasterSlaveDict( 'mydict', [(Foo,self.newFoo)] ).
    Now inserted objects are type-checked against Foo and framework.newFoo() is called to generate
    new instances in slaves to match instances inserted in the master.
    """
    UserDict.IterableUserDict.__init__(self)
    self._name = name
    self._classFactoryDict = {}
    self.addTypes( classData )
    self.pushKey = 0
    self.__started = False
  def addTypes( self, classData ):
    import types
    if type(classData) != types.TupleType and type(classData) != types.ListType:
      raise TypeError, 'arMasterSlaveDict() error: classData parameter must be a list or tuple.'
    for item in classData:
      if type(item) == types.TupleType:
        if len(item) != 2:
          raise TypeError, 'arMasterSlaveDict() error: if a tuple, each item of classData must contain 2 elements.'
        classRef = item[0]
        classFactory = item[1]
      else:
        classRef = item
        classFactory = classRef
      classKey = self.composeKey( classRef )
      if not callable( classFactory ):
        raise TypeError, 'arMasterSlaveDict(): invalid class constructor or factory.'
      self._classFactoryDict[classKey] = classFactory
  def start( self, framework ):  # call in framework onStart() method or start callback
    """ d.start( framework ).
    Should be called in your framework's onStart() (start callback)."""
    if not self.__started:
      framework.initSequenceTransfer( self._name )
    self.__started = True
  def packState( self, framework ):
    """ d.packState( framework ).
    Should be called in your framework's onPreExchange(). It iterates through the dictionary's contents, calls each
    item's getState() method (which should return a sequence type containing only Ints, Floats, Strings,
    and nested sequences) and adding a state message with each returned tuple to its message
    queue. Besides the object's state, this method contains its class and its dictionary key.
    Needless to say, all classes must provide a getState(). Finally, it calls
    framework.setSequence() to hand its message queue to the framework.
    """
    messages = []
    if len(self.data) > 0:
      for key in self.data:
        item = self.data[key]
        itemState = item.getState()
        itemClass = self.composeKey( item.__class__ )
        messages.append( (key, itemClass, item.getState()) )
    framework.setSequence( self._name, messages )
  def unpackState( self, framework ):
    """ d.unpackState( framework ).
    Should be called in your framework's onPostExchange(), optionally only in slaves. Calls
    framework.getSequence to get the message queue from the master (see doc string for packState()),
    For each message in the queue, it checks whether an object with the appropriate key value already exists;
    if not, it calls the appropriate class constructor or factory to create it. Then it set its state using
    the objects setState() method. This method should expect a tuple
    with the same structure as that returned by getState(). For example, if your class' getState() method
    returned a 3-element numarray array, then setState() should expect a 3-element tuple. Finally, any objects
    with keys _not_ referenced in the message queue are removed (because they have presumably been deleted
    from the master).
    """
    keysToDelete = self.data.keys()
    messages = framework.getSequence( self._name )
    for message in messages:
      key = message[0]
      classKey = message[1]
      newState = message[2]
      if not self.data.has_key( key ):
        classFactory = self._classFactoryDict[classKey]
        self.data[key] = classFactory()
        keysToDelete.append(key)
      elif self.composeKey( self.data[key].__class__ ) != classKey:
        del self.data[key]
        classFactory = self._classFactoryDict[classKey]
        self.data[key] = classFactory()
      self.data[key].setState( newState )
      keysToDelete.remove( key )
    for key in keysToDelete:
      del self.data[key]
  def composeKey( self, cls ):
    return cls.__module__+'.'+cls.__name__
  def __setitem__(self, key, newItem):
    if not type(key) in arMasterSlaveDict.keyTypes:
      raise KeyError, 'arMasterSlaveDict keys must be Ints, Floats, or Strings.'
    newKey = self.composeKey( newItem.__class__ )
    if not self._classFactoryDict.has_key( newKey ):
      raise TypeError, 'arMasterSlaveDict error: non-registered type '+str(newKey)+' in __setitem__().'
    self.data[key] = newItem
  def __delitem__( self, key ):
    del self.data[key]
  def delValue( self, value ):
    """ d.delValue( contained object reference ).
    Delete an item from the dictionary by reference, i.e. if you have an external reference to
    an item in the dictionary, call delValue() with that reference to delete it."""
    for item in self.iteritems():
      if item[1] is value:
        del self[item[0]]
        return
    raise LookupError, 'arMasterSlaveDict.delValue() error: item not found.'
  def push( self, object ):
    """ d.push( object ).
    arMasterSlaveDict maintains an internal integer key for use with this method. Each time you call it,
    it checks to see whether that key already exists. If so, it increments it in a loop until it finds an
    unused key. Then it inserts the object using the key and increments it again. """
    while self.data.has_key( self.pushKey ):
      self.pushKey += 1
    self[self.pushKey] = object
    key = self.pushKey
    self.pushKey += 1
    return key
  def clear(self):
    self.data.clear()
  def copy(self):
    raise AttributeError, 'arMasterSlaveDict does not allow copying of itself.'
  def popitem(self):
    raise AttributeError, 'arMasterSlaveDict does not support popitem().'
  def draw( self, framework=None ):
    """ d.draw( framework=None ).
    Loops through the dictionary, checks each item for an attribute named 'draw'. If
    an object has this attribute, it calls it, passing 'framework' as an argument if it is not None.  """
    for item in self.data.itervalues():
      if not hasattr( item, 'draw' ):
        continue
      drawMethod = getattr( item, 'draw' )
      if not callable( drawMethod ):
        continue
      if framework:
        item.draw( framework )
      else:
        item.draw()
  def processInteraction( self, effector ):
    """ d.processInteraction( effector ).
    Handles interaction between an arEffector and the objects in the dictionary. This is
    too complex a subject to describe here, see the Syzygy documentation on user interaction.
    Note that not all objects in the container have to be interactable, any objects that
    aren't instances of sub-classes of arInteractable are skipped.
    """
    # Interact with the grabbed object, if any.
    if effector.hasGrabbedObject():
      # get the grabbed object.
      grabbedPtr = effector.getGrabbedObject()
      # If this effector has grabbed an object not in this list, dont
      # interact with any of this list
      grabbedObject = None
      for item in self.data.itervalues():
        if isinstance( item, arInteractable ):
          # HACK! Found object in list
          if item is grabbedPtr:
            grabbedObject = item
            break
      if not grabbedObject:
        return False # not an error, just means no interaction occurred
      if grabbedObject.enabled():
        # If its grabbed an object in this set, interact only with that object.
        return grabbedObject.processInteraction( effector )
    # check to see if effector is already touching an object
    # if so, and it does not belong to this list, then abort
    oldTouchedObject = None
    oldTouchedPtr = None
    oldTouchedAddress = None
    if effector.hasTouchedObject():
      oldTouchedPtr = effector.getTouchedObject()
      for item in self.data.itervalues():
        if isinstance( item, arInteractable ):
          # HACK! Found object in list
          if item is oldTouchedPtr: 
            oldTouchedObject = item
      if not oldTouchedObject:
        return False # not an error, just means no interaction occurred
                     # with items in this list
    # Figure out the closest interactable to the effector (as determined
    # by their matrices). Go ahead and touch it (while untouching the
    # previously touched object if such are different).
    minDist = 1.e10    # A really big number, havent found how to get
                       # the max in python yet
    newTouchedObject = None
    for item in self.data.itervalues():
      if isinstance( item, arInteractable ):
        if item.enabled():
          dist = effector.calcDistance( item.getMatrix() )
          if (dist >= 0.) and (dist < minDist):
            minDist = dist
            newTouchedObject = item
    if oldTouchedPtr:
      if (not newTouchedObject) or (newTouchedObject != oldTouchedObject):
        oldTouchedPtr.untouch( effector )
    if not newTouchedObject:
      # Not touching any objects.
      return False
    # Finally, and most importantly, process the action of the effector on
    # the interactable.
    return newTouchedObject.processInteraction( effector )




# Auto-resizeable container for identical drawable objects.
# Usage:
# (1) add or delete objects from the objList property as desired on the master.
# (2) call dumpStateToString() in the preExchange() & use the frameworks
#       setString() method.
# (3) call frameworks getString() in postExchange() & setStateFromString()
#       method to finish synchronization.
# The class __init__ method will be called with the appropriate stateTuple
# for each new object if the list has been expanded on the master, so the
# __init__() should call setStateArgs(). If the
# correct number of objects already exists, then each will have its
# setStateArgs() method called directly.
class arMasterSlaveListSync:
  # Note that classFactory should be the name of the object class.
  def __init__(self,classFactory,useCPickle=False):
    self.objList = []
    self.classFactory = classFactory
    self.useCPickle = useCPickle
  def draw(self):
    for item in self.objList:
      item.draw()
  def dumpStateToString(self):
    stateList = []
    for item in self.objList:
      stateList.append(item.getStateArgs())
    # pickle seems to be considerably less flaky than cPickle here
    if self.useCPickle:
      pickleString = cPickle.dumps(stateList)
    else:
      if hasattr(pickle,'HIGHEST_PROTOCOL'):
        pickleString = pickle.dumps(stateList,pickle.HIGHEST_PROTOCOL)
      else:
        pickleString = pickle.dumps(stateList,True)
    return pickleString
  def setStateFromString(self,pickleString):
    stateList = cPickle.loads(pickleString)
    numItems = len(stateList)
    myNumItems = len(self.objList)
    if numItems < myNumItems:
      self.objList = self.objList[:numItems]
      myNumItems = numItems
    for i in range(myNumItems):
      self.objList[i].setStateArgs( stateList[i] )
    if numItems > myNumItems:
      for i in range(myNumItems,numItems):
        self.objList.append( self.classFactory( stateList[i] ) )



class arPySzgApp(arPyMasterSlaveFramework):
  """ Weird new framework sub-class. Has the following properties:
 
  1) Acts like a dictionary, i.e. you can do things like:

       self['my_data'] = [1,2,'foobar',3.1416]

     ...where self is the framework/application object. More specifically,
     you can treat it like an arMasterSlaveDict,
     with the same restrictions on data types for both dictionary
     keys and content. You can register classes to be transferred
     using self.addTransferTypes(), which calls arMasterSlaveDict.addTypes().

  2) Data inserted into the framework in this way are automagically
     transferred from master to slaves (provided you've called
     arPySzgApp.__init__() in you subclass' __init__().
 
  3) You can interact with the objects by calling self.processInteraction(),
     and draw them using self.drawItems() """   

  def __init__(self):
    arPyMasterSlaveFramework.__init__(self)
    self.__dict = arMasterSlaveDict( '__app_dict' )
    self.__usePrompt = '--prompt' in sys.argv
  def startCallback( self, framework, client ):
    if self.__usePrompt:
      if self.getMaster():
        ar_initPythonPrompt( self )
    # Register the dictionary of objects to be shared between master & slaves
    self.__dict.start( self )
    return self.onStart( client )
  def onStart( self, client ):
    return True
  def preExchangeCallback( self, framework ):
    if self.__usePrompt:
      ar_doPythonPrompt()
    self.onPreExchange()
    self.__dict.packState( self )
  def onPreExchange( self ):
    pass
  def postExchangeCallback( self, framework ):
    if not self.getMaster():
      # Unpack the message queue and use it to update the set
      # of objects in the dictionary as well as the state of each (using
      # its setState() method).
      self.__dict.unpackState( self )
    self.onPostExchange()
  def onPostExchange( self ):
    pass
  def addTransferTypes( self, classData ):
    self.__dict.addTypes( classData )
  def __getitem__( self, key ):
    return self.__dict[key]
  def __setitem__( self, key, value ):
    self.__dict[key] = value
  def __delitem__( self, key, value ):
    del self.__dict[key]
  def delValue( self, value ):
    self.__dict.delValue( value )
  def push( self, object ):
    self.__dict.push( object )
  def clear(self):
    self.__dict.clear()
  def processInteraction( self, effector ):
    self.__dict.processInteraction( effector )
  def drawItems( self ):
    self.__dict.draw()
  


class SzgRunner(object):
  app = None
  def __init__( self, app=None ):
    if app:
      self.app = app
  def __call__( self ):
    if not self.app.init(sys.argv):
      raise RuntimeError,'Unable to init application.'
    # Never returns unless something goes wrong
    if not self.app.start():
      raise RuntimeError,'Unable to start application.'

def szgrun( appClass ):
  SzgRunner( app=appClass() )()


#import rencode2

#def arMathEncoder():
#  """Returns a function that encodes x.toTuple()."""
#  def func( x, r ):
#    rencode2.encode_list( x.toTuple(), r )
#  return func
#  

#def arMathDecoder( cls ):
#  """Call class constructor with tuple."""
#  def func( x, f ):
#    x, f = rencode2.decode_list(x, f)
#    n = cls(x)
#    return n, f
#  return func


#class MasterSlaveApp( arMasterSlaveFramework ):
#  def __init__( self ):
#    arMasterSlaveFramework.__init__(self)
#    self._dict = dict()
#    self.registerType( arVector2, encode_func=arMathEncoder(), decode_func=arMathDecoder() )
#    self.registerType( arVector3, encode_func=arMathEncoder(), decode_func=arMathDecoder() )
#    self.registerType( arVector4, encode_func=arMathEncoder(), decode_func=arMathDecoder() )
#    self.registerType( arMatrix4, encode_func=arMathEncoder(), decode_func=arMathDecoder() )
#    self.registerType( arQuaternion, encode_func=arMathEncoder(), decode_func=arMathDecoder() )
#  def registerType( self, cls, encode_func=rencode2.defaultEncoder, \
#      decode_func=rencode2.defaultDecoder ):
#    rencode2.registerType( cls, encode_func, decode_func )
#  def __len__( self ):
#    return len( self._dict )
#  def __getitem__( self, key ):
#    return self._dict[key]
#  def __setitem__( self, key, value ):
#    self._dict[key] = value
#  def __delitem__( self, key ):
#    del self._dict[key]
#  def __iter__( self ):
#    return self._dict.__iter__()
#  def __contains__( self, item ):
#    return self._dict.__contains__( item )
#  def onStart( self, szgClient ):
#    self.initStringTransfer( '_dict' )
#    return arMasterSlaveFramework.onStart( self, szgClient )
#  def send( self ):
#    transferString = rencode2.dumps( self._dict )
#    self.setStringTransfer( '_dict', transferString )
#  def receive( self ):
#    transferString = self.getStringTransfer( '_dict' )
#    self._dict = rencode2.loads( transferString )
    
    
