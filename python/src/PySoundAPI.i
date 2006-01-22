// (c) 2004, William Baker (wtbaker@uiuc.edu)
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU LGPL as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/lesser.html).

arDatabaseNode* dsMakeNode(const string&, const string&, const string&);

int dsPlayer(const arMatrix4&, const arVector3&, float);

int dsTransform(const string&, const string&, const arMatrix4&);
bool dsTransform(int, const arMatrix4&);

int dsLoop(const string&, const string&, const string&, int, float, const arVector3&);
int dsLoop(const string&, const string&, const string&, int, float, const float*);
bool dsLoop(int, const string&, int, float, const arVector3&);
bool dsLoop(int, const string&, int, float, const float*);

int dsSpeak( const string& name, const string& parent, const string& text );
bool dsSpeak( int ID, const string& text );

int dsStream( const string& name, const string& parent, const string& fileName,
              int paused, float amplitude, int time);
bool dsStream(int ID, const string& fileName, int paused, float amplitude,
              int time);

bool dsErase(const string&);

%pythoncode %{

def ar_speak( message ):
  return dsSpeak( 'messages', 'root', message )

%}
