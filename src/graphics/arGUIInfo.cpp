/**
 * @file arGUIInfo.cpp
 * Implementation of the arGUIInfo, arGUIInfo, arGUIMouseInfo and arGUIWindowInfo classes.
 */
#include "arPrecompiled.h"

#include <iostream>

#include "arGUIInfo.h"

arGUIInfo::arGUIInfo( arGUIEventType eventType, arGUIState state,
                      int windowID, int flag ) :
  _eventType( eventType ),
  _state( state ),
  _windowID( windowID ),
  _flag( flag )
{

}

// in this, and all the constructors below, the return value of
// getDataFieldIndex should be checked before being passed to getDataInt, it's
// possible it can return -1 in which case getDataInt fails, but if any of
// these getDataInt's fail, then the whole constructor should likewise 'fail'
arGUIInfo::arGUIInfo( arStructuredData& data )
{
  _state = arGUIState( data.getDataInt( data.getDataFieldIndex( "state" ) ) );
  _eventType = arGUIEventType( data.getDataInt( data.getDataFieldIndex( "eventType" ) ) );

  _windowID = data.getDataInt( data.getDataFieldIndex( "windowID" ) );

  _flag = data.getDataInt( data.getDataFieldIndex( "flag" ) );
}

arGUIInfo::~arGUIInfo( void )
{

}

arGUIKeyInfo::arGUIKeyInfo( arGUIEventType eventType, arGUIState state,
                      int windowID, int flag, arGUIKey key,
                      int ctrl, int alt ) :
  arGUIInfo( eventType, state, windowID, flag ),
  _key( key ),
  _ctrl( ctrl ),
  _alt( alt )
{

}

arGUIKeyInfo::arGUIKeyInfo( arStructuredData& data ) :
  arGUIInfo( data )
{
  if( data.getDataInt( data.getDataFieldIndex( "eventType" ) ) != AR_KEY_EVENT ) {
    std::cout << "Cannot build arKeyInfo from this arStructuredData" << std::endl;
    return;
  }

  _key = data.getDataInt( data.getDataFieldIndex( "key" ) );

  _ctrl = data.getDataInt( data.getDataFieldIndex( "ctrl" ) );
  _alt = data.getDataInt( data.getDataFieldIndex( "alt" ) );
}

arGUIKeyInfo::~arGUIKeyInfo( void )
{

}

arGUIMouseInfo::arGUIMouseInfo( arGUIEventType eventType, arGUIState state,
                          int windowID, int flag, arGUIButton button,
                          int posX, int posY, int prevPosX, int prevPosY ) :
  arGUIInfo( eventType, state, windowID, flag ),
  _button( button ),
  _posX( posX ),
  _posY( posY ),
  _prevPosX( prevPosX ),
  _prevPosY( prevPosY )
{

}

arGUIMouseInfo::arGUIMouseInfo( arStructuredData& data ) :
  arGUIInfo( data )
{
  if( data.getDataInt( data.getDataFieldIndex( "eventType" ) ) != AR_MOUSE_EVENT ) {
    std::cout << "Cannot build arMouseInfo from this arStructuredData" << std::endl;
    return;
  }

  _button = data.getDataInt( data.getDataFieldIndex( "button" ) );

  _posX = data.getDataInt( data.getDataFieldIndex( "posX" ) );
  _posY = data.getDataInt( data.getDataFieldIndex( "posY" ) );

  _prevPosX = data.getDataInt( data.getDataFieldIndex( "prevPosX" ) );
  _prevPosY = data.getDataInt( data.getDataFieldIndex( "prevPosY" ) );

}

arGUIMouseInfo::~arGUIMouseInfo( void )
{

}

arGUIWindowInfo::arGUIWindowInfo( arGUIEventType eventType, arGUIState state,
                            int windowID, int flag, int posX, int posY,
                            int sizeX, int sizeY ) :
  arGUIInfo( eventType, state, windowID, flag ),
  _posX( posX ),
  _posY( posY ),
  _sizeX( sizeX ),
  _sizeY( sizeY )
{

}

arGUIWindowInfo::arGUIWindowInfo( arStructuredData& data ) :
  arGUIInfo( data )
{
  if( data.getDataInt( data.getDataFieldIndex( "eventType" ) ) != AR_WINDOW_EVENT ) {
    std::cout << "Cannot build arWindowInfo from this arStructuredData" << std::endl;
    return;
  }

  _posX = data.getDataInt( data.getDataFieldIndex( "posX" ) );
  _posY = data.getDataInt( data.getDataFieldIndex( "posY" ) );

  _sizeX = data.getDataInt( data.getDataFieldIndex( "sizeX" ) );
  _sizeY = data.getDataInt( data.getDataFieldIndex( "sizeY" ) );
}

arGUIWindowInfo::~arGUIWindowInfo( void )
{

}
