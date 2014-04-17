#ifndef AREXPERIMENTDATAFIELD_H
#define AREXPERIMENTDATAFIELD_H

#include <string>
#include <map>
#include <set>
#include <ostream>
#include "arDataType.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

typedef std::set< std::string > arStringSet_t;
typedef std::map< std::string,arStringSet_t > arStringSetMap_t;

class SZG_CALL arExperimentDataField {
  friend std::ostream& operator<<(std::ostream&, const arExperimentDataField&);
  public:
    arExperimentDataField( const std::string name, const arDataType typ,
                           const void* ad=0, const unsigned int s=1,
                           bool ownPtr = false );
    arExperimentDataField( const arExperimentDataField& x );
    arExperimentDataField& operator=( const arExperimentDataField& x );
    ~arExperimentDataField();
    bool operator==( const arExperimentDataField& x );
    bool operator!=( const arExperimentDataField& x );
    const std::string getName() const { return _name; }
    const arDataType getType() const { return _type; }
    const unsigned int getSize() const { return _size; }
    void* const getAddress() const { return _address; }
    bool selfOwned() const { return _ownPtr; }
    // Note that makeStorage() returns a valid pointer on success even if size == 0
    void* const makeStorage( unsigned int size );
    bool setData( arDataType typ, const void* const address, unsigned int size );
    bool sameNameType( const arExperimentDataField& x );
    static void setFloatCompPrecision( double x );
  private:
    std::string _name;
    arDataType _type;
    void* _address;
    unsigned int _size;
    // this is here so that we can return a valid data pointer even when the
    // user requests a data field of size 0. _dataSize in this case gets set
    // to 1 (it's invisible to the user) and used as the allocation size.
    unsigned int _dataSize;
    bool _ownPtr;
//    static double _floatCompPrecision;
};

std::ostream& SZG_CALL operator<<(std::ostream& s, const arExperimentDataField& d);

typedef std::map< std::string, arExperimentDataField > arNameDataMap_t;

#endif        //  #ifndefAREXPERIMENTDATAFIELD_H

