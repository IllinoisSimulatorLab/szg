#ifndef AREXPERIMENT_H
#define AREXPERIMENT_H

#include <string>
#include <vector>
#include <map>
#include "arSZGAppFramework.h"
#include "arDataType.h"
#include "arSZGClient.h"
#include "arExperimentDataRecord.h"
#include "arTrialGenerator.h"
#include "arDataSaver.h"
#include "arExperimentTrialPhase.h"
#include "arHumanSubject.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"


/// Main experiment class

/// This is the only experiment-related class that the application
/// has to see. Generally, there should only be one of them. The
/// sequence of events should be: instantiate, tell it what the
/// various experimental parameters (factors) are via the various
/// set...Factor() methods, tell it what data fields are to be saved
/// via the setDataField() method. init(); for
/// the most common type of experiment, "enumerated" (i.e. the
/// config file enumerates all parameter values for each trial),
/// the config file will be read in and validated at this point.
/// Before each trial, call newTrial(); this will save data from the
/// preceding trial, load parameters for the next one, and return
/// false if it's time to stop.
///
/// Basically, the application just needs to know how to run an
/// individual trial given any possible combination of trial values;
/// the arExperiment and its (private) arTrialGenerator member are
/// responsible for determining the order of trials, based on info
/// in a configuration file.
///
/// Required database parameters:
/// SZG_EXPT/path is the path to the base experiment directory.  The
/// path for this particular experiment is base path + path delimiter
/// (/ or \) + executable name + Data. This directory must contain
/// separate subdirectories for each subject's config and data files,
/// named by the subject's identifier string (SZG_EXPT/subject). This
/// might be set to e.g. the subject's initials, or "S1", "S2", etc.
/// The config/data file prefix for this session should be in
/// SZG_EXPT/file_name. The actual config and data file names will depend
/// on the experimental method (the method used to determine the
/// parameter values for the next trial) and the desired data format,
/// which are specified by SZG_EXPT/method and SZG_EXPT/data_style.
/// As of this writing (10/1/02), the only available options are
/// SZG_EXPT/method = "enumerated" and SZG_EXPT/data_style = "xml".
/// If the file prefix is set to e.g. "test", the config file name for this
/// case should be "test_config.xml" and the data file will be "test_dat.xml"
///
/// Example: The experiment name (and the executable name) is "expt1" (executable
/// name is really "expt1.exe" for win32).  SZG_EXPT/subect = "SHMOO".
/// SZG_EXPT/file_name = "session42", SZG_EXPT/method = "enumerated",
/// SZG_EXPT/data_style = "xml", SZG_EXPT/path = "home/public/expt".
/// The main experiment directory must be /home/public/expt/expt1Data.
/// This must contain the subject database, "subject_data.xml", which in turn
/// must contain a record whose "label" field is "SHMOO". It must also
/// contain this subject's data directory, also named "SHMOO".
/// /home/public/expt/expt1Data/SHMOO must contain an enumerated xml config file,
/// "session42_config.xml". It must _NOT_ contain a file or directory named
/// "session42_dat.xml", because the experiment will attempt to create this and will
/// fail if it already exists.

class SZG_CALL arExperiment {
  public:    
    arExperiment();
    virtual ~arExperiment();
    
    /// Define an experimental parameter or 'factor'..
    /// Note that if address == 0, an internal buffer will
    /// be allocated
    bool addFactor( const std::string fname, const arDataType theType,
                    const void* address=0, const unsigned int theSize=1 );
    /// Get pointer to a factor
    const void* const getFactor( const std::string fname,
                           const arDataType theType,
                           unsigned int& theSize );
    /// Define a character string factor with a list of |-delimited legal values.
    bool addCharFactor( const std::string fname, const char* const legalValues,
                        const char* address=0 );
    bool addCharFactor( const std::string fname, vector<string>& legalValues,
                        const char* address=0 );
    /// Define a data field (before init() only)
    bool addDataField( const std::string fname, const arDataType theType );
    /// Define a data field (before init()) or change its address and size (after init())
    /// Deprecated for former purpose, use addDataField() instead.
    bool setDataField( const std::string fname, const arDataType theType,
                       const void* address, const unsigned int theSize=1 );
    /// Get pointer to a data field
    const void* getDataField( const std::string fname,
                              const arDataType theType,
                              unsigned int& theSize );
    /// Set the experiment name. This would normally be the name of the executable
    /// (minus .exe under windows). _Use this only in special cases_, e.g. when
    /// creating a demo that will reside in the same directory as an experiment.
    /// Must be called before init().
    bool setName( const std::string& name );
    /// Get the experiment name.
    std::string getName() const { return _experimentName; }
    /// Determine whether or not data will be saved. This would normally be read
    /// from SZG_EXPT/save_data, but if this has been called that parameter will
    /// be ignored. Use this only in special cases, e.g. when
    /// creating a demo that will reside in the same directory as an experiment.
    /// Must be called before init().
    bool saveData( bool yesno );
    /// Set the trial-generation method. This would normally be read from
    /// SZG_EXPT/method, but if this has been called that parameter will be ignored.
    /// Must be called before init().
    bool setTrialGenMethod( const std::string& method );
    bool setTrialGenMethod( arTrialGenerator* methodPtr );
    /// Add a data-saver. Must be called before init().
    bool addDataSaver( const std::string& dsName, arDataSaver* dsPtr );
    /// Initialize the experiment.
    bool init( arSZGAppFramework* framework, arSZGClient& SZGClient );
    /// Start the experiment.
    bool start();
    /// Stop the experiment.
    bool stop();
    /// modify the world, check for a trial phase change or trial end
//    bool update();
    /// Save data from last trial, get parameters for next.
    bool newTrial( bool saveLastTrial=true );
    /// Return current trial number.
    long currentTrialNumber() const;
    /// Return total number of trials (-1 if undefined, not prespecified).
    long numberTrials() const;
    /// Return completion status.
    bool completed() const;
    
    /// Define a parameter whose value is to be read in from the human subject database.
    bool addSubjectParameter( const std::string theName, const arDataType theType );
    /// Get the value of a named subject parameter for the current human subject.
    bool getSubjectParameter( const std::string theName, arDataType theType, void* address );
    bool getCharSubjectParameter( const std::string& theName, std::string& value );
    
    /// Add or replace a trial phase object by name
    virtual bool setTrialPhase( const std::string theName, arExperimentTrialPhase* thePhase );
    /// Get the name of the current trial phase
    std::string currentTrialPhase();
    /// Activate a trial phase by name
    virtual bool activateTrialPhase( const std::string theName, arSZGAppFramework* fw );
    /// Calls update() method of active trial phase object
    virtual bool updateTrialPhase( arSZGAppFramework* fw );
    virtual bool updateTrialPhase( arSZGAppFramework* fw, arIOFilter* filter, arInputEvent& event );
    /// Allocate a named storage buffer
    /// If it has the same name and type as a data field, the data field
    /// will be set to point to the allocated buffer
    /// If it has the same name but a different type, it will fail.
//    void* const makeStorage( const std::string& name, arDataType typ, unsigned int size );
    /// Get a named storage buffer
//    void* const getStorage( const std::string& name, arDataType typ, unsigned int& size );
    /// Allocate storage for internally-stored data field
    void* const allocateDataField( const std::string& name, arDataType typ, unsigned int size );
    bool setDataFieldData( const std::string& name, arDataType typ, const void* const address, unsigned int size );
    
  protected:
    void _printDataFields();
    bool _configured;
    bool _running;
    bool _completed;
    long _currentTrialNumber;
    std::string _experimentName;
    std::string _trialGenMethod;
    std::string _dataStyle;
    
    arTrialGenerator* _trialGen;
    arDataSaverMap_t _dataSavers;
    arExperimentDataRecord _factors;
    arExperimentDataRecord _dataFields;    
    arStringSetMap_t _legalStringValues;
    
    arHumanSubject _subject;
    
    arExperimentTrialPhaseMap_t _trialPhases;
    arExperimentTrialPhase* _currentPhasePtr;
};
#endif        //  #ifndefAREXPERIMENT_H

