//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef ARPSEUDOFORTH_H
#define ARPSEUDOFORTH_H

#include "arMath.h"
#include "arStructuredData.h"
#include "arInputEvent.h"
#include "arInputState.h"
#include <vector>
#include <string>
#include <list>

class arPForth;

namespace arPForthSpace {

// Base class for run-time behaviors  
class SZG_CALL arPForthAction {
  public:
    virtual ~arPForthAction() {}
    virtual bool run( arPForth* )=0;
};

// Base class for compile-time behaviors
class SZG_CALL arPForthCompiler {
  public:
    virtual ~arPForthCompiler() {}
    virtual bool compile( arPForth*,
                        vector<arPForthSpace::arPForthAction*>& )=0;
};

// Run-time behavior to place a (constant) number on the stack
class SZG_CALL FetchNumber : public arPForthAction {
  private:
    float _value;
  public:
    FetchNumber( const float value ):_value(value) {}
    virtual ~FetchNumber() {}
    virtual bool run( arPForth* pf );
};

// Most basic compile-time behavior, for words with no compile-time
// side effects.
class SZG_CALL SimpleCompiler : public arPForthCompiler {
  private:
    arPForthAction* _action;
  public:
    SimpleCompiler( arPForthAction* action ):_action( action ) {}
    virtual ~SimpleCompiler() {}
    virtual bool compile( arPForth* pf,
                  vector<arPForthSpace::arPForthAction*>& actionList );
};

class SZG_CALL PlaceHolderCompiler: public arPForthCompiler {
  private:
    string _theWord;
  public:
    PlaceHolderCompiler( const string theWord ):_theWord( theWord ) {}
    virtual ~PlaceHolderCompiler() {}
    virtual bool compile( arPForth* pf,
                          vector<arPForthSpace::arPForthAction*>& actionList );
};

// Compiles a number from the next word in the input stream (the source code)
class SZG_CALL NumberCompiler: public arPForthCompiler {
  private:
    string _theWord;
  public:
    NumberCompiler( const string theWord ):_theWord( theWord ) {}
    virtual ~NumberCompiler() {}
    virtual bool compile( arPForth* pf,
                  vector<arPForthSpace::arPForthAction*>& actionList );
};

// Compiler for words that allocate a predetermined number of cells 
// in the dataspace (determined by constructor arg)
class SZG_CALL VariableCompiler : public arPForthCompiler {
  private:
    unsigned long _size;
  public:
    VariableCompiler( const unsigned long size ):_size( size ) {}
    virtual ~VariableCompiler() {}
    virtual bool compile( arPForth* pf,
                  vector<arPForthSpace::arPForthAction*>& actionList );
};

// Compiler for words that allocate variable numbers of cells 
// in the dataspace (determined by next word in input stream)
class SZG_CALL ArrayCompiler : public arPForthCompiler {
  public:
    ArrayCompiler() {}
    virtual ~ArrayCompiler() {}
    virtual bool compile( arPForth* pf,
                  vector<arPForthSpace::arPForthAction*>& actionList );
};

class SZG_CALL arPForthDictionaryEntry {
  public:
    std::string _word;
    arPForthCompiler* _compiler;
    arPForthDictionaryEntry( const std::string theWord,
                           arPForthCompiler* comp ) :
                           _word( theWord ),
                           _compiler( comp )
                           {}
    arPForthDictionaryEntry( const arPForthDictionaryEntry& d ):
      _word(d._word),
      _compiler(d._compiler)
      {}
    virtual ~arPForthDictionaryEntry() {}
};

struct arPForthException {
  std::string _message;
  arPForthException( const std::string message ):_message(message) {}
};

SZG_CALL bool ar_PForthAddStandardVocabulary( arPForth* pf );

} // namespace arPForthSpace



class SZG_CALL arPForthProgram {
  friend class arPForth;
  public:
    arPForthProgram() {}
    ~arPForthProgram();
  private:
    std::vector<arPForthSpace::arPForthAction*> _actionList;
    std::vector<arPForthSpace::arPForthAction*> _transientActions;
};

/// FORTH interpreter for input-device filters.

class SZG_CALL arPForth {
  public:
    // "user" interface
    arPForth();
    virtual ~arPForth();
    bool compileProgram( const std::string sourceCode );
    bool runProgram();
    bool runProgram( arPForthProgram* program );
    arPForthProgram* getProgram();
    std::vector<std::string> getVocabulary();
    
    // "programmer" interface (for adding new words).
    // be prepared to catch arPForthExceptions if you call some
    // of these other than as part of a normal compile or run.
    bool compileWord( const string theWord,
                      vector<arPForthSpace::arPForthAction*>& actionList );
    bool runSubprogram( vector<arPForthSpace::arPForthAction*>& actionList );
    float stackPop();
    void stackPush( const float val );
    unsigned int stackSize();
    float stackElement( const unsigned int i );
    bool insertStackElement( const unsigned int i, const float val );
    std::string nextWord();
    std::string peekNextWord();
    bool addDictionaryEntry( const arPForthSpace::arPForthDictionaryEntry& entry );
    void addCompiler( arPForthSpace::arPForthCompiler* compiler );
    void addAction( arPForthSpace::arPForthAction* action );
    void addTransientAction( arPForthSpace::arPForthAction* action );
    bool addSimpleActionWord( const string theWord, arPForthSpace::arPForthAction* action );
    bool findWord( std::string theWord );
    bool getWord( std::string theWord,
        std::vector<arPForthSpace::arPForthDictionaryEntry>::iterator& iter );
    void testFailAddress( const long address, unsigned long size );
    unsigned long allocateStorage( const unsigned long theSize );
    unsigned long allocateString();
    float getDataValue( long address );
    void putDataValue( long address, const float val );
    arMatrix4 getDataMatrix( long address );
    void putDataMatrix( long address, const arMatrix4& val );
    void getDataArray( long address, float* const ptr, unsigned long size );
    void putDataArray( long address, const float* const ptr, unsigned long size );
    std::string getString( long address );
    void putString( long address, const std::string& val );
    bool operator!() { return !_valid; }
    void printDataspace();
    void printStringspace();
    bool anonymousActionsAreTransient;
    
  protected:
    bool _valid;

  private:
    arPForthProgram* _program;
    std::vector<float> _dataSpace;
    std::list< std::string > _inputWords;
    std::vector<float> _theStack; // want a bit more access than stack<> allows...
    std::vector<std::string> _stringSpace;
    // _actions is a list of all actions available in the language
    std::vector<arPForthSpace::arPForthAction*> _actions;
    std::vector<arPForthSpace::arPForthCompiler*> _compilers;
    std::vector<arPForthSpace::arPForthDictionaryEntry> _dictionary;
};

#endif        //  #ifndefARPSEUDOFORTH_H
