//********************************************************
// Syzygy is licensed under the BSD license v2
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
bool twoImages = false;

bool init(arMasterSlaveFramework&, arSZGClient& cli) {
  const string dataPath(cli.getAttribute( "SZG_DATA", "path" ));
  if (!leftImage.readPPM( leftFile, dataPath )){
    cerr << "StereoViewer error: failed to read picture file \"" << leftFile
           << "\" from path " << dataPath << ".\n";
    return false;
  }
  if (!leftImage.flipHorizontal()) {
    cerr << "StereoViewer error: failed to flip picture file " << leftFile << endl;
    return false;
  }
  cout << "StereoViewer remark: Image size " << leftImage.getWidth()/2 << " X "
       << leftImage.getHeight() << endl;
  if (twoImages) {
    if (!rightImage.readPPM( rightFile, dataPath )){
      cerr << "StereoViewer error: failed to read picture file " << rightFile
           << " from path " << dataPath << ".\n";
      return false;
    }
    if (!rightImage.flipHorizontal()) {
      cerr << "StereoViewer error: failed to flip picture file " << rightFile
           << endl;
      return false;
    }
  }

  glClearColor(0,0,0,0); // OpenGL initialization
  return true;
}

void showImage( arTexture& theImage, bool fLeftHalf, bool fHalf ) {
  const int imageWidth = theImage.getWidth();
  const int imageHeight = theImage.getHeight();
  const int xOffset = imageWidth >= screenWidth ? 0 :
    (int)floor( 0.5*(screenWidth-imageWidth) );
  const int yOffset = imageHeight >= screenHeight ? 0 :
    (int)floor( 0.5*(screenHeight-imageHeight) );
  glRasterPos3i( xOffset, yOffset, 0 );

  if (fHalf) {
    glPixelStorei( GL_UNPACK_ROW_LENGTH, theImage.getWidth() );
    glPixelStorei( GL_UNPACK_SKIP_PIXELS, fLeftHalf ? imageWidth : 0 );
  }

  glDrawPixels( imageWidth, imageHeight, GL_RGB, GL_UNSIGNED_BYTE,
                theImage.getPixels());
}

void display( arMasterSlaveFramework&, arGraphicsWindow& graphicsWindow, arViewport&) {
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glOrtho( 0, screenWidth, 0, screenHeight, -1, 1 );
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
  const bool fLeftEye = graphicsWindow.getCurrentEyeSign() <= 0.0f; // fw.getCurrentEye() <= 0.;
  if (twoImages)
    showImage( fLeftEye ? leftImage : rightImage, false, false); // full image
  else
    showImage( leftImage, fLeftEye, true); // half image
}

#ifdef UNUSED
void reshape(arMasterSlaveFramework&, int width, int height) {
  screenWidth = width;
  screenHeight = height;
  glViewport(0,0,width,height);
}
#endif

void windowEvent( arMasterSlaveFramework& fw, arGUIWindowInfo* windowInfo ) {
  if( !windowInfo )
    return;

  switch( windowInfo->getState() ) {
    case AR_WINDOW_RESIZE:
      screenWidth = windowInfo->getSizeY();
      screenHeight = windowInfo->getSizeX();
      fw.getWindowManager()->setWindowViewport(
        windowInfo->getWindowID(), 0, 0, windowInfo->getSizeX(), windowInfo->getSizeY() );
    break;
  default: // avoid compiler warning
    break;
  }
}

int main(int argc, char** argv){
  twoImages = false;
  switch (argc) {
  case 3:
    twoImages = true;
    rightFile = argv[2];
    // fallthrough
  case 2:
    leftFile = argv[1];
    break;
  default:
    cerr << "usage: StereoViewer left_image right_image\n"
         << "       StereoViewer stereo_pair_image\n";
    return 1;
  }

  arMasterSlaveFramework framework;
  if (!framework.init(argc, argv))
    return 1;

  framework.setStartCallback(init);
  framework.setDrawCallback(display);
  // framework.setReshapeCallback(reshape);
  framework.setWindowEventCallback(windowEvent);
  return framework.start() ? 0 : 1;
}
