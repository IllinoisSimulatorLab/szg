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
  
    void receiveData(int,arStructuredData*);
    bool sourceReconfig(int);
  
    // iOwnIt iff the input node owns it & should delete it.
    void addInputSource( arInputSource* theSource, bool iOwnIt );
    int addFilter( arIOFilter* theFilter, bool iOwnIt );
    void addInputSink( arInputSink* theSink, bool iOwnIt );
    
    bool removeFilter( int filterID );
    bool replaceFilter( int filterID, arIOFilter* newFilter, bool iOwnIt );

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

  protected:
    void _lock();
    void _unlock();
    void _setSignature(int,int,int);
    void _remapData( unsigned int channelNumber, arStructuredData* data );
    void _filterEventQueue( arInputEventQueue& queue );
    void _updateState( arInputEventQueue& queue );
    int _findUnusedFilterID();
    
    arInputLanguage _inp;
    arInputEventQueue _eventQueue;
    std::vector< arInputState > _filterStates;
    
    // event callback pointer
    void (*_eventCallback)( arInputEvent& inputEvent );
    
    arMutex _dataSerializationLock;
    std::list<arInputSource*> _inputSourceList;
    std::list<arIOFilter*> _inputFilterList;
    std::list<arInputSink*> _inputSinkList;
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

    typedef std::list<arInputSource*>::iterator arSourceIterator;
    typedef std::list<arInputSink*>::iterator arSinkIterator;
    typedef std::list<arIOFilter*>::iterator arFilterIterator;
};

#endif
