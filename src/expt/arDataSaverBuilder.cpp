#include "arPrecompiled.h"
#include <string>
#include "arDataSaverBuilder.h"

// Add header files for new saver types here
#include "arXMLDataSaver.h"
#include "arDummyDataSaver.h"

arDataSaver* arDataSaverBuilder::build( const std::string dataStyle ) {
  if (dataStyle == "xml") {
    cerr << "arDataSaverBuilder remark: data_style == " << dataStyle << endl;
    arDataSaver* dataSaver = (arDataSaver*)new arXMLDataSaver;
    if (dataSaver==0)
      cerr << "arDataSaverBuilder error: couldn't create arXMLDataSaver.\n";
    return dataSaver;
  } else if (dataStyle == "none") {
    cerr << "arDataSaverBuilder remark: data_style == " << dataStyle << endl;
    arDataSaver* dataSaver = (arDataSaver*)new arDummyDataSaver;
    if (dataSaver==0)
      cerr << "arDataSaverBuilder error: couldn't create arDummyDataSaver.\n";
    return dataSaver;
  } else {
    cerr << "arDataSaverBuilder error: data_style is not a valid output data type.\n"
         << "    Valid values are: xml, none\n"
         << "    That is all.\n";
    return (arDataSaver*)0;
  }
}


