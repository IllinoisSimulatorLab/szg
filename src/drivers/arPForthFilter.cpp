//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arPForthFilter.h"
#include "arFileTextStream.h"

arFilteringPForth::arFilteringPForth() {
  _valid &= ar_PForthAddEventVocabulary(this) && ar_PForthAddDatabaseVocabulary(this);
}

arFilteringPForth::~arFilteringPForth() {
}

arPForthFilter::arPForthFilter( const unsigned progNumber ) :
    _progNumber( progNumber ),
    _progName("arPForthFilter(" + ar_intToString(_progNumber) + ")"),
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
                               unsigned& eventIndex ) {
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
  eventIndex = (unsigned) theInt;
  return true;
}

bool arPForthFilter::loadProgram(const string& progText) {
  if (!_pforth) {
    ar_log_error() << "arPForthFilter failed to initialize.\n";
    return false;
  }

  stringstream parsingStream(progText);
  string firstToken;
  parsingStream >> firstToken;
  if (firstToken == "") {
    // Empty or missing filter program.
    return true;
  }

  ar_log_remark() << "arPForthFilter program text = \n" << progText << "\n";
  if (!_pforth.compileProgram( progText )) {
    ar_log_error() << "arPForthFilter failed to compile program:\n" << progText << "\n\n";
    return false;
  }
  if (!_pforth.runProgram()) {
    ar_log_error() << "arPForthFilter failed to run program.\n";
    return false;
  }

  vector<string> words = _pforth.getVocabulary();
  vector<string>::const_iterator iter;
  arInputEventType eventType;
  unsigned eventIndex = 0;
  for (iter = words.begin(); iter != words.end(); ++iter) {

    std::string word( *iter );
    if (word.substr(0, 7) == "filter_") {
      if (!_pforth.compileProgram( word )) {
        ar_log_error() << "arPForthFilter failed to compile word " << word << "\n";
        continue;
      }
      arPForthProgram* prog = _pforth.getProgram();
      if (!prog) {
        ar_log_error() << "arPForthFilter failed to export program.\n";
        continue;
      }
      if (word == "filter_all_events") {
        _allEventsFilterProgram = prog;
      }
      else if (word == "filter_all_buttons") {
        _allButtonsFilterProgram = prog;
      }
      else if (word == "filter_all_axes") {
        _allAxesFilterProgram = prog;
      }
      else if (word == "filter_all_matrices") {
        _allMatricesFilterProgram = prog;
      }
      else if (ar_parsePForthFilterWord( word, eventType, eventIndex )) {
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
            ar_log_error() << "arPForthFilter ignoring invalid event type " <<
              eventType << ".\n";
        }
      }
    }
  }
  _valid = true;
  return true;
}

// Return false iff program exists, yet fails to run.
//
// CHANGE WARNING! I originally set it up so that specific filters
// (e.g. filter_button_0) were run _before_ general ones (e.g. filter_all_buttons).
// I have since decided this was bad, and have reversed the order.
// Now filter_all_events comes _first_, followed by e.g. filter_all_buttons, followed by
// e.g. filter_button_0.
bool arPForthFilter::_processEvent( arInputEvent& inputEvent ) {
  if (!_valid)
    return true;

  ar_PForthSetInputEvent( &inputEvent );
  const unsigned i = inputEvent.getIndex();

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
      ar_log_error() << "PForth program " << _progName <<
        " ignoring event with unexpected type " << eventType << "\n";
      return true;
  }
  std::vector< arPForthProgram* >::iterator iter;
  for (iter = programs.begin(); iter != programs.end(); ++iter) {
    if (!*iter) {
      ar_log_error() << "arPForthFilter: NULL program pointer.\n";
      return false;
    }
    if (!_pforth.runProgram( *iter )) {
      ar_log_error() << "arPForthFilter failed to run program.\n";
      return false;
    }
  }
  return true;
}
