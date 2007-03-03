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

class SZG_CALL arLaunchInfo{
 public:
  arLaunchInfo(){}
  ~arLaunchInfo(){}

  string computer;
  string process;
  string context;
  string tradingTag;
  string info;
};

class SZG_CALL arAppLauncher{
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

  bool isMaster();
  int getNumberScreens();
  int getPipeNumber();
  int getMasterPipeNumber();
  string getMasterName();
  string getTriggerName();
  string getScreenName(int num);
  bool getRenderProgram(const int num, string& computer, string& renderName);
  void updateRenderers(const string& attribute, const string& value);

 private:
  arSZGClient*       _client;
  bool               _clientIsMine;
  bool               _vircompDefined;
  int                _numberPipes;
  string*            _pipeComp;
  string*            _pipeScreen;
  arLaunchInfo*      _renderLaunchInfo;
  string             _appType;
  string             _renderProgram;
  string             _vircomp;
  string             _location;
  string             _exeName;
  list<arLaunchInfo> _serviceList;
  bool               _onlyIncompatibleServices;

  string _firstToken(const string&);
  string _getAttribute(const string& g, const string& p, const string& v)
    { return _client->getAttribute(_vircomp, g, p, v); }
  bool _setAttribute(const string& group, const string& param, const string& value);
  bool _prepareCommand();
  void _setNumberPipes(int);
  bool _setPipeName(int);
  string _screenName(int i);
  void _addService(const string& computer, const string& process,
                   const string& context, const string& tradingTag,
                   const string& info);
  bool _trylock();
  void _unlock();
  bool _execList(list<arLaunchInfo>* appsToLaunch);
  void _blockingKillByID(list<int>*);
  void _graphicsKill(string);
  bool _demoKill();
  void _relaunchAllServices(list<arLaunchInfo>&, list<int>&);
  void _relaunchIncompatibleServices(list<arLaunchInfo>&, list<int>&);

  string _getRenderContext(int i);
  string _getInputContext();
  string _getSoundContext();
};

#endif
