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
  long theLong;
  int theInt;
  if (!ar_stringToLongValid( s, theLong ) ||
      !ar_longToIntValid( theLong, theInt ) ||
      theInt < 0) {
    return false;
  }
  eventIndex = (unsigned int) theInt;
  return true;
}

// determines whether word is a string of the form
// filter_<event type>_<number>
struct IsFilterWord {
  bool operator()( const string theWord ) {
    arInputEventType eventType;
    unsigned int eventIndex;
    return ar_parsePForthFilterWord( theWord, eventType, eventIndex );
  }
};
    
bool arPForthFilter::configure( arSZGClient* SZGClient ) {
  if (!_pforth) {
    cerr << "failed to initialize PForth.\n";
    return false;
  }
  
  ar_PForthSetSZGClient( SZGClient );

  // Load the programs.
  const arSlashString programList(SZGClient->getAttribute("SZG_PFORTH","program_names"));
  if (programList == "NULL") {
    cout << _progName << " remark: no filter program set.\n";
    return true;
  }

  const std::string dataPath(SZGClient->getAttribute("SZG_DATA","path"));
  if (dataPath == "NULL") {
    cout << _progName << " remark: no data path set.\n";
    return true;
  }

  const string programName(programList[_progNumber]);
  if (programName == "") {
    cerr << _progName
         << " warning: SZG_PFORTH/program_names program index out of bounds.\n";
    return false;
  }

  FILE* fp = ar_fileOpen( programName+".pf", "PForth", dataPath, "rt" );
  if (!fp) {
    cerr << "arPForthFilter error: failed to open file " << programName << ".pf in "
         << dataPath << ar_pathDelimiter() << "PForthPrograms" << endl;
    return false;
  }
  char buf[10000];
  int numRead = fread( buf, sizeof( char ), 9999, fp );
  buf[numRead] = '\0';
  fclose(fp);
  if (ferror(fp)) {
    cerr << "arPForthFilter error: failed to read file " << programName << ".pf in "
         << dataPath << ar_pathDelimiter() << "PForthPrograms" << endl;
    return false;
  } 
  if (numRead == 9999 && !feof(fp)) {
    cerr << "arPForthFilter error: current filter program limit is 9999 chars.\n"
         << "   Change this in arPForthFilter::configure().\n";
    return false;
  }
  std::string progText( buf );
  
//  arDataTemplate progfileTemplate( "PForth" );
//  progfileTemplate.addAttribute( "name", AR_CHAR );
//  progfileTemplate.addAttribute( "program", AR_CHAR );
//  arTemplateDictionary progfileDict;
//  progfileDict.add( &progfileTemplate );
//  arStructuredDataParser progParser( &progfileDict );
//  
//  string dataPath(SZGClient->getAttribute("SZG_DATA","path"));
//  ar_pathAddSlash( dataPath );
//  const string fileName("PForth_programs.xml");
//  arFileTextStream fileStream;
//  if (!fileStream.ar_open(fileName,"",dataPath)) {
//    cerr << _progName << " warning: failed to open file \""
//         << fileName << "\" in path \"" << dataPath << "\".\n";
//    return false;
//  }

//  arStructuredData* progData = NULL;
//  bool found = false;
//  string progName, progText;
//  while (!found && (progData = progParser.parse(&fileStream)) != 0) {
//    if (!ar_getStructuredDataStringField( progData, "name", progName )) {
//      cerr << _progName << " warning: failed to read <name> field.\n";
//      progParser.recycle( progData );
//      fileStream.ar_close();
//      return false;
//    }

//    if (progName == programName) {
//      if (!ar_getStructuredDataStringField( progData, "program", progText )) {
//        cerr << _progName << " warning: failed to read <program> field.\n";
//        progParser.recycle( progData );
//        fileStream.ar_close();
//        return false;
//      }
//      found = true;
//    }
//    progParser.recycle( progData );
//  }
//  fileStream.ar_close();
//  if (!found) {
//    cerr << _progName << " warning: failed to find program " << programName
//         << " in " << fileName << ".\n";
//    return false;
//  }  
  if (!_pforth.compileProgram( progText )) {
    cerr << _progName << " warning: failed to compile " 
         << programName << " with text:\n\n" << progText << endl << "--------------------" << endl;
    return false;
  }
  if (!_pforth.runProgram()) {
    cerr << _progName << " warning: failed to run " << programName << ".\n";
    return false;
  }

  cout << _progName << " remark: ran " << programName << endl;
  
  vector<string> words = _pforth.getVocabulary();
  vector<string>::const_iterator iter;
  arInputEventType eventType;
  unsigned int eventIndex;
  for (iter = words.begin(); iter != words.end(); ++iter) {

    std::string word( *iter );
    if (word.substr(0,7) == "filter_") {
      if (!_pforth.compileProgram( word )) {
        cerr << _progName << " warning: failed to compile word "
	     << word << endl;
        continue;
      }
      arPForthProgram* prog;
      if (!(prog = _pforth.getProgram())) {
        cerr << _progName << " warning: failed to export program.\n";
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
        //cerr << word << " " << eventType << " " << eventIndex << endl;
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
            cerr << _progName << " warning: ignoring invalid event type.\n";
        }
      }
    }
  }
  _valid = true;
  return true;
}

// Return false iff program exists, yet fails to run.
bool arPForthFilter::_processEvent( arInputEvent& inputEvent ) {
  if (!_valid)
    return true;

  ar_PForthSetInputEvent( &inputEvent );
  const unsigned int i = inputEvent.getIndex();

  static std::vector< arPForthProgram* > programs;
  programs.clear();
  const int eventType = inputEvent.getType();
  switch (eventType) {
    case AR_EVENT_BUTTON:
      if (i < _buttonFilterPrograms.size())
        if (_buttonFilterPrograms[i])
          programs.push_back( _buttonFilterPrograms[i] );
      if (_allButtonsFilterProgram)
        programs.push_back( _allButtonsFilterProgram );
      break;
    case AR_EVENT_AXIS:
      if (i < _axisFilterPrograms.size())
        if (_axisFilterPrograms[i])
          programs.push_back( _axisFilterPrograms[i] );
      if (_allAxesFilterProgram)
        programs.push_back( _allAxesFilterProgram );
      break;
    case AR_EVENT_MATRIX:
      if (i < _matrixFilterPrograms.size())
        if (_matrixFilterPrograms[i])
          programs.push_back( _matrixFilterPrograms[i] );
      if (_allMatricesFilterProgram)
        programs.push_back( _allMatricesFilterProgram );
      break;
    default:
      cerr << _progName << " warning: ignoring event with unexpected type "
           << eventType << endl;
      return true;
  }
  if (_allEventsFilterProgram) {
    programs.push_back( _allEventsFilterProgram );
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
