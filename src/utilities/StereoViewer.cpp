//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include <stdio.h>
#include <stdlib.h>
#include "arMasterSlaveFramework.h"

string leftFile, rightFile;
arTexture leftImage, rightImage;
int screenWidth = -1, screenHeight = -1;
int numImages = -1;

bool init(arMasterSlaveFramework&, arSZGClient& cli) {
  const string dataPath(cli.getAttribute( "SZG_DATA", "path" ));
  if (!leftImage.readPPM( leftFile, dataPath )){
    cerr << "StereoViewer error: failed to read picture file \""
         << leftFile << "\".\n";
    return false;
  }
  if (!leftImage.flipHorizontal()) {
    cerr << "StereoViewer error: failed to flip picture " << leftFile << endl;
    return false;
  }
  cerr << "Image size: " << leftImage.getWidth()/2 << " X "
       << leftImage.getHeight() << endl;
  if (numImages == 2) {
    if (!rightImage.readPPM( rightFile, dataPath )){
      cerr << "StereoViewer error: failed to read picture file " << rightFile
           << " from path " << dataPath << ".\n";
      return false;
    }
    if (!rightImage.flipHorizontal()) {
      cerr << "StereoViewer error: failed to flip picture " << rightFile
           << endl;
      return false;
    }
  }

  glClearColor(0,0,0,0); // OpenGL initialization
  return true;
}

void showCenteredImage( arTexture& theImage ) {
  const int imageWidth = theImage.getWidth();
  const int imageHeight = theImage.getHeight();
  int xOffset = 0;
  int yOffset = 0;
  if (imageWidth < screenWidth)
    xOffset = (int)floor( 0.5*(screenWidth-imageWidth) );
  if (imageHeight < screenHeight)
    yOffset = (int)floor( 0.5*(screenHeight-imageHeight) );
  glRasterPos3i( xOffset, yOffset, 0 );
  glDrawPixels( imageWidth, imageHeight, GL_RGB, GL_UNSIGNED_BYTE,
                theImage.getPixels());
}

void showCenteredHalfImage( arTexture& theImage, bool leftHalf ) {
  const int imageWidth = theImage.getWidth()/2;
  const int imageHeight = theImage.getHeight();
  int xOffset = 0;
  int yOffset = 0;
  if (imageWidth < screenWidth)
    xOffset = (int)floor( 0.5*(screenWidth-imageWidth) );
  if (imageHeight < screenHeight)
    yOffset = (int)floor( 0.5*(screenHeight-imageHeight) );
  glRasterPos3i( xOffset, yOffset, 0 );
  glPixelStorei( GL_UNPACK_ROW_LENGTH, theImage.getWidth() );
  glPixelStorei( GL_UNPACK_SKIP_PIXELS, leftHalf ? imageWidth : 0 );
  glDrawPixels( imageWidth, imageHeight, GL_RGB, GL_UNSIGNED_BYTE,
                theImage.getPixels());
}

void display( arMasterSlaveFramework& fw, arGraphicsWindow& graphicsWindow, arViewport& viewport ) {
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glOrtho( 0, screenWidth, 0, screenHeight, -1, 1 );
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

  const bool fLeftEye = graphicsWindow.getCurrentEyeSign() <= 0.0f; // fw.getCurrentEye() <= 0.;

  if (numImages == 2)
    showCenteredImage( fLeftEye ? leftImage : rightImage );
  else
    showCenteredHalfImage( leftImage, fLeftEye );
}

void reshape(arMasterSlaveFramework&, int width, int height) {
  screenWidth = width;
  screenHeight = height;
  glViewport(0,0,width,height);
}

void windowEvent( arMasterSlaveFramework& fw, arGUIWindowInfo* windowInfo ) {
  if( !windowInfo ) {
    return;
  }

  switch( windowInfo->getState() ) {
    case AR_WINDOW_RESIZE:
      screenWidth = windowInfo->getSizeY();
      screenHeight = windowInfo->getSizeX();
      fw.getWindowManager()->setWindowViewport( windowInfo->getWindowID(),
                                                0, 0, windowInfo->getSizeX(), windowInfo->getSizeY() );
    break;

    default:
    break;
  }
}



int main(int argc, char** argv){
  if (argc != 2 && argc != 3) {
    cerr << "usage: StereoViewer left_image right_image\n"
         << "       StereoViewer stereo_pair_image\n";
    return 1;
  }

  numImages = argc-1;
  leftFile = argv[1];
  if (numImages == 2)
    rightFile = argv[2];

  arMasterSlaveFramework framework;
  if (!framework.init(argc, argv))
    return 1;

  framework.setStartCallback(init);
  framework.setDrawCallback(display);
  // framework.setReshapeCallback(reshape);
  framework.setWindowEventCallback(windowEvent);
  return framework.start() ? 0 : 1;
}
