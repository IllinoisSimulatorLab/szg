//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// Implementation of the arGUIInfo, arGUIInfo, arGUIMouseInfo and arGUIWindowInfo classes.

#include "arPrecompiled.h"
#include "arLogStream.h"
#include "arGUIInfo.h"

arGUIInfo::arGUIInfo( arGUIEventType eventType, arGUIState state,
                      int windowID, int flag, void* userData ) :
  _eventType( eventType ),
  _state( state ),
  _windowID( windowID ),
  _flag( flag ),
  _userData( userData ),
  _wm(NULL)
{
}

// in this, and all the constructors below, the return value of
// getDataInt should be getting checked, it's possible it can return -1 in
// which case getDataInt has failed, but if any of these getDataInt's fail,
// then the whole constructor should likewise 'fail'
arGUIInfo::arGUIInfo( arStructuredData& data )
{
  _state = arGUIState( data.getDataInt( "state" ) );
  _eventType = arGUIEventType( data.getDataInt( "eventType" ) );
  _windowID = data.getDataInt( "windowID" );
  _flag = data.getDataInt( "flag" );

  // use getDataInt and <reinterpret_cast> instead?
  // Experimenting with getting rid of this. Pointers are not guaranteed to
  // have 32 bits! Instead, this is stuffed in arGUIWindow::getNextGUIEvent.
  //data.dataOut( "userData", &_userData, AR_INT, 1 );
  _userData = NULL;
  _wm = NULL;
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
  if ( arGUIEventType( data.getDataInt( "eventType" ) ) != AR_KEY_EVENT ) {
    ar_log_error() << "arGUIKeyInfo: not an AR_KEY_EVENT.\n";
    return;
  }

  _key = data.getDataInt( "key" );
  _ctrl = data.getDataInt( "ctrl" );
  _alt = data.getDataInt( "alt" );
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
  if ( arGUIEventType( data.getDataInt( "eventType" ) ) != AR_MOUSE_EVENT ) {
    ar_log_error() << "arGUIMouseInfo: not an AR_MOUSE_EVENT.\n";
    return;
  }

  _button = data.getDataInt( "button" );

  _posX = data.getDataInt( "posX" );
  _posY = data.getDataInt( "posY" );

  _prevPosX = data.getDataInt( "prevPosX" );
  _prevPosY = data.getDataInt( "prevPosY" );

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
  if ( arGUIEventType( data.getDataInt( "eventType" ) ) != AR_WINDOW_EVENT ) {
    ar_log_error() << "arGUIWindowInfo: not an AR_WINDOW_EVENT.\n";
    return;
  }

  _posX = data.getDataInt( "posX" );
  _posY = data.getDataInt( "posY" );

  _sizeX = data.getDataInt( "sizeX" );
  _sizeY = data.getDataInt( "sizeY" );
}
