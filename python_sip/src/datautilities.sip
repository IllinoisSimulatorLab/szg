// Defines mapping from python to C++ strings.
%Include typemaps.sip

string ar_versionString();

string ar_getUser();

// Looks for the named file in the given subdirectory of the path
string ar_fileFind(const string& name,
		            const string& subdirectory,
		            const string& path);
// Looks for the named directory in the given subdirectory of the path
string ar_directoryFind(const string& name,
		                 const string& subdirectory,
		                 const string& path);

