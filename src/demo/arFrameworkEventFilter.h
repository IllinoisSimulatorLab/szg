#ifndef ARFRAMEWORKEVENTFILTER_H
#define ARFRAMEWORKEVENTFILTER_H

#include "arIOFilter.h"

class arSZGAppFramework;

class arFrameworkEventFilter : public arIOFilter {
  public:
    arFrameworkEventFilter( arSZGAppFramework* fw = 0 );
    virtual ~arFrameworkEventFilter() {}
    void setFramework( arSZGAppFramework* fw ) { _framework = fw; }
    arSZGAppFramework* getFramework() const { return _framework; }
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent ) { return true; }
  private:
    arSZGAppFramework* _framework;
};

typedef bool (*arFrameworkEventCallback)( arInputEvent& event, arIOFilter* filter, arSZGAppFramework* fw );

class arCallbackEventFilter : public arFrameworkEventFilter {
  public:
    arCallbackEventFilter( arSZGAppFramework* fw = 0, arFrameworkEventCallback cb = 0 );
    virtual ~arCallbackEventFilter() {}
    void setCallback( arFrameworkEventCallback cb ) { _callback = cb; }
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent );
  private:
    arFrameworkEventCallback _callback;
};

#endif        //  #ifndefARFRAMEWORKEVENTFILTER_H

