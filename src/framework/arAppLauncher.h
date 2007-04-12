//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_APP_LAUNCHER_H
#define AR_APP_LAUNCHER_H

#include "arSZGClient.h"
#include "arFrameworkCalling.h"

#include <list>
using namespace std;

// Launch and kill apps on multiple nodes of a cluster.

class SZG_CALL arLaunchInfo {
 public:
  arLaunchInfo(){}
  arLaunchInfo(const string& a, const string& b, const string& c) :
    computer(a), process(b), context(c) {}
  arLaunchInfo(const string& a, const string& b, const string& c,
    const string& d, const string& e) :
    computer(a), process(b), context(c), tradingTag(d), info(e) {}

  string computer;
  string process;
  string context;
  string tradingTag;
  string info;
};

typedef list<arLaunchInfo>::const_iterator iLaunch;

class SZG_CALL arPipe {
 public:
  string hostname;
  string screenname; // e.g. "SZG_DISPLAY3"
  arLaunchInfo renderer;
  arPipe() {}
  arPipe(const string& a, const string& b) : hostname(a), screenname(b) {}
};

class SZG_CALL arAppLauncher {
  // Needs assignment operator and copy constructor, for pointer members.
 public:
  arAppLauncher(const char* exeName = NULL);
  ~arAppLauncher();

  bool setRenderProgram(const string&);
  bool setAppType(const string&);
  bool setVircomp();
  bool setVircomp(const string& vircomp); 
  string getVircomp() const { return _vircomp; }   
  string getLocation() const { return _location; }

  bool connectSZG(int& argc, char** argv);
  bool setSZGClient(arSZGClient*);

  void killApp();
  bool killAll();
  bool launchApp();
  bool restartServices();
  bool screenSaver();
  bool setParameters();
  bool waitForKill();

  // A "screen" of the v.c. is a window that is rendered into.
  // See doc/GraphicsConfiguration.html.

  bool isMaster() const;
  int getNumberScreens() const { return _pipes.size(); }
  string getMasterName() const;
  string getTriggerName() const;
  string getScreenName(const int) const;
  bool getRenderProgram(const int num, string& computer, string& renderName);
  void updateRenderers(const string& attribute, const string& value);

 private:
  arSZGClient* _szgClient;
  bool _ownedClient;
  bool _vircompDefined;
  bool _onlyIncompatibleServices; // Relaunch only services which are incompatible.
  string _appType;
  string _renderer;
  string _vircomp;
  string _location;
  string _exeName;
  vector<arPipe> _pipes;
  list<arLaunchInfo> _serviceList;

  string _getAttribute(const string& g, const string& p, const string& v) const
    { return _szgClient->getAttribute(_vircomp, g, p, v); }
  int _getAttributeInt(const string& g, const string& p, const string& v) const
    { return _szgClient->getAttributeInt(_vircomp, g, p, v); }
  bool _setAttribute(const string& g, const string& p, const string& v)
    { return _szgClient->setAttribute(_vircomp, g, p, v); }

  string _firstToken(const string&) const;
  bool _prepareCommand();
  bool _setScreenName(int);
  string _screenName(int i) const;
  void _addService(const string& computer, const string& process,
                   const string& context, const string& tradingTag,
                   const string& info);
  bool _trylock();
  void _unlock();
  bool _execList(list<arLaunchInfo>* appsToLaunch);
  void _blockingKillByID(list<int>*);
  void _graphicsKill(const string&);
  bool _demoKill();
  void _relaunchAllServices(list<arLaunchInfo>&, list<int>&);
  void _relaunchIncompatibleServices(list<arLaunchInfo>&, list<int>&);

  string _getRenderContext(const int) const;
  string _getInputContext() const;
  string _getSoundContext() const;
  int _getPID(const int i, const string& name)
    { return _szgClient->getProcessID(_pipes[i].hostname, name); }
  bool _szgClientOK() const;
  bool _iValid(const int i) const { return i>=0 && i<getNumberScreens(); }
};

#endif
