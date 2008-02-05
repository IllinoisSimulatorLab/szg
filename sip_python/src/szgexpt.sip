%Module _szgexpt 0

// Defines mapping from python to C++ strings.
%Include typemaps.sip

//%Include szgclient.sip
//%Include input.sip
//%Include interaction.sip
//%Include frameworks.sip

%Import szg.sip

%ModuleHeaderCode
#include <string>
using namespace std;
%End

class arTrialGenerator {
%TypeHeaderCode
#include "arTrialGenerator.h"
%End
  public:
    arTrialGenerator( const string& comment="" );
    virtual ~arTrialGenerator();
    string comment();
    
// Don't need to call this from Python at this point....
//
//    virtual bool init( const string experiment,
//                       string configPath,
//                       const arHumanSubject& subject,
//                       arExperimentDataRecord& factors,
//                       const arStringSetMap_t& legalStringValues,
//                       arSZGClient& SZGClient );
                    
    virtual bool newTrial( arExperimentDataRecord& factors ) = 0 /PyName=onNewTrial/;
    unsigned long trialNumber() const;
    virtual long numberTrials() const;
  private:
    arTrialGenerator( const arTrialGenerator& );
};


class arExperimentTrialPhase {
%TypeHeaderCode
#include "arExperimentTrialPhase.h"
%End
  public:
    arExperimentTrialPhase( const string& name="" );
    virtual ~arExperimentTrialPhase();
    virtual bool init( arSZGAppFramework*, arExperiment* ) /PyName=onInit/;
    virtual bool update( arSZGAppFramework*, arExperiment* ) /PyName=onUpdate/;
    virtual bool update( arSZGAppFramework*, arExperiment*,
                         arIOFilter*, arInputEvent& event ) /PyName=onUpdate/;
    string getName() const;
  private:
    arExperimentTrialPhase( const arExperimentTrialPhase& );
};


class arExperiment {
%TypeHeaderCode
#include "arExperiment.h"
%End
  public:    
    arExperiment();
    virtual ~arExperiment();
    
    /// Define an experimental parameter or 'factor'..
    /// Note that if address == 0, an internal buffer will
    /// be allocated
    //bool addFactor( const string fname, const arDataType theType,
    //                const void* address=0, const unsigned int theSize=1 );
    bool addLongFactor( const string& sname );
%MethodCode
      sipRes = sipCpp->addFactor( *a0, AR_LONG );
%End

    bool addDoubleFactor( const string& sname );
%MethodCode
      sipRes = sipCpp->addFactor( *a0, AR_DOUBLE );
%End

    bool addStringFactor( const string& sname );
%MethodCode
      sipRes = sipCpp->addFactor( *a0, AR_CHAR );
%End


    /// Get pointer to a factor
    //const void* const getFactor( const string fname,
    //                       const arDataType theType,
    //                       unsigned int& theSize );
    vector<long> getLongFactor( const string& sname );
%MethodCode
      unsigned int size;
      const long* const tmp = (const long* const)sipCpp->getFactor( *a0, AR_LONG, size );
      if (!tmp) {
        PyErr_SetString( PyExc_RuntimeError, "getLongFactor() failed." );
        sipIsErr = 1;
      } else {
        sipRes = new vector<long>( tmp, tmp+size );
      }
%End

    vector<double> getDoubleFactor( const string& sname );
%MethodCode
      unsigned int size;
      const double* const tmp = (const double* const)sipCpp->getFactor( *a0, AR_DOUBLE, size );
      if (!tmp) {
        PyErr_SetString( PyExc_RuntimeError, "getDoubleFactor() failed." );
        sipIsErr = 1;
      } else {
        sipRes = new vector<double>( tmp, tmp+size );
      }
%End

    string getStringFactor( const string& sname );
%MethodCode
      unsigned int size;
      const char* const tmp = (const char* const)sipCpp->getFactor( *a0, AR_CHAR, size );
      if (!tmp) {
        PyErr_SetString(PyExc_RuntimeError,"getStringFactor() failed");
        sipIsErr = 1;
      } else {
        sipRes = new string(tmp);
      }
%End


    /// Define a character string factor with a list of |-delimited legal values.
//    bool addCharFactor( const string fname, const char* const legalValues,
//                        const char* address=0 );
    /// Define a data field (before init() only)
    //bool addDataField( const string fname, const arDataType theType );

    bool addLongDataField( const string& sname );
%MethodCode
      sipRes = sipCpp->addDataField( *a0, AR_LONG );
%End

    bool addDoubleDataField( const string& sname );
%MethodCode
      sipRes = sipCpp->addDataField( *a0, AR_DOUBLE );
%End

    bool addStringDataField( const string& sname );
%MethodCode
      sipRes = sipCpp->addDataField( *a0, AR_CHAR );
%End

    /// Define a data field (before init()) or change its address and size (after init())
    /// Deprecated for former purpose, use addDataField() instead.
    //bool setDataField( const string fname, const arDataType theType,
    //                   const void* address, const unsigned int theSize=1 );

    bool setLongData( const string& sname, vector<long>& );
%MethodCode
      long* tmp = new long[a1->size()];
      if (!tmp) {
        PyErr_SetString(PyExc_MemoryError, "setLongData() memory allocation failed");
        sipIsErr = 1;
      } else {
        vector<long>::const_iterator iter;
        int i(0);
        for (iter=a1->begin(); iter != a1->end(); ++iter) {
          tmp[i++] = *iter;
        }
        sipRes = sipCpp->setDataFieldData( *a0, AR_LONG, (const void* const)tmp, a1->size() );
      }
      delete[] tmp;
%End

    bool setDoubleData( const string& sname, vector<double>& );
%MethodCode
      double* tmp = new double[a1->size()];
      if (!tmp) {
        PyErr_SetString(PyExc_MemoryError, "setDoubleData() memory allocation failed");
        sipIsErr = 1;
      } else {
        vector<double>::const_iterator iter;
        int i(0);
        for (iter=a1->begin(); iter != a1->end(); ++iter) {
          tmp[i++] = *iter;
        }
        sipRes = sipCpp->setDataFieldData( *a0, AR_DOUBLE, (const void* const)tmp, a1->size() );
      }
      delete[] tmp;
%End

    bool setStringData( const string& sname, const string& stringData );
%MethodCode
      sipRes = sipCpp->setDataFieldData( *a0, AR_CHAR, (const void* const)a1->c_str(), (unsigned int)a1->size() );
%End

    /// Get pointer to a data field
    //const void* getDataField( const string fname,
    //                          const arDataType theType,
    //                          unsigned int& theSize );
    vector<long> getLongData( const string& sname );
%MethodCode
      unsigned int size;
      const long* const tmp = (const long* const)sipCpp->getDataField( *a0, AR_LONG, size );
      if (!tmp) {
        PyErr_SetString( PyExc_RuntimeError, "getLongData() failed." );
        sipIsErr = 1;
      } else {
        sipRes = new vector<long>( tmp, tmp+size );
      }
%End

    vector<double> getDoubleData( const string& sname );
%MethodCode
      unsigned int size;
      const double* const tmp = (const double* const)sipCpp->getDataField( *a0, AR_DOUBLE, size );
      if (!tmp) {
        PyErr_SetString( PyExc_RuntimeError, "getDoubleData() failed." );
        sipIsErr = 1;
      } else {
        sipRes = new vector<double>( tmp, tmp+size );
      }
%End

    string getStringData( const string& sname );
%MethodCode
      unsigned int size;
      const char* const tmp = (const char* const)sipCpp->getDataField( *a0, AR_CHAR, size );
      if (!tmp) {
        PyErr_SetString(PyExc_RuntimeError,"getStringData() failed");
        sipIsErr = 1;
      } else {
        sipRes = new string(tmp);
      }
%End

    /// Set the experiment name. This would normally be the name of the executable
    /// (minus .exe under windows). _Use this only in special cases_, e.g. when
    /// creating a demo that will reside in the same directory as an experiment.
    /// Must be called before init().
    bool setName( const string& name );
    /// Get the experiment name.
    string getName() const;
    /// Determine whether or not data will be saved. This would normally be read
    /// from SZG_EXPT/save_data, but if this has been called that parameter will
    /// be ignored. Use this only in special cases, e.g. when
    /// creating a demo that will reside in the same directory as an experiment.
    /// Must be called before init().
    bool saveData( bool yesno );
    /// Set the trial-generation method. This would normally be read from
    /// SZG_EXPT/method, but if this has been called that parameter will be ignored.
    /// Must be called before init().
    bool setTrialGenMethod( const string& method );
    bool setTrialGenMethod( arTrialGenerator* methodPtr );
    /// Add a data-saver. Must be called before init().
//    bool addDataSaver( const string& dsName, arDataSaver* dsPtr );
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
    //bool addSubjectParameter( const string theName, const arDataType theType );

    bool addLongSubjectParameter( const string& sname );
%MethodCode
      sipRes = sipCpp->addSubjectParameter( *a0, AR_LONG );
%End

    bool addDoubleSubjectParameter( const string& sname );
%MethodCode
      sipRes = sipCpp->addSubjectParameter( *a0, AR_DOUBLE );
%End

    bool addStringSubjectParameter( const string& sname );
%MethodCode
      sipRes = sipCpp->addSubjectParameter( *a0, AR_CHAR );
%End

    /// Get the value of a named subject parameter for the current human subject.
    //bool getSubjectParameter( const string theName, arDataType theType, void* address );

    long getLongSubjectParameter( const string& sname );
%MethodCode
      long* tmp(NULL);
      if (!sipCpp->getSubjectParameter( *a0, AR_LONG, (void*)tmp )) {
        PyErr_SetString(PyExc_ValueError,"getLongSubjectParameter() failed");
        sipIsErr = 1;
      } else {
        sipRes = *tmp;
      }
%End

    double getDoubleSubjectParameter( const string& sname );
%MethodCode
      double tmp;
      // Allow one exception because this one is already universally defined to be a float
      if (*a0 == "eye_spacing_cm") {
        float ftmp;
        if (!sipCpp->getSubjectParameter( *a0, AR_FLOAT, (void*)&ftmp )) {
          PyErr_SetString(PyExc_ValueError,"getDoubleSubjectParameter() failed");
          sipIsErr = 1;
        } else {
          sipRes = (double)ftmp;
        }
      } else {
        if (!sipCpp->getSubjectParameter( *a0, AR_DOUBLE, (void*)&tmp )) {
          PyErr_SetString(PyExc_ValueError,"getDoubleSubjectParameter() failed");
          sipIsErr = 1;
        } else {
          sipRes = tmp;
        }
      }
%End

   // bool getCharSubjectParameter( const string& theName, string& value );
    string getStringSubjectParameter( const string& theName );
%MethodCode
      string value;
      if (!sipCpp->getCharSubjectParameter( *a0, value )) {
        PyErr_SetString(PyExc_ValueError,"getStringSubjectParameter() failed");
        sipIsErr = 1;
      } else {
        sipRes = new string(value);
      }
%End
    
    /// Add or replace a trial phase object by name
    virtual bool setTrialPhase( const string theName, arExperimentTrialPhase* thePhase );
    /// Get the name of the current trial phase
    string currentTrialPhase();
    /// Activate a trial phase by name
    virtual bool activateTrialPhase( const string theName, arSZGAppFramework* fw );
    /// Calls update() method of active trial phase object
    virtual bool updateTrialPhase( arSZGAppFramework* fw );
    virtual bool updateTrialPhase( arSZGAppFramework* fw, arIOFilter* filter, arInputEvent& event );
    /// Allocate a named storage buffer
    /// If it has the same name and type as a data field, the data field
    /// will be set to point to the allocated buffer
    /// If it has the same name but a different type, it will fail.
//    void* const makeStorage( const string& name, arDataType typ, unsigned int size );
    /// Get a named storage buffer
//    void* const getStorage( const string& name, arDataType typ, unsigned int& size );
    /// Allocate storage for internally-stored data field
    //void* const allocateDataField( const string& name, arDataType typ, unsigned int size );
    //bool setDataFieldData( const string& name, arDataType typ, const void* const address, unsigned int size );

};


class arExperimentDataRecord  {
  public:
    arExperimentDataRecord( const string& name="" );
    ~arExperimentDataRecord();

    void setName( const string& name );
    string getName() const;

    unsigned int getNumberFields();
    vector<string> getFieldNames() const;

    bool fieldExists( const string& name );
//    bool fieldExists( const string& name, arDataType typ );

    bool addStringField( const string& name );
%MethodCode
      sipRes = sipCpp->addField( *a0, AR_CHAR );
%End

    bool addLongField( const string& name );
%MethodCode
      sipRes = sipCpp->addField( *a0, AR_LONG );
%End

    bool addDoubleField( const string& name );
%MethodCode
      sipRes = sipCpp->addField( *a0, AR_DOUBLE );
%End

    bool setStringField( const string& name, const string& val );
%MethodCode
      sipRes = sipCpp->setStringFieldValue( *a0, *a1 );
%End

    bool setLongField( const string& name, vector<long>& val );
%MethodCode
      long* tmp = new long[a1->size()];
      if (!tmp) {
        PyErr_SetString(PyExc_ValueError,"arExperimentDataRecord error: new in setLongField() failed.");
        sipIsErr = 1;
      } else {
        long* it = tmp;
        vector<long>::const_iterator iter;
        for (iter = a1->begin(); iter != a1->end(); ++iter) {
          *it++ = *iter;
        }
        bool stat = sipCpp->setFieldValue( *a0, AR_LONG, (const void* const)tmp, a1->size() );
        delete[] tmp;
        sipRes = stat;
      }
%End

    bool setDoubleField( const string& name, vector<double>& val );
%MethodCode
      double* tmp = new double[a1->size()];
      if (!tmp) {
        PyErr_SetString(PyExc_ValueError,"arExperimentDataRecord error: new in setDoubleField() failed.");
        sipIsErr = 1;
      } else {
        double* it = tmp;
        vector<double>::const_iterator iter;
        for (iter = a1->begin(); iter != a1->end(); ++iter) {
          *it++ = *iter;
        }
        bool stat = sipCpp->setFieldValue( *a0, AR_DOUBLE, (const void* const)tmp, a1->size() );
        delete[] tmp;
        sipRes = stat;
      }
%End

    string getStringField( const string& name );
%MethodCode
      string value;
      if (!sipCpp->getStringFieldValue( *a0, value )) {
        PyErr_SetString(PyExc_ValueError,"arExperimentDataRecord error: getStringFieldValue() failed.");
        sipIsErr = 1;
      } else {
        sipRes = new string(value);
      }
%End

    vector<long> getLongField( const string& name );
%MethodCode
      unsigned int size;
      long* tmp = (long*)sipCpp->getFieldAddress( *a0, AR_LONG, size );
      if (!tmp) {
        string msg = "arExperimentDataRecord error: getFieldAddress("+*a0+") failed.";
        PyErr_SetString(PyExc_ValueError,msg.c_str());
        sipIsErr = 1;
      } else {
        sipRes = new vector<long>( tmp, tmp+size );
      }    
%End

    vector<double> getDoubleField( const string& name );
%MethodCode
      unsigned int size;
      double* tmp = (double*)sipCpp->getFieldAddress( *a0, AR_DOUBLE, size );
      if (!tmp) {
        string msg = "arExperimentDataRecord error: getFieldAddress("+*a0+") failed.";
        PyErr_SetString(PyExc_ValueError,msg.c_str());
        sipIsErr = 1;
      } else {
        sipRes = new vector<double>( tmp, tmp+size );
      }    
%End
  private:
    arExperimentDataRecord( const arExperimentDataRecord& );
};


class arGluQuadric : arInteractableThing {
%TypeHeaderCode
#include "arGluQuadric.h"
%End
  public:
    arGluQuadric();
    arGluQuadric( const arGluQuadric& x );
    virtual ~arGluQuadric();
    void setPointStyle();
    void setLineStyle();
    void setSilhouetteStyle();
    void setFillStyle();
    void setNormalsOutside( bool trueFalse );
    void setNoNormals();
    void setFlatNormals();
    void setSmoothNormals();
    virtual void draw( arMasterSlaveFramework* fw=0 )  = 0;
};

class arGluSphere : arGluQuadric {
%TypeHeaderCode
#include "arGluSphere.h"
%End
  public:
    arGluSphere( double radius, int slices=30, int stacks=5 );
    arGluSphere( const arGluSphere& x );
    virtual ~arGluSphere();
    virtual void draw( arMasterSlaveFramework* fw=0 );
    void setRadius( double radius );
    void setSlicesStacks( int slices, int stacks );
};


class arGluCylinder : arGluQuadric {
%TypeHeaderCode
#include "arGluCylinder.h"
%End
  public:
    arGluCylinder( double startRadius, double endRadius, double length, int slices=30, int stacks=5 );
    arGluCylinder( const arGluCylinder& x );
    virtual ~arGluCylinder();
    virtual void draw( arMasterSlaveFramework* fw=0 );
    void setRadii( double startRadius, double endRadius );
    void setLength( double length );
    void setSlicesStacks( int slices, int stacks );
};


class arGluDisk : arGluQuadric {
%TypeHeaderCode
#include "arGluDisk.h"
%End
  public:
    arGluDisk( double innerRadius, double outerRadius, int slices=30, int rings=5 );
    arGluDisk( const arGluDisk& x );
    virtual ~arGluDisk();
    virtual void draw( arMasterSlaveFramework* fw=0 );
    void setRadii( double innerRadius, double outerRadius );
    void setSlicesRings( int slices, int rings );
};


class arPatternedBox : arInteractableThing {
%TypeHeaderCode
#include "arPatternedBox.h"
%End
  public:
    arPatternedBox();
    virtual ~arPatternedBox();
    arPatternedBox( const arPatternedBox& b );
    void setUseTexture( bool useTexture );
    bool getUseTexture() const;
    void setSize( const float w, const float h, const float d, const float s=0 );
    virtual void draw( arMasterSlaveFramework* fw=0 );
    void setTextureScale( float sc );
};


class arGluTessObject : arInteractableThing {
%TypeHeaderCode
#include "arGluTessObject.h"
%End
  public:
    arGluTessObject( bool useDisplayList = false );
    arGluTessObject( const arGluTessObject& x );
    virtual ~arGluTessObject();
    void setScaleFactors( const arVector3& scales );
    void setScaleFactors( float x, float y, float z );
    arVector3 getScaleFactors() /TransferBack/;
    void setTextureScales( float sScale, float tScale );
    void setTextureOffsets( float sOffset, float tOffset );
    void useDisplayList( bool use );
    bool buildDisplayList();
    virtual void draw();
    void addContour( vector<arVector3>& vertexSequence  );
};



