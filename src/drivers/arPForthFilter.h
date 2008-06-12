//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef ARPFORTHFILTER_H
#define ARPFORTHFILTER_H

#include "arIOFilter.h"
#include "arPForth.h"
#include "arPForthEventVocabulary.h"
#include "arPForthDatabaseVocabulary.h"
#include "arDriversCalling.h"

class SZG_CALL arFilteringPForth : public arPForth {
 public:
  arFilteringPForth();
  ~arFilteringPForth();
};

// Filter that uses the arPForth FORTH interpreter.

class SZG_CALL arPForthFilter: public arIOFilter {
  public:
    arPForthFilter(const unsigned int progNumber = 0);
    ~arPForthFilter();
    bool loadProgram(const string& progText);
  protected:
    bool _processEvent(arInputEvent&);
  private:
    const unsigned _progNumber;
    const string _progName;
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

#endif
