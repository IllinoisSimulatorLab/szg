print 'szgexpt:',__file__
from _szgexpt import *
import szg
import time
import copy


class arPyExptTrialGenerator(arTrialGenerator):
  'Override the trial generator docstring to set a comment.'
  def __init__( self ):
    if hasattr( self, '__doc__' ):
      arTrialGenerator.__init__(self,self.__doc__)
    else:
      raise RuntimeError, "Trial generator _must_ have a __doc__ attribute (initial comment describing experiment)"
    self.trialNum = 0
    self.totalTrials = -1
    self.completed = False
  def handleSpecialSubjects( self, subjectLabel ):
    raise NotImplementedError, 'The arPyExptTrialGenerator.handleSpecialSubjects() method has not been overridden!'
  def onNewTrial( self, factors ):
    # Should return True if new parameters have been generated, False if not (and should
    # set self.completed = True if returning False because experiment is over).
    raise NotImplementedError, 'The arPyExptTrialGenerator.onNewTrial() method has not been overridden!'
  def setFactors( self, factors, factorDict ):
    factDict = copy.copy( factorDict )
    for i in range(len(getExpt().__class__.factors)):
      fac = getExpt().factors[i]
      factorName = fac[0]
      factorType = fac[1]
      try:
        c = factDict[factorName]
        del factDict[factorName]
      except KeyError:
        print "arPyExptTrialGenerator.setFactors(): factorDict missing '"+factorName+"'."
        return False
      if factorType == 'string' or factorType == 'stringset':
        if not factors.setStringField( factorName, c  ):
          print "Failed to set factor '"+factorName+"' with value ("+str(c)+")."
          return False        
      else:
        if type(c) == type([]):
          val = c
        elif type(c) == type(()):
          val = list(c)
        else:
          val = [c]
        if factorType == 'long':
          try:
            if not factors.setLongField( factorName, val  ):
              print "Failed to set factor '"+factorName+"' with value ("+str(val)+")."
              return False        
          except Exception, msg:
            print "Failed to set factor '"+factorName+"' with value ("+str(val)+")."
            print msg
            return False        
        else:
          try:
            if not factors.setDoubleField( factorName, val  ):
              print "Failed to set factor '"+factorName+"' with value ("+str(val)+")."
              return False        
          except Exception, msg:
            print "Failed to set factor '"+factorName+"' with value ("+str(val)+")."
            print msg
            return False        
    if len(factDict) > 0:
      print "arPyExptTrialGenerator.setFactors(): factorDict contains extra entries."
      print factDict
      return False
    return True


# Experiment classes
class arPyExptTrialPhase(object):
  def __init__( self ):
    self.firstTrial = True
  def init(self):
    raise NotImplementedError, 'The arPyExptTrialPhase.init() method has not been overridden!'
  def update(self):
    raise NotImplementedError, 'The arPyExptTrialPhase.update() method has not been overridden!'
  def finish(self):
    pass
  def getName(self):
    return self.__class__.__name__

class arPyTimedExptTrialPhase(arPyExptTrialPhase):
  duration = 0. # seconds
  def start(self):
    self._startTime = time.clock()
  def time(self):
    return time.clock() - self._startTime
  def done(self):
    return time.clock() - self._startTime > self.duration


class arPyExptException(RuntimeError):
  pass


_expt = None

class arPyExperiment(arExperiment):
  experimentName = ''               # String
  subjectParameters =  {'gender':'string'}
  factors = ()                      # Tuple of (name_string,type_string) tuples.
                                    #  type_string must be 'double', 'long', or 'string'.
  dataFields = ()                   # Ditto.
  trialGeneratorClass = None        # arPyExptTrialGenerator subclass.
  trialPhaseClasses = ()            # Tuple of arPyExptTrialPhase Subclasses. First one will be activated first.
  dataDebug = True                  # If True, warns if any data fields haven't been updated on each trial.

  def __init__( self ):
    global _expt
    if _expt is not None:
      raise RuntimeError, 'You can only have one experiment instance at a time, fool!'
    _expt = self
    arExperiment.__init__(self)
    self.valid = True
    self.responseMessage = ''
    self.trialData = []
    self.savingData = True
    self.currentTrialPhase = None

  def started(self):
    return self.currentTrialPhase is not None

  def makeTrialPhases( self ):
    self.trialPhases = {}
    for c in self.__class__.trialPhaseClasses:
      self.trialPhases[c.__name__] = c()

  def getTrialPhaseName(self):
    if not self.currentTrialPhase:
      return None
    return self.currentTrialPhase.getName()

  def getEyeSpacing( self ):
    return self.getDoubleSubjectParameter( 'eye_spacing_cm' )

  def activateTrialPhase( self, name ):
    if self.currentTrialPhase:
      self.currentTrialPhase.finish()
    try:
      print 'Activating',name
      self.currentTrialPhase = self.trialPhases[name]
    except KeyError:
      if self.trialPhases.has_key( 'ErrorTrialPhase' ):
        self.activateTrialPhase( 'ErrorTrialPhase' )
      getApp().reportError( 'No trial phase named '+name )
      return
    self.currentTrialPhase.init()

  def __nonzero__(self):
    return self.valid

  def start(self):
    try:
      for (name,typeString) in self.subjectParameters.iteritems():
        if typeString == 'string':
          if not self.addStringSubjectParameter( name ):
            raise arPyExptException, "Failed to add "+name+" subject parameter."
        elif typeString == 'double':
          if not self.addDoubleSubjectParameter( name ):
            raise arPyExptException, "Failed to add "+name+" subject parameter."
        elif typeString == 'long':
          if not self.addLongSubjectParameter( name ):
            raise arPyExptException, "Failed to add "+name+" subject parameter."
        else:
          raise ValueError, "Subject parameter type strings must be 'string', 'double', or 'long'"
      self.trialGenerator = self.trialGeneratorClass()
      if not self.setTrialGenerator( self.trialGenerator ):
        raise arPyExptException, 'Failed to assign trial generator.'
      if not self.setName( self.experimentName ):
        raise arPyExptException, 'Error: setName() failed.'
      # Add experimental factors
      self.addFactorSequence( self.factors  )
      # Add data fields
      self.addDataFieldSequence( self.dataFields )
      if not self.init( getApp(), getApp().getSZGClient() ):
        raise arPyExptException, 'Failed to initialize experiment.'
      subjectLabel = self.getStringSubjectParameter( 'label' )
      self.trialGenerator.handleSpecialSubjects( subjectLabel )
      if not arExperiment.start(self):
        raise arPyExptException, 'Failed to start() experiment.'
    except arPyExptException, msg:
      print msg
      self.valid = False
      return False
    print 'Experiment initialized.'
    self.makeTrialPhases()
    if not self.valid:
      print self.experimentName,'failed to start.'
      return False
    self.startTime = time.clock()
    print 'Starting experiment.'
    self.activateTrialPhase( self.trialPhaseClasses[0].__name__ )
    return True

  def addFactorSequence( self, factorSequence ):
    for factorTuple in factorSequence:
      factorName = factorTuple[0]
      factorType = factorTuple[1]
      stat = False
      if factorType == 'long':
        stat = self.addLongFactor( factorName )
      elif factorType == 'double': 
        stat = self.addDoubleFactor( factorName )
      elif factorType == 'string': 
        stat = self.addStringFactor( factorName )
      elif factorType == 'stringset': 
        defaultList = factorTuple[2]
        stat = self.addStringFactorSet( factorName, defaultList )
      else:
        raise arPyExptException, 'arPyExperiment error: addFactorSequence() called with invalid type ' \
           +str(factorType)+' for factor '+str(factorName)
        return False
      if not stat:
        raise arPyExptException, 'arPyExperiment error: addFactorSequence() failed for factor ' \
           +str(factorName)
        return False
    return True

  def addDataFieldSequence( self, dataFieldSequence ):
    for dataFieldTuple in dataFieldSequence:
      dataFieldName = dataFieldTuple[0]
      dataFieldType = dataFieldTuple[1]
      stat = False
      if dataFieldType == 'long':
        stat = self.addLongDataField( dataFieldName )
      elif dataFieldType == 'double': 
        stat = self.addDoubleDataField( dataFieldName )
      elif dataFieldType == 'string': 
        stat = self.addStringDataField( dataFieldName )
      else:
        raise TypeError, 'arPyExperiment error: addDataFieldSequence() called with invalid type ' \
           +str(dataFieldType)+' for dataField '+str(dataFieldName)
        return False
      if not stat:
        raise RuntimeError, 'arPyExperiment error: addDataFieldSequence() failed for dataField ' \
           +str(dataFieldName)
        return False
    return True

  def setDataFieldSequence( self, dataFieldSequence ):
    for dataFieldTuple in dataFieldSequence:
      dataFieldName = dataFieldTuple[0]
      dataFieldType = dataFieldTuple[1]
      dataFieldValue = dataFieldTuple[2]
      stat = False
      if dataFieldType == 'long':
        stat = self.setLongData( dataFieldName, dataFieldValue )
      elif dataFieldType == 'double': 
        stat = self.setDoubleData( dataFieldName, dataFieldValue )
      elif dataFieldType == 'string': 
        stat = self.setStringData( dataFieldName, dataFieldValue )
      else:
        raise arPyExptException, 'arPyExperiment error: setDataFieldSequence() called with invalid type ' \
           +str(dataFieldType)+' for dataField '+str(dataFieldName)
        return False
      if not stat:
        raise arPyExptException, 'arPyExperiment error: setDataFieldSequence() failed for dataField ' \
           +str(dataFieldName)
        return False
    return True

  def update(self):
    if self.currentTrialPhase:
      self.currentTrialPhase.update()

  def sendGuiResponseMessages( self, messageBody ):
    if not getApp().guiProcessID:
      print self.experimentName,'warning: no gui process found.'
      return
    cl = getApp().getSZGClient()
    trialNum = self.trialGenerator.trialNum
    totalTrials = self.trialGenerator.totalTrials
    if totalTrials == -1:
      totalTrials = trialNum
    msgBody = str(trialNum)+'|'+str(totalTrials)
    m = cl.sendMessage( 'trialnum', msgBody, getApp().guiProcessID, False )
    if not self.savingData:
      messageBody = 'DATA NOT BEING SAVED!!!\n\n'+messageBody
    m = cl.sendMessage( 'responseinfo', messageBody, getApp().guiProcessID, False )
    
  def saveData(self):
    # Note: should return True or False.
    raise NotImplementedError, 'The arPyExperiment.saveData() method has not been overridden!'

  def saveTrialData(self):
    exptFieldNames = [i[0] for i in self.__class__.dataFields]
    trialFieldNames = [i[0] for i in self.trialData]
    if self.__class__.dataDebug:
      for n in exptFieldNames:
        if not n in trialFieldNames:
          print "WARNING: data field '"+n+"' has not been updated."
    try:
      self.setDataFieldSequence( self.trialData )
    except arPyExptException, value:
      print value
      self.trialData = []
      return False
    self.trialData = []
    return True

def getExpt():
  return _expt


_app = None

class arPyExptApp(szg.arPyMasterSlaveFramework):
  unitsPerFoot = 2.54*12./100.   # default to meters
  nearClipDistance = .1
  farClipDistance = 20.

  def __init__(self):
    global _app
    if _app is not None:
      raise RuntimeError, 'You can only have one application instance at a time, fool!'
    _app = self
    szg.arPyMasterSlaveFramework.__init__(self)
    self.imASpy = False
    self.guiProcessID = None
    # Tell the framework what units we're using.
    self.setUnitConversion( self.unitsPerFoot )
    # Near & far clipping planes.
    self.setClipPlanes( self.nearClipDistance, self.farClipDistance )

  # Note returning False will cause program to exit.
  def onStart( self, szgClient ):
    if self.getMaster():
      if hasattr( self.__class__, 'experimentClass' ):
        self.findGuiProcess()
        print 'creating experiment'
        self.experiment = self.__class__.experimentClass( self, szgClient )
        print 'done creating experiment'
        if not self.experiment:
          print 'experiment creation failed.'
          return False
        print 'experiment creation succeeded.'
        if szgClient.getAttribute( 'SZG_EXPT', 'save_data' )=='false':
          print 'DATA NOT BEING SAVED.'
          self.experiment.savingData = False
        else:
          print 'Data being saved.'
    if szgClient.getAttribute( 'SZG_EXPT', 'im_a_spy' )=='true':
      print 'I AM A SPY!'
      self.imASpy = True
    return szg.arPyMasterSlaveFramework.onStart( self, szgClient )

  def onWindowStartGL( self, winInfo ):
    szg.arPyMasterSlaveFramework.onWindowStartGL( self, winInfo )
    self.viewportSize = (winInfo.getSizeX(),winInfo.getSizeY())

  def onWindowEvent( self, winInfo ):
    szg.arPyMasterSlaveFramework.onWindowEvent( self, winInfo )
    self.viewportSize = (winInfo.getSizeX(),winInfo.getSizeY())

  def onPreExchange( self ):
    szg.arPyMasterSlaveFramework.onPreExchange(self)
    if hasattr( self, 'experiment' ):
      if self.experiment:
        if not self.experiment.started():
          if not self.experiment.start():
            raise RuntimeError, 'The experiment failed to start.'
          # Normally the eye spacing is read from the Syzygy database. We want to use
          # the value read from the subject database file instead.
          eyeSpacing = self.experiment.getEyeSpacing()
          self.setEyeSpacing( eyeSpacing/(2.54*12) )
        self.experiment.update()

  def onPostExchange( self ):
    szg.arPyMasterSlaveFramework.onPostExchange(self)

  # Draw callback
  def onDraw( self, win, viewport ):
    raise NotImplementedError, "arPyExptApp.onDraw() must be overridden!"

  def dps(self):
    cl = self.getSZGClient()
    ot = cl.getProcessList()
    stringList = ot.split(':')
    processList = []
    for item in stringList:
      tokenList = item.split("/")
      if len(tokenList) == 3:
        ipAddress = tokenList[0]
        ipList = ipAddress.split(".")
        computerName = ipList[0]
        processName = tokenList[1]
        processNumber = int(tokenList[2])
        processList.append( (computerName, processName, processNumber) )
    return processList
    
  def findGuiProcess(self):
    processList = self.dps()
    processNumbers = [item[2] for item in processList if item[1] == 'exptgui']
    if len(processNumbers)==0:
      print 'Failed to find exptgui'
      self.guiProcessID = None
      return
    self.guiProcessID = processNumbers[0]

  def reportError( self, msg ):
    print 'ERROR:',msg
    self.speak( 'The following error has occurred: '+msg )


def getApp():
  return _app

# utility function (from ASPN Python Cookbook) to generate all combinations of members of two sequences
# modified by JAC to allow hierarchical randomization.
def combine(*seqin,**kwds):
  '''returns a list of all combinations of argument sequences.
for example: combine((1,2),(3,4)) returns
[[1, 3], [1, 4], [2, 3], [2, 4]]'''
  def rloop(seqin,listout,comb,shuffleLevels,sequenceLevel):
    '''recursive looping function'''
    if seqin:             # any more sequences to process?
      seq = copy.copy( seqin[0] )
      if shuffleLevels and sequenceLevel in shuffleLevels:
        random.shuffle( seq )
      for item in seq:
        newcomb=comb+[item]   # add next item to current comb
        # call rloop w/ rem seqs, newcomb
        rloop(seqin[1:],listout,newcomb,shuffleLevels,sequenceLevel+1)
    else:               # processing last sequence
      listout.append(comb)    # comb finished, add to list
  shuffleLevels = None
  if kwds.has_key('shuffleLevels'):
    shuffleLevels = kwds['shuffleLevels']
  listout=[]            # listout initialization
  rloop(seqin,listout,[],shuffleLevels,1)     # start recursive process
  return listout

def permutations(l):
  # Compute the list of all permutations of l
  if len(l) <= 1:
    return [l]
  r = []
  for i in range(len(l)):
    s = l[:i] + l[i+1:]
    p = permutations(s)
    for x in p:
      r.append(l[i:i+1] + x)
  return r



