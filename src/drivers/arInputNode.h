//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INPUT_NODE_H
#define AR_INPUT_NODE_H

#include "arInputSink.h"
#include "arInputSource.h"
#include "arIOFilter.h"
#include "arInputState.h"
#include "arInputEventQueue.h"
#include "arDriversCalling.h"
#include <list>
#include <vector>

// You can chain arInputNodes.

class SZG_CALL arInputNode: public arInputSink {
  // Needs assignment operator and copy constructor, for pointer members.
  public:
    arInputNode( bool bufferEvents = false );
    // If anyone ever derives from this class, make the following virtual:
    // destructor init start stop restart receiveData sourceReconfig.
    ~arInputNode();

    bool init(arSZGClient&);
    bool start();
    bool stop();
    bool restart();

    void receiveData(int, arStructuredData*);
    bool sourceReconfig(int);

    // Allow apps to post input events.
    void postEventQueue( arInputEventQueue& queue );
    
    // iOwnIt iff the input node owns it & should delete it.
    void addInputSource( arInputSource*, bool iOwnIt );
    int addFilter( arIOFilter*, bool iOwnIt );
    void addInputSink( arInputSink*, bool iOwnIt );

    bool removeFilter( int ID );
    bool replaceFilter( int ID, arIOFilter* newFilter, bool iOwnIt );

    void setEventCallback(void (*callback)(arInputEvent&))
      { _eventCallback = callback; }

    // getXXX() aren't const, because they use _dataSerializationLock.
    int getButton(int);
    float getAxis(int);
    arMatrix4 getMatrix(int);

    int getNumberButtons() const;
    int getNumberAxes() const;
    int getNumberMatrices() const;

    void processBufferedEvents();

    arInputState _inputState;

  private:
    void _setSignature(int, int, int);
    void _remapData( unsigned channelNumber, arStructuredData* data );
    void _filterEventQueue( arInputEventQueue& queue );
    void _updateState( arInputEventQueue& queue );
    int _findUnusedFilterID() const;

    arInputLanguage _inp;
    arInputEventQueue _eventQueue;
    std::vector< arInputState > _filterStates;

    void (*_eventCallback)( arInputEvent& inputEvent );

    arLock _dataSerializationLock;
    std::list<arInputSource*> _sources;
    std::list<arIOFilter*> _filters;
    std::list<arInputSink*> _sinks;
    std::vector<bool> _iOwnSources;
    std::vector<bool> _iOwnFilters;
    std::vector<bool> _iOwnSinks;

    int _currentChannel;

    bool _bufferInputEvents;
    arInputEventQueue _eventBuffer;

  private:
    bool _complained;
    bool _initOK;
    string _label;

    typedef list<arInputSource*>::iterator iterSrc;
    typedef list<arInputSink*>::iterator iterSink;
    typedef list<arIOFilter*>::iterator iterFlt;
    typedef list<arIOFilter*>::const_iterator constIterFlt;
};

#endif
