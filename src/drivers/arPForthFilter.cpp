//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arPForthFilter.h"
#include "arFileTextStream.h"
#include <sstream>

arFilteringPForth::arFilteringPForth(){
  _valid = _valid && ar_PForthAddEventVocabulary(this)
                  && ar_PForthAddDatabaseVocabulary(this);
}

arFilteringPForth::~arFilteringPForth(){
}

arPForthFilter::arPForthFilter( const unsigned int progNumber ) :
    _progNumber( progNumber ),
    _valid(false),
    _allEventsFilterProgram(0),
    _allButtonsFilterProgram(0),
    _allAxesFilterProgram(0),
    _allMatricesFilterProgram(0)
{
  _buttonFilterPrograms.reserve(256);
  _axisFilterPrograms.reserve(256);
  _matrixFilterPrograms.reserve(256);
  ar_PForthSetFilter(this);

  // Simplest way of inserting a number into a string, sigh.
  std::stringstream s;
  s <<  "arPForthFilter(" << _progNumber << ")";
  s >> _progName;
}

arPForthFilter::~arPForthFilter() {
  if (_allEventsFilterProgram)
    delete _allEventsFilterProgram;
  if (_allButtonsFilterProgram)
    delete _allButtonsFilterProgram;
  if (_allAxesFilterProgram)
    delete _allAxesFilterProgram;
  if (_allMatricesFilterProgram)
    delete _allMatricesFilterProgram;
  std::vector<arPForthProgram*>::iterator iter;
  for (iter = _buttonFilterPrograms.begin(); iter != _buttonFilterPrograms.end(); ++iter)
    if (*iter)
      delete *iter;
  for (iter = _axisFilterPrograms.begin(); iter != _axisFilterPrograms.end(); ++iter)
    if (*iter)
      delete *iter;
  for (iter = _matrixFilterPrograms.begin(); iter != _matrixFilterPrograms.end(); ++iter)
    if (*iter)
      delete *iter;
  _buttonFilterPrograms.clear();
  _axisFilterPrograms.clear();
  _matrixFilterPrograms.clear();
}

bool ar_parsePForthFilterWord( const string& theWord,
                               arInputEventType& eventType,
                               unsigned int& eventIndex ) {
  std::istringstream wordStream( theWord );
  string s;
  getline( wordStream, s, '_' ); // prefix
  if (s != "filter")
    return false;

  getline( wordStream, s, '_' ); // type
  if (s == "button")
    eventType = AR_EVENT_BUTTON;
  else if (s == "axis")
    eventType = AR_EVENT_AXIS;
  else if (s == "matrix")
    eventType = AR_EVENT_MATRIX;
  else
    return false;

  getline( wordStream, s ); // number
  long theLong = -1;
  int theInt = -1;
  if (!ar_stringToLongValid( s, theLong ) ||
      !ar_longToIntValid( theLong, theInt ) ||
      theInt < 0) {
    return false;
  }
  eventIndex = (unsigned int) theInt;
  return true;
}

bool arPForthFilter::configure(const string& progText ) {
  if (!_pforth) {
    cerr << "arPForthFilter error: failed to initialize.\n";
    return false;
  }

  // Make sure the program is not empty... If it is, print a message and
  // return true.
  stringstream parsingStream(progText);
  string firstToken;
  parsingStream >> firstToken;
  if (firstToken == ""){
    // cout << "arPForthFilter remark: no filter program set.\n";
    return true;
  }

  cout << "arPForthFilter remark: program text = \n" << progText << endl;
  if (!_pforth.compileProgram( progText )) {
    cerr << "arPForthFilter warning: failed to compile program:\n"
         << progText << "\n\n";
    return false;
  }
  if (!_pforth.runProgram()) {
    cerr << "arPForthFilter warning: failed to run program.\n";
    return false;
  }

  cout << "arPForthFilter remark: ran program." << endl;
  
  vector<string> words = _pforth.getVocabulary();
  vector<string>::const_iterator iter;
  arInputEventType eventType;
  unsigned int eventIndex;
  for (iter = words.begin(); iter != words.end(); ++iter) {

    std::string word( *iter );
    if (word.substr(0,7) == "filter_") {
      if (!_pforth.compileProgram( word )) {
        cerr << "arPForthFilter warning: failed to compile word "
	     << word << endl;
        continue;
      }
      arPForthProgram* prog = _pforth.getProgram();
      if (!prog) {
        cerr << "arPForthFilter warning: failed to export program.\n";
        continue;
      }
      if (word == "filter_all_events") {
        _allEventsFilterProgram = prog;
      } else if (word == "filter_all_buttons") {
        _allButtonsFilterProgram = prog;
      } else if (word == "filter_all_axes") {
        _allAxesFilterProgram = prog;
      } else if (word == "filter_all_matrices") {
        _allMatricesFilterProgram = prog;
      } else if (ar_parsePForthFilterWord( word, eventType, eventIndex )) {
        switch (eventType) {
          case AR_EVENT_BUTTON:
            if (eventIndex >= _buttonFilterPrograms.size())
              _buttonFilterPrograms.insert( _buttonFilterPrograms.end(),
                           eventIndex-_buttonFilterPrograms.size()+1, 0 );
            _buttonFilterPrograms[eventIndex] = prog;
            break;
          case AR_EVENT_AXIS:
            if (eventIndex >= _axisFilterPrograms.size())
              _axisFilterPrograms.insert( _axisFilterPrograms.end(),
                           eventIndex-_axisFilterPrograms.size()+1, 0 );
            _axisFilterPrograms[eventIndex] = prog;
            break;
          case AR_EVENT_MATRIX:
            if (eventIndex >= _matrixFilterPrograms.size())
              _matrixFilterPrograms.insert( _matrixFilterPrograms.end(),
                           eventIndex-_matrixFilterPrograms.size()+1, 0 );
            _matrixFilterPrograms[eventIndex] = prog;
            break;
          default:
            cerr << "arPForthFilter warning: ignoring invalid event type.\n";
        }
      }
    }
  }
  _valid = true;
  return true;
}

// Return false iff program exists, yet fails to run.
//CHANGE WARNING! I originally set it up so that the more specific filters
//(e.g. filter_button_0) were run _before_ the more generic ones (e.g. filter_all_buttons).
//I have since decided this was a bad thing to do, and have reversed the order.
//Now filter_all_events comes _first_, followed by e.g. filter_all_buttons, followed by
//e.g. filter_button_0.
bool arPForthFilter::_processEvent( arInputEvent& inputEvent ) {
  if (!_valid)
    return true;

  ar_PForthSetInputEvent( &inputEvent );
  const unsigned int i = inputEvent.getIndex();

  static std::vector< arPForthProgram* > programs;
  programs.clear();
  if (_allEventsFilterProgram) {
    programs.push_back( _allEventsFilterProgram );
  }
  const int eventType = inputEvent.getType();
  switch (eventType) {
    case AR_EVENT_BUTTON:
      if (_allButtonsFilterProgram)
        programs.push_back( _allButtonsFilterProgram );
      if (i < _buttonFilterPrograms.size())
        if (_buttonFilterPrograms[i])
          programs.push_back( _buttonFilterPrograms[i] );
      break;
    case AR_EVENT_AXIS:
      if (_allAxesFilterProgram)
        programs.push_back( _allAxesFilterProgram );
      if (i < _axisFilterPrograms.size())
        if (_axisFilterPrograms[i])
          programs.push_back( _axisFilterPrograms[i] );
      break;
    case AR_EVENT_MATRIX:
      if (_allMatricesFilterProgram)
        programs.push_back( _allMatricesFilterProgram );
      if (i < _matrixFilterPrograms.size())
        if (_matrixFilterPrograms[i])
          programs.push_back( _matrixFilterPrograms[i] );
      break;
    default:
      cerr << _progName << " warning: ignoring event with unexpected type "
           << eventType << endl;
      return true;
  }
  std::vector< arPForthProgram* >::iterator iter;
  for (iter = programs.begin(); iter != programs.end(); ++iter) {
    if (!*iter) {
      cerr << "arPForthFilter error: NULL program pointer.\n";
      return false;
    }
    if (!_pforth.runProgram( *iter )) {
      cerr << "arPForthFilter error: failed to run program.\n";
      return false;
    }
  }
  return true;
}
