// $Id: PySceneGraph.i,v 1.1 2005/03/18 20:13:01 crowell Exp $
// (c) 2004, Peter Brinkmann (brinkman@math.uiuc.edu)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).


// ******************** based on arInterfaceObject.h ********************

class arInterfaceObject{
  friend void ar_interfaceObjectIOPollTask(void*);
 public:
  arInterfaceObject();
  ~arInterfaceObject();
  
  void setInputDevice(arInputNode*);
  bool start();
  void setNavMatrix(const arMatrix4&);
  arMatrix4 getNavMatrix();
  void setObjectMatrix(const arMatrix4&);
  arMatrix4 getObjectMatrix();

  void setSpeedMultiplier(float);
  
  void setNumMatrices( const int num );
  void setNumButtons( const int num );
  void setNumAxes( const int num );
  
  int getNumMatrices() const;
  int getNumButtons() const;
  int getNumAxes() const;
  
  bool setMatrix( const int num, const arMatrix4& mat );
  bool setButton( const int num, const int but );
  bool setAxis( const int num, const float val );
  void setMatrices( const arMatrix4* matPtr );
  void setButtons( const int* butPtr );
  void setAxes( const float* axisPtr );
  
  arMatrix4 getMatrix( const int num ) const;
  int getButton( const int num ) const;
  float getAxis( const int num ) const;
};

// **************** based on arDistSceneGraphFramework.h *******************

class arDistSceneGraphFramework : public arSZGAppFramework {
 public:
  arDistSceneGraphFramework();
  ~arDistSceneGraphFramework() {}

  arGraphicsDatabase* getDatabase();
                                                                               
  // inherited pure virtual functions
  bool init(int&,char**);
  bool start();
  void stop(bool);
  void loadNavMatrix();
                                                                               
  void setDataBundlePath(const string& bundlePathName, 
                         const string& bundleSubDirectory);

  void setAutoBufferSwap(bool);
  void swapBuffers();
                                                                               
  void setViewer();
  void setPlayer();
                                                                               
  bool restart();
                                                                               
  void setHeadMatrixID(int);
  const string getNavNodeName() const;
                                                                               
  arInputNode* getInputDevice() const;
};
