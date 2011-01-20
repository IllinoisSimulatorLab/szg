// Defines mapping from python to C++ strings.
%Include typemaps.sip

%Include database.sip

arDatabaseNode* dsMakeNode(const string&, const string&, const string&);

int dsPlayer(const arMatrix4&, const arVector3&, float);

int dsTransform(const string&, const string&, const arMatrix4&);
bool dsTransform(int, const arMatrix4&);

int dsLoop(const string&, const string&, const string&, int, float, const arVector3&);
//int dsLoop(const string&, const string&, const string&, int, float, const float*);
bool dsLoop(int, const string&, int, float, const arVector3&);
//bool dsLoop(int, const string&, int, float, const float*);

int dsSpeak( const string& name, const string& parent, const string& text );
bool dsSpeak( int ID, const string& text );

int dsStream( const string& name, const string& parent, const string& fileName,
              int paused, float amplitude, int time);
bool dsStream(int ID, const string& fileName, int paused, float amplitude,
              int time);

bool dsErase(const string&);

int ar_speak( const string& message );
%MethodCode
  sipRes = dsSpeak( "messages", "root", *a0 );
%End


class arTTS {
%TypeHeaderCode
#include "arTTS.h"
%End
  public:
    arTTS();
    ~arTTS();
    
    void init();
    void speak( const string& text );
};

