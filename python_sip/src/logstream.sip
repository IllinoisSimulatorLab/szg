// Defines mapping from python to C++ strings.
%Include typemaps.sip

%ModuleHeaderCode
using namespace std;
%End

// SILENT:   Absolutely nothing.
// CRITICAL: Essential state;  unrecoverable errors (app or component terminates).
// ERROR:    Something's wrong.
// WARNING:  The user likely didn't intend this (use WARNING sparingly).
// REMARK:   Everything else.
// DEBUG:    For ISL developers.

enum {
  AR_LOG_SILENT=0,
  AR_LOG_CRITICAL,
  AR_LOG_ERROR,
  AR_LOG_WARNING,
  AR_LOG_REMARK,
  AR_LOG_DEBUG,
  AR_LOG_NIL // invalid value
};

int ar_stringToLogLevel(const string&);
string ar_logLevelToString(int);
bool ar_setLogLevel( const string& level, const bool fVerbose = true );

class arLogStream {
 public:
  arLogStream();
  ~arLogStream();

  void setHeader(const string&);
  bool setLogLevel(int);
  void setTimestamp(const bool);
  string logLevel();
  bool logLevelDefault();

  arLogStream& write( long n );
%MethodCode
    sipRes = &(sipCpp->operator<<( a0 ));
%End

  arLogStream& write( double n );
%MethodCode
    sipRes = &(sipCpp->operator<<( a0 ));
%End

  arLogStream& write( const string& s );
%MethodCode
    sipRes = &(sipCpp->operator<<( *a0 ));
%End

 private:
  arLogStream( const arLogStream& );
};

// Accessors (singleton).
arLogStream& ar_log_critical();
arLogStream& ar_log_error();
arLogStream& ar_log_warning();
arLogStream& ar_log_remark();
arLogStream& ar_log_debug();

void ar_setLogLabel( const string& label );
string ar_getLogLabel();
string ar_logLevelsExpected();


