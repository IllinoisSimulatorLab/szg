//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INPUT_NODE_H
#define AR_INPUT_NODE_H

#include "arInputSink.h"
#include "arInputSource.h"
#include "arIOFilter.h"
#include "arInputState.h"
#include "arInputEventQueue.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"
#include <list>
#include <vector>

/// You can chain these together.

class SZG_CALL arInputNode: public arInputSink {
  // Needs assignment operator and copy constructor, for pointer members.
  public:
    arInputNode( bool bufferEvents = false );
    // if anyone ever derives from this class, make the following virtual:
    // destructor init start stop restart receiveData sourceReconfig.
    ~arInputNode();
  
    bool init(arSZGClient&);
    bool start();
    bool stop();
    bool restart();
  
    void receiveData(int,arStructuredData*);
    bool sourceReconfig(int);
  
    // iOwnIt == true => the input node owns it & should delete.
    void addInputSource( arInputSource* theSource, bool iOwnIt );
    void addFilter( arIOFilter* theFilter, bool iOwnIt );
    void addInputSink( arInputSink* theSink, bool iOwnIt );
    
    void removeFilter(  arIOFilter* theFilter );

    void setEventCallback(void (*callback)(arInputEvent&))
      { _eventCallback = callback; }
  
    // getXXX() aren't const, because they use _dataSerializationLock.
    int getButton(int);
    float getAxis(int);
    arMatrix4 getMatrix(int);
  
    /// \todo some classes use getNumberButtons, others getNumButtons (etc).  Be consistent.
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
    typedef std::list<arInputSource*>::iterator arSourceIterator;
    typedef std::list<arInputSink*>::iterator arSinkIterator;
    typedef std::list<arIOFilter*>::iterator arFilterIterator;
};

#endif
