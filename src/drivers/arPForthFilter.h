//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef ARPFORTHFILTER_H
#define ARPFORTHFILTER_H

#include "arIOFilter.h"
#include "arPForth.h"
#include "arPForthEventVocabulary.h"
#include "arPForthDatabaseVocabulary.h"

class SZG_CALL arFilteringPForth : public arPForth {
  public:
    arFilteringPForth() :
      arPForth()  {
      _valid = _valid && arPForthSpace::ar_PForthAddEventVocabulary(this)
                      && arPForthSpace::ar_PForthAddDatabaseVocabulary(this);
    }
};

/// Filter that uses the arPForth FORTH interpreter.

class SZG_CALL arPForthFilter: public arIOFilter {
  public:
    arPForthFilter( const unsigned int progNumber = 0 );
    ~arPForthFilter();
  
    bool configure( arSZGClient* client );
  protected:
    bool _processEvent( arInputEvent& inputEvent );
  private:
    unsigned int _progNumber;
    string _progName;
    bool _valid;
    arFilteringPForth _pforth;
    arPForthProgram* _allEventsFilterProgram;
    arPForthProgram* _allButtonsFilterProgram;
    arPForthProgram* _allAxesFilterProgram;
    arPForthProgram* _allMatricesFilterProgram;
    std::vector<arPForthProgram*> _buttonFilterPrograms;
    std::vector<arPForthProgram*> _axisFilterPrograms;
    std::vector<arPForthProgram*> _matrixFilterPrograms;
};

#endif        //  #ifndefARPFORTHFILTER_H
