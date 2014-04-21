import os
import random
import time
import szg
import szgexpt
from szgexpt import getExpt, getApp


EXPT_PARAMS = """
<szg_config>
    <assign>
        NULL SZG_EXPT experiment TestExpt
        NULL SZG_EXPT path {FILE_DIR}
        NULL SZG_EXPT subject TEST
        NULL SZG_EXPT data_file test
        NULL SZG_EXPT save_data true
    </assign>
</szg_config>
""".format( FILE_DIR=os.path.dirname( os.path.abspath( __file__ ) ).replace('/','\\') )



class TestTrialGenerator(szgexpt.arPyExptTrialGenerator):
    'Dummy Experiment Trial Generator'

    def __init__(self):
        szgexpt.arPyExptTrialGenerator.__init__(self)
        self.trialNum = 0
        self.totalTrials = 10


    def handleSpecialSubjects( self, subjectLabel ):
        pass


    def onNewTrial( self, factors ):
        self.trialNum += 1
        if self.trialNum > self.totalTrials:
            self.completed = True
            return False
        print 'Trial generator: trial',self.trialNum
        distraction = random.choice( ['YES','NO'] )
        interval = random.uniform( 0.5, 1.5 )
        twoWayTraffic = random.choice( [0,1] )
        repNum = self.trialNum
        currTrialDict = {
                'distraction':distraction,
                'car_interval':[interval],
                'two_way_traffic':[twoWayTraffic],
                'repetition_number':[repNum]
                }
        if not self.setFactors( factors, currTrialDict ): 
            print 'Failed to set parameters.'
            return False
        return True



class NewTrialPhase(szgexpt.arPyExptTrialPhase):

    def init(self):
        if not self.getTrialParams():
            return
        if self.firstTrial:
            subjectName = getExpt().getStringSubjectParameter( 'name' )
            subjectLabel = getExpt().getStringSubjectParameter( 'label' )
            print "Hello, {}".format( subjectName )
            print "There will be {} trials in all.".format( getExpt().trialGenerator.totalTrials )
            self.firstTrial = False
        getExpt().startTime = time.clock()
        getExpt().activateTrialPhase( 'ResponseTrialPhase' )

    def getTrialParams(self):
        if not getExpt().newTrial():
            if getExpt().completed():
                if getExpt().trialGenerator.completed:
                    getExpt().activateTrialPhase( 'EndTrialPhase' )
                else:
                    print 'An error occurred in the trial generator.'
                    getExpt().activateTrialPhase( 'ErrorTrialPhase' )
            else:
                print 'Failed to generate trial parameters.'
                getExpt().activateTrialPhase( 'ErrorTrialPhase' )
            return False
        return True

    def update( self ):
        pass

    def finish(self):
        getExpt().trialData = [('trial_number', 'long', [getExpt().trialGenerator.trialNum])]



class ResponseTrialPhase(szgexpt.arPyExptTrialPhase):

    def init( self ):
        pass

    def update( self ):
        if random.uniform( 0., 1. ) < 0.99:
            return
        self.duration = time.clock() - getExpt().startTime
        self.success = random.choice( [0,1] )
        getExpt().activateTrialPhase( 'NewTrialPhase' )


    def finish(self):
        getExpt().trialData.extend([ \
                        ('success',            'long',   [self.success]), \
                        ('trial_duration_sec', 'double', [self.duration]) \
                    ])
        if not getExpt().saveData():
            getExpt().activateTrialPhase( 'ErrorTrialPhase' )
            print 'Failed to save trial data'
            return
       


class EndTrialPhase(szgexpt.arPyTimedExptTrialPhase):

    def init( self ):
        self.duration = 1.
        print 'Thats the end of the session. Thank you.'
        self.start()


    def update( self ):
        if self.done():
            getExpt().stop()
            getApp().stop( False )



class ErrorTrialPhase(szgexpt.arPyExptTrialPhase):

    def init( self ):
        print 'ERROR! Exiting...'
        getExpt().stop()
        getApp().stop( False )

    def update( self ):
        pass



class TestExperiment(szgexpt.arPyExperiment):
    'Dummy Experiment'
    experimentName = 'TestExpt'
    subjectParameters =  {'gender':'string','age':'long'}
    factors = ( \
              ('distraction',            'string'), \
              ('car_interval',           'double'), \
              ('two_way_traffic',        'long'), \
              ('repetition_number',      'long') \
              )
    dataFields = ( \
              ('trial_number',            'long'), \
              ('success',                 'long'), \
              ('trial_duration_sec',      'double'), \
              )
    trialGeneratorClass = TestTrialGenerator
    trialPhaseClasses = ( \
          NewTrialPhase, \
          ResponseTrialPhase, \
          EndTrialPhase, \
          ErrorTrialPhase
          )
    dataDebug = True
    
    def __init__( self ):
        szgexpt.arPyExperiment.__init__( self )


    def saveData( self ):
        return self.saveTrialData()
        

class TestExptApp(szgexpt.arPyExptApp):
    experimentClass = TestExperiment

    def onDraw( self, win, viewport ):
        pass

if __name__ == '__main__':
    f = open( 'szg_parameters.xml', 'wb' )
    f.write( EXPT_PARAMS )
    f.close()
    szg.szgrun( TestExptApp )
