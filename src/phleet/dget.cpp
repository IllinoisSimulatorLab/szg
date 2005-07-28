//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arXMLParser.h"
#include "arSZGClient.h"

string walkDownDocTree(arSZGClient& szgClient, arSlashString& paramList){
  int pathPlace = 0;
  // The first element in the path gives the name of the global attribute.
  string docString = szgClient.getGlobalAttribute(paramList[0]);
  TiXmlDocument doc;
  doc.Clear();
  doc.Parse(docString.c_str());
  if (doc.Error()){
    cout << "dget error: in parsing gui config xml on line: " 
         << doc.ErrorRow() << endl;
    return string("NULL");
  }
  TiXmlNode* node = doc.FirstChild();
  if (!node || !node->ToElement()){
    cout << "dget error: malformed XML (global node).\n";
    return string("NULL");
  }
  TiXmlNode* child = node;
  pathPlace = 1;
  while (pathPlace < paramList.size()){
    // As we are searching down the doc tree, we allow an array syntax for
    // picking out child elements.
    // For instance,
    //     szg_viewport[0]
    // or
    //     szg_viewport
    // mean the first szg_viewport child, while
    //     szg_viewport[1]
    // means the second child.
    unsigned int firstArrayLoc;
    // The default is to use the first child element with the appropriate name.
    int actualArrayIndex = 0;
    // This is the actual type of the element. The default is the current step
    // in the path... but this could change if there is an array index.
    string actualElementType = paramList[pathPlace];
    if ( (firstArrayLoc = paramList[pathPlace].find('[')) 
	  != string::npos ){
      // There might be a valid array index.
      unsigned int lastArrayLoc 
        = paramList[pathPlace].find_last_of("]");
      if (lastArrayLoc == string::npos){
	// It seems like we should have a valid array index, but we do not.
	cout << "dget error: invalid array index in "
	     << paramList[pathPlace] << ".\n";
	return string("NULL");
      }
      string potentialIndex 
        = paramList[pathPlace].substr(firstArrayLoc+1, 
                                      lastArrayLoc-firstArrayLoc-1);
      stringstream indexStream;
      indexStream << potentialIndex;
      indexStream >> actualArrayIndex;
      if (indexStream.fail()){
	cout << "dget error: invalid array index " << potentialIndex
	     << ".\n";
	return string("NULL");
      }
      if (actualArrayIndex < 0){
	cout << "dget error: array index cannot be negative.\n";
	return string("NULL");
      }
      actualElementType = paramList[pathPlace].substr(0, firstArrayLoc);
    }
    // Must get the first child. If we want something further, iterate
    // from that starting place.
    TiXmlNode* newChild = child->FirstChild(actualElementType);
    if (newChild){
      // Step through siblings.
      int which = 1;
      while (newChild && which <= actualArrayIndex){
        newChild = newChild->NextSibling();
	which++;
      }
    }
     
    if (!newChild){
      // This might still be OK. It could be an "attribute".
      if (!child->ToElement()->Attribute(paramList[pathPlace])){
	// Neither an "element" or an "attribute". This is an error.
	cout << "dget error: " << paramList[pathPlace]
	     << " is neither an element or an attribute.\n";
	return string("NULL");
      }
      else{
	// OK, it's an attribute. This is an error if it isn't the last
	// value on the path.
        string attribute = child->ToElement()->Attribute(paramList[pathPlace]);
        if (pathPlace != paramList.size() -1){
	  cout << "dget warning: attribute name ("
	       << paramList[pathPlace] << ") not last in path list.\n";
	}
	return attribute;
      }
    }
    else{
      // Check to make sure this is valid XML. 
      if (!newChild->ToElement()){
	cout << "dget error: malformed XML in " << paramList[pathPlace] 
             << ".\n";
        return string("NULL");
      }
      // Valid XML. Continue the walk down the tree.
      // NOTE: Syzygy supports POINTERS. So, a node might really be stored
      // inside another global parameter. Like so:
      //     <szg_screen usenamed="left_wall" />
      // If this is the case, we must get THAT document and start again.
      // NOTE: We also want to be able to SET the pointer. So, when the
      // very next step in the path is "usenamed" then do not retrieve the
      // pointed-to data.
      if (newChild->ToElement()->Attribute("usenamed")
          && (pathPlace+1 >= paramList.size() 
              || paramList[pathPlace+1] != "usenamed")){
	string pointerName(newChild->ToElement()->Attribute("usenamed"));
        string newDocString 
          = szgClient.getGlobalAttribute(pointerName);
        doc.Clear();
        doc.Parse(newDocString.c_str());
        newChild = doc.FirstChild();
	if (!newChild || !newChild->ToElement()){
          cout << "dget error: malformed XML in pointer ("
	       << newDocString << ").\n";
	  return string("NULL");
	}
      }
      child = newChild;
    }
    pathPlace++;
  }
  // By making it out here, we know that the final piece of the path refers
  // to an element.
  //std::stringstream output;
  //output << *child;
  //return output.str();
  std::string output;
  output << *child;
  return output;
}

int main(int argc, char** argv){
  // NOTE: arSZGClient::init(...) must come before the argument parsing...
  // otherwise -szg user=... and -szg server=... will not work.
  arSZGClient szgClient; 
  szgClient.init(argc, argv);
  if (!szgClient) {
    cerr << "dget error: failed to initialize SZGClient.\n";
    return 1;
  }
  bool fAll = false;
  if (argc >= 2 && !strcmp(argv[1],"-a")){
    fAll = true;
  }
  if ((fAll && argc > 3) || (!fAll && (argc==1 || argc==3 || argc>4))){
    cerr << "usage:\n  "
         << argv[0] << " computer parameter_group parameter_name\n  "
	 << argv[0] << " global_parameter_name\n  "
	 << argv[0] << " -a             (Produce a dbatch-file.)\n  "
	 << argv[0] << " -a substring\n  ";
    return 1;
  }

  if (fAll){
    cout << szgClient.getAllAttributes(argc==3 ? argv[2] : "ALL");
  }
  else{
    if (argc==2){
      arSlashString paramList(argv[1]);
      if (paramList.size() > 1){
        cout << walkDownDocTree(szgClient, paramList) << endl;
      }
      else{
        cout <<  szgClient.getGlobalAttribute(argv[1]) << endl;
      }
    }
    else{
      // argc = 4
      cout << szgClient.getAttribute(argv[1], argv[2], argv[3], "") << endl;
    }
  }
  return 0;
}
