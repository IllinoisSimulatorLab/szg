//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arPForth.h"
#include "arStructuredData.h"

using std::vector;
using std::string;
using std::istringstream;

// Behaviors declared in arPForth.h

namespace arPForthSpace {

// Run-time behaviors

bool FetchNumber::run( arPForth* pf ) {
  if (!pf)
    return false;
  pf->stackPush( _value );
  return true;
}

// Compile-time behaviors

bool SimpleCompiler::compile( arPForth* pf,
                              vector<arPForthSpace::arPForthAction*>& actionList ) {
  if (!pf)
    return false;
  actionList.push_back( _action );
  return true;
}

bool VariableCompiler::compile( arPForth* pf,
                               vector<arPForthSpace::arPForthAction*>& /*actionList*/ ) {
  if (!pf)
    return false;
  const string theName = pf->nextWord();
  if (theName == "PFORTH_NULL_WORD")
    throw arPForthException("end of input reached prematurely.");

  if (pf->findWord( theName ))
    throw arPForthException("word " + theName + " already in dictionary.");

  const unsigned long address = pf->allocateStorage( _size ); // oops, needs a test
  arPForthAction* action = new FetchNumber( (float)address );
  pf->addAction( action );
  arPForthCompiler* compiler = new SimpleCompiler( action );
  pf->addCompiler( compiler );
  return pf->addDictionaryEntry( arPForthDictionaryEntry( theName, compiler ));
}

bool ArrayCompiler::compile( arPForth* pf,
                               vector<arPForthSpace::arPForthAction*>& /*actionList*/ ) {
  if (!pf)
    return false;
  const string numItemsString = pf->nextWord();
  const string theName = pf->nextWord();
  if ((numItemsString == "PFORTH_NULL_WORD")||(theName == "PFORTH_NULL_WORD"))
    throw arPForthException("end of input reached prematurely.");

  long numItemsSigned = -1;
  if (!ar_stringToLongValid( numItemsString, numItemsSigned ))
    throw arPForthException("string->numItems conversion failed.");

  if (pf->findWord( theName ))
    throw arPForthException("word " + theName + " already in dictionary.");

  unsigned long numItems( static_cast<unsigned long>(numItemsSigned) );
  unsigned long address = pf->allocateStorage( numItems ); // oops, needs a test
  arPForthAction* action = new FetchNumber( (float)address );
  pf->addAction( action );
  arPForthCompiler* compiler = new SimpleCompiler( action );
  pf->addCompiler( compiler );
  return pf->addDictionaryEntry( arPForthDictionaryEntry( theName, compiler ));
}

bool NumberCompiler::compile( arPForth* pf,
                             vector<arPForthSpace::arPForthAction*>& actionList ) {
  if (!pf)
    return false;
  double theDouble = 0.;
  float theFloat = 0.;
  if (!ar_stringToDoubleValid( _theWord, theDouble ))
    throw arPForthException("string->float conversion failed.");

  if (!ar_doubleToFloatValid( theDouble, theFloat ))
    throw arPForthException("string->float conversion failed.");

  arPForthAction* action = new FetchNumber(theFloat);
  if (pf->anonymousActionsAreTransient)
    pf->addTransientAction( action );
  else
    pf->addAction( action );
  actionList.push_back( action );
  return true;
}

bool PlaceHolderCompiler::compile( arPForth* pf,
                vector<arPForthSpace::arPForthAction*>& /*actionList*/ ) {
  if (!pf)
    return false;
  throw arPForthException("syntax error involving " + _theWord + ".");
  return true;
}

//struct ConstantCompile : public arPForthCompiler {
//  ConstantCompile() {}
//  bool compile( arPForth* pf, const string theWord );
//}
//bool ConstantCompile::compile( arPForth* pf, const string theWord ) {
//  if (!pf)
//    return false;
//  const string theName = pf->nextWord();
//  if (!pf->findWord( theName ))
//    return false;
//  if (pf->_wordIndex( theName )!=-1)
//    return false;
//  unsigned long address = -1;
//  if ((address = pf->_allocateStorage( _size )) < 0)
//    return false;
//  if (!pf->addDictionaryEntry( theName, FetchAddress( address ) ))
//    return false;
//  return true;
//}

} // namespace arPForthSpace

arPForthProgram::~arPForthProgram() {
  std::vector< arPForthSpace::arPForthAction* >::iterator iter;
  for (iter = _transientActions.begin(); iter != _transientActions.end(); iter++)
    delete *iter;
  _transientActions.clear();
  _actionList.clear();
}
  
arPForth::arPForth():
  anonymousActionsAreTransient(true),
  _program(0) {
  _valid = arPForthSpace::ar_PForthAddStandardVocabulary(this);
}

arPForth::~arPForth() {
  _inputWords.clear();
  _theStack.clear();
  _dataSpace.clear();
  if (_program != 0)
    delete _program;
  _dictionary.clear();
  unsigned i = 0;
  for (i=0; i<_actions.size(); i++)
    delete _actions[i];
  _actions.clear();
  for (i=0; i<_compilers.size(); i++)
    delete _compilers[i];
  _compilers.clear();
}

bool arPForth::addDictionaryEntry( const arPForthSpace::arPForthDictionaryEntry& entry ) {
  if (findWord( entry._word )) {
    cerr << "arPForth error: " << entry._word << " already in dictionary.\n";
    return false;
  }
  _dictionary.push_back( entry );
  return true;
}

void arPForth::addCompiler( arPForthSpace::arPForthCompiler* compiler ) {
  _compilers.push_back( compiler );
}

void arPForth::addAction( arPForthSpace::arPForthAction* action ) {
  _actions.push_back( action );
}

void arPForth::addTransientAction( arPForthSpace::arPForthAction* action ) {
  if (_program == 0) {
    cerr << "arPForth error: no program to add triansient action to.\n";
    return;
  }
  _program->_transientActions.push_back( action );
}

bool arPForth::addSimpleActionWord(
  const string& theWord, arPForthSpace::arPForthAction* action ) {
  if (!action)
    return false;
  addAction( action );
  arPForthSpace::arPForthCompiler* compiler = new arPForthSpace::SimpleCompiler(action);
  if (!compiler)
    return false;
  addCompiler( compiler );
  return addDictionaryEntry( arPForthSpace::arPForthDictionaryEntry( theWord, compiler ) );
}
  
float arPForth::stackPop() {
  if (_theStack.empty())
    throw arPForthSpace::arPForthException("stack underflow.");

  float val = _theStack.back();
  _theStack.pop_back();
  return val;
}

void arPForth::stackPush( const float val ) {
  _theStack.push_back( val );
}

unsigned arPForth::stackSize() const {
  return _theStack.size();
}

// Cheat.
float arPForth::stackElement( const unsigned i ) {
  if (i >= _theStack.size())
    throw arPForthSpace::arPForthException("invalid stack access.");
  return _theStack[i];
}

// Cheat.
bool arPForth::insertStackElement( const unsigned i, const float val ) {
  if (i > _theStack.size())
    return false;
  _theStack.insert( _theStack.begin() + i, val );
  return true;
}

bool arPForth::findWord( string theWord ) {
  vector<arPForthSpace::arPForthDictionaryEntry>::const_iterator dummy;
  return getWord( theWord, dummy );
}

bool arPForth::getWord ( const string& theWord,
          vector<arPForthSpace::arPForthDictionaryEntry>::const_iterator& iter ) const {
  for (iter=_dictionary.begin(); iter!=_dictionary.end(); ++iter)
    if (iter->_word == theWord)
      return true;
  return false;
}

void arPForth::testFailAddress( const long address, unsigned long size ) {
  if ((address < 0)||(address+size-1 > _dataSpace.size()-1))
    throw arPForthSpace::arPForthException("invalid address.");
}

float arPForth::getDataValue( long address ) {
  testFailAddress( address, 1 );
  return _dataSpace[address];
}

void arPForth::putDataValue( long address, const float val ) {
  testFailAddress( address, 1 );
  _dataSpace[address] = val;  
}

arMatrix4 arPForth::getDataMatrix( long address ) {
  testFailAddress( address, 16 );
  return arMatrix4( &_dataSpace[address] );
}

void arPForth::putDataMatrix( long address, const arMatrix4& val ) {
  testFailAddress( address, 16 );
  memcpy( &_dataSpace[address], val.v, 16*sizeof(float));
}

void arPForth::getDataArray( long address, float* const ptr, unsigned long size ) {
  testFailAddress( address, size );
  if (!ptr)
    throw arPForthSpace::arPForthException("NULL pointer.");
  memcpy( ptr, &_dataSpace[address], size*sizeof(float) );
}

void arPForth::putDataArray( long address, const float* const ptr, unsigned long size ) {
  testFailAddress( address, size );
  if (!ptr)
    throw arPForthSpace::arPForthException("NULL pointer.");
  memcpy( &_dataSpace[address], ptr, size*sizeof(float) );
}

std::string arPForth::getString( long address ) {
  if ((address < 0)||(address > (long)_stringSpace.size()-1))
    throw arPForthSpace::arPForthException("invalid string address.");
  return _stringSpace[address];
}

void arPForth::putString( long address, const std::string& val ) {
  if ((address < 0)||(address > (long)_stringSpace.size()-1))
    throw arPForthSpace::arPForthException("invalid string address.");
  _stringSpace[address] = val;
}

unsigned long arPForth::allocateStorage( const unsigned long size ) {
  unsigned long newAddress = _dataSpace.size()+1;
  _dataSpace.insert( _dataSpace.end(), size+1, 0. );
  _dataSpace[newAddress-1] = (float)size;
  return newAddress;
}

unsigned long arPForth::allocateString() {
  _stringSpace.push_back("");
  return _stringSpace.size()-1;
}

string arPForth::nextWord() {
  if (_inputWords.empty())
    return "PFORTH_NULL_WORD";
  const string s = _inputWords.front();
  _inputWords.pop_front();
  return s;
}

string arPForth::peekNextWord() const {
  return _inputWords.empty() ? "PFORTH_NULL_WORD" : _inputWords.front();
}

void arPForth::printDataspace() const {
  cerr << "Data space:\n";
  for (vector<float>::const_iterator i = _dataSpace.begin(); i != _dataSpace.end(); ++i)
    cerr << "[ " << *i << " ]";
  cerr << "\n";
}

void arPForth::printStringspace() const {
  cerr << "String space:\n";
  for (vector<string>::const_iterator i = _stringSpace.begin(); i != _stringSpace.end(); ++i)
    cerr << "[ " << *i << " ]";
  cerr << "\n";
}

void arPForth::printStack() const {
    cerr << "Stack:\n";
    for (unsigned i=0; i<_theStack.size(); i++)
      cerr << "  [ " << _theStack[i] << " ]\n";
}

void arPForth::printDictionary() const {
  cerr << "Dictionary:\n";
  for (unsigned i=0; i<_dictionary.size(); i++) {
    if (i > 0)
      cerr << ", ";
    cerr << _dictionary[i]._word;
  }
  cerr << "\n";
}

bool arPForth::compileWord( const string theWord,
                            vector<arPForthSpace::arPForthAction*>& actionList ) {
  const unsigned char temp = (unsigned char)*theWord.c_str();
  vector<arPForthSpace::arPForthDictionaryEntry>::const_iterator iter;
  if (getWord( theWord, iter )) {
    // found in dictionary
    if (!iter->_compiler->compile( this, actionList ))
      return false;
  } else if (isdigit(temp)||(temp == '-')||(temp == '.')) {
    if (!arPForthSpace::NumberCompiler(theWord).compile( this, actionList ))
      return false;
  } else {
    throw arPForthSpace::arPForthException("dictionary has no word " + theWord + ".");
  }
  return true;
}

bool arPForth::compileProgram( const string sourceCode ) {
  try {
    anonymousActionsAreTransient = true;
    _inputWords.clear();
    if (_program)
      delete _program;
    _program = new arPForthProgram;
    if (!_program) {
      cerr << "arPForth error: failed to allocate program memory.\n";
      return false;
    }
//    _program.clear();
//    _transientActions.clear();
    istringstream newStream( sourceCode );
    string wordString;
    while (newStream >> wordString)
      _inputWords.push_back( wordString );
    string theWord;
    while ((theWord = nextWord()) != "PFORTH_NULL_WORD")
      if (!compileWord( theWord, _program->_actionList ))
        return false;
    return true;
  }
  catch (arPForthSpace::arPForthException ce) {
    cerr << "arPForth error: " << ce._message << "\n";
//    _program.clear();
    if (_program) {
      delete _program;
      _program = NULL;
    }
    anonymousActionsAreTransient = true;

    printDictionary();
    printDataspace();
    printStack();
    return false;
  }
}

// Hand off the _program pointer and relinquish responsibility for it.
arPForthProgram* arPForth::getProgram() {
  arPForthProgram* temp = _program;
  _program = NULL;
  return temp;
}

bool arPForth::runSubprogram( vector<arPForthSpace::arPForthAction*>& actionList ) {
  vector<arPForthSpace::arPForthAction*>::iterator iter;
  for (iter = actionList.begin(); iter != actionList.end(); iter++)
    if (!(*iter)->run( this ))
      return false;
  return true;
}

bool arPForth::runProgram() {
  try {
    if (!_program) {
      cerr << "arPForth error: internal program == Nil.\n";
      return false;
    }
    return runSubprogram( _program->_actionList );
   }
  catch (arPForthSpace::arPForthException ce) {
    cerr << "arPForth error: " << ce._message << "\n";
    printDataspace();
    printStack();
    return false;
  }
}

bool arPForth::runProgram(  arPForthProgram* program ) {
  try {
    if (program)
      return runSubprogram( program->_actionList );
    cerr << "arPForth error: passed program == Nil.\n";
    return false;
  }
  catch (arPForthSpace::arPForthException ce) {
    cerr << "arPForth error: " << ce._message << "\n";
    printDataspace();
    printStack();
    return false;
  }
}

// Return vector by value.  Not for cpu-intensive situations.
vector<string> arPForth::getVocabulary() {
  vector<string> wordList;
  vector<arPForthSpace::arPForthDictionaryEntry>::iterator i;
  for (i=_dictionary.begin(); i!=_dictionary.end(); i++)
    wordList.push_back( i->_word );
  return wordList;
}
