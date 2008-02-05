//%Module szggraphics 0

// Defines mapping from python to C++ strings.
%Include typemaps.sip

%Include math.sip
%Include szgclient.sip
%Include opengl.sip

// global extra code
%ModuleHeaderCode
#include "arAxisAlignedBoundingBox.h"
#include "arHead.h"
#include "arTexFont.h"
#include "arMaterial.h"
#include "arLargeImage.h"
#include "arTexture.h"
#include <string>
using namespace std;
%End


void ar_setTextureAllowNotPowOf2( bool onoff );
bool ar_getTextureAllowNotPowOf2();


list<string> ar_parseLineBreaks(const string& text);


class arAxisAlignedBoundingBox {
 public:
  arAxisAlignedBoundingBox();
  ~arAxisAlignedBoundingBox();
  arVector3 center;
  float xSize;
  float ySize;
  float zSize;
};


// Description of a 3D rectangle upon which text can get printed.
// Passed in to the arTexFont renderString command.
// Also includes tab-width info.
class arTextBox {
 public:
  // NOTE: The height of the text box is, essentially, calculated
  // from the physical width and the proportions of the font.
  arTextBox( float w=2.0, int cols=80, float spacing=1.2,
             arVector3 upLeft=arVector3(0,0,0), arVector3 col=arVector3(1,1,1),
             int tabW=2 );
  ~arTextBox();

  float width;
  int columns;
  float lineSpacing;
  arVector3 upperLeft;
  arVector3 color;
  int tabWidth;
};

class arTexFont {
  public:
    arTexFont();
    ~arTexFont();
    
    bool load( const string& fontFilePath,
               int transparentColor=0 );
    void setFontTexture( const arTexture& newFont );
    float characterWidth();
    float lineHeight(arTextBox& format);
    float characterHeight();
    void lineFeed(int& currentColumn /In,Out/, int& currentRow /In,Out/, arTextBox& format );
    void advanceCursor(int& currentColumn, int& currentRow, arTextBox& format);
    void renderGlyph(int c, int& currentColumn /In,Out/, int& currentRow /In,Out/, arTextBox& format);
    float getTextWidth(const string& text, arTextBox& format);
    float getTextHeight(const string& text, arTextBox& format);
    void getTextMetrics(const string& text, arTextBox& format, float& width /Out/, float& height /Out/);
    void getTextMetrics(list<string>& parse, arTextBox& format, float& width /Out/, float& height /Out/);
    void renderString(const string& text, arTextBox& format);
    void renderText(list<string>& parse, arTextBox& format );
    bool renderFile(const string& filename, arTextBox& format);
};


class arMaterial {
 public:
  arMaterial();
  arMaterial(const arMaterial&);
  ~arMaterial();

  arVector3 diffuse;
  arVector3 ambient;
  arVector3 specular;
  arVector3 emissive;
  float exponent;     // i.e. shininess
  float alpha;        // transparency of the material
 
  void activateMaterial();
};


class arTexture {
 public:
  arTexture();
  virtual ~arTexture();
  arTexture( const arTexture& rhs );
  //arTexture& operator=( const arTexture& rhs );
  arTexture( const arTexture& rhs, unsigned int left, unsigned int bottom, 
                                   unsigned int width, unsigned int height );
  //bool operator!() const;
  int __nonzero__() const;
%MethodCode
    sipRes = !!*sipCpp;
%End

  // Reference-counted because of how textures are shared in arGraphicsDatabase.
  int getRef() const;
  arTexture* ref();
  // Caller may not use *this after unref(), just like after ~arTexture().
  void unref(bool debug = false);

  bool activate(bool forceReload = false);
  void deactivate() const;

  int getWidth()  const;
  int getHeight() const;
  int getDepth()  const;
  int numbytes() const;
  //const char* getPixels() const;
  void setPixels(char* pixels, int width, int height);
  //char* getSubImage( unsigned int left, unsigned int bottom, unsigned int width, unsigned int height ) const;
  void setTextureFunc( int texfunc );
  void mipmap(bool fEnable);
  void repeating(bool fEnable);
  void grayscale(bool fEnable);
  bool dummy();

  bool readImage(const string& fileName, 
                 const string& subdirectory, 
                 const string& path,
                 int alpha = -1, bool complain = true);
  bool readPPM(const string& fileName, 
               const string& subdirectory, 
               const string& path,
               int alpha = -1, bool complain = true);
  bool writePPM(const string& fileName); 
  bool writePPM(const string& fileName, const string& path);
  bool writePPM(const string& fileName, const string& subdirectory, 
                const string& path);
  bool readJPEG(const string& fileName, 
                const string& subdirectory, 
                const string& path,
                int alpha = -1, bool complain = true);
  bool writeJPEG(const string& fileName);
  bool writeJPEG(const string& fileName, const string& path);
  bool writeJPEG(const string& fileName, const string& subdirectory, 
                 const string& path);
  
  //bool fill(int w, int h, bool alpha, const char* pixels);
  bool flipHorizontal();
};


class arLargeImage {
public:
  arLargeImage( unsigned int tileWidth=256, unsigned int tileHeight=0 );
  arLargeImage( const arTexture& x, unsigned int tileWidth, unsigned int tileHeight );
  arLargeImage( const arLargeImage& x );
  //arLargeImage& operator=( const arLargeImage& x );
  virtual ~arLargeImage();

  bool setTileSize( unsigned int tileWidth, unsigned int tileHeight=0 );
  void setImage( const arTexture& image );
  unsigned int getWidth() const;
  unsigned int getHeight() const;
  unsigned int numTilesWide() const;
  unsigned int numTilesHigh() const;
  virtual void makeTiles();
  // Indices start at lower left.
  // returned value owned by C++
  arTexture* getTile( unsigned int colNum, unsigned int rowNum );
  virtual void draw();

  arTexture originalImage;
};

