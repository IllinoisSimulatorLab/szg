print 'szgexpt:',__file__
from _szgexpt import *

class arPyExperiment(arExperiment):
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
        raise TypeError, 'arPyExperiment error: addFactorSequence() called with invalid type ' \
           +str(factorType)+' for factor '+str(factorName)
        return False
      if not stat:
        raise RuntimeError, 'arPyExperiment error: addFactorSequence() failed for factor ' \
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
        raise TypeError, 'arPyExperiment error: setDataFieldSequence() called with invalid type ' \
           +str(dataFieldType)+' for dataField '+str(dataFieldName)
        return False
      if not stat:
        raise RuntimeError, 'arPyExperiment error: setDataFieldSequence() failed for dataField ' \
           +str(dataFieldName)
        return False
    return True
  

class arPyTrialGenerator(arTrialGenerator):
  def setFactorSequence( self, factorRecord, factorSequences ):
    for factorSequence in factorSequences:
      factorName = factorSequence[0]
      factorType = factorSequence[1]
      factorValue = factorSequence[2]
      stat = False
      if factorType == 'double': 
        stat = factorRecord.setDoubleField( factorName, factorValue )
      elif factorType == 'long':
        stat = factorRecord.setLongField( factorName, factorValue )
      elif factorType == 'string':
        stat = factorRecord.setStringField( factorName, factorValue )
      else:
        raise TypeError, 'arPyTrialGenerator: setFactorTuples() called with bad type for factor "'+ \
                str(factorName)+'" with type "'+str(factorType)
      if not stat:
        raise RuntimeError, 'arPyTrialGenerator: setFactorTuples() failed for factor "'+ \
                str(factorName)+'" with type "'+str(factorType)+'" and value:\n'+str(factorValue)
        return False
    return True
  def onNewTrial( self, factors ):
    raise RuntimeError, 'arPyTrialGenerator: you must override onNewTrial().'
    return False


class arPyExperimentTrialPhase(arExperimentTrialPhase):
  def onInit( self, framework, expt ):
    print 'arPyExperimentTrialPhase: you must override onInit().'
    raise RuntimeError, 'arPyExperimentTrialPhase: you must override onInit().'
    return False
  def onUpdate( self, framework, expt ):
    print 'arPyExperimentTrialPhase: you must override onUpdate().'
    raise RuntimeError, 'arPyExperimentTrialPhase: you must override onUpdate().'
    return False
  def onUpdateEvent( self, framework, expt, filter, event ):
    print 'arPyExperimentTrialPhase: you must override onUpdateEvent().'
    raise RuntimeError, 'arPyExperimentTrialPhase: you must override onUpdateEvent().'
    return False
  
