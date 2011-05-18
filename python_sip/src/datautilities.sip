// Defines mapping from python to C++ strings.
%Include typemaps.sip

string ar_versionString( bool verbose=true );
string ar_versionInfo( bool verbose=true );

string ar_getUser();

// Looks for the named file in the given subdirectory of the path
string ar_fileFind(const string& name,
		            const string& subdirectory,
		            const string& path);
// Looks for the named directory in the given subdirectory of the path
string ar_directoryFind(const string& name,
		                 const string& subdirectory,
		                 const string& path);


class ar_timeval {
 public:
  int sec;
  int usec;
  ar_timeval();
  ar_timeval(int s, int u);
  bool zero() const;
};

ar_timeval ar_time() /TransferBack/;
double ar_difftime(const ar_timeval& end, const ar_timeval& start);
double ar_difftimeSafe(const ar_timeval& end, const ar_timeval& start);
ar_timeval& ar_addtime(ar_timeval& result, const ar_timeval& addend); // returns "result"
string ar_currentTimeString();

