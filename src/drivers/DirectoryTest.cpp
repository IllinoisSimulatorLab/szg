#include "arDirectory.h"

using namespace std;

int main() {
  cout << "\nTesting setPath:\n";
  arDirectory here;
  here.setPath(".");
  if (!here) {
    cout << "setPath test failed\n";
    return 0;
  }
  cout << "Path = " << here.path().c_str() << endl << endl;
  unsigned int i;
  
  std::vector<std::string> fileList = here.files();
  cout << "List of " << fileList.size() << " files:\n\n";
  for (i=0; i<fileList.size(); i++)
    cout << fileList[i].c_str() << endl;
  cout << endl;
  
  std::vector<std::string> dirList = here.subDirectories();
  cout << "\nList of " << dirList.size() << " subdirectories:\n\n";
  for (i=0; i<dirList.size(); i++)
    cout << dirList[i].c_str() << endl;
  cout << endl;

  cout << "Testing file search:\n";
  cout << here.fileExists(".bashrc") ?
    "File .bashrc found.\n\n" :
    "No file named .bashrc found.\n\n";
  
  cout << "Testing subdirectory search:\n";
  arDirectory tempDir = here.getSubdirectory("temp");
  cout << !tempDir ?
    "Subdirectory temp not found.\n" :
    "Opened subdirectory temp.\n";
  
  return 0;
}
