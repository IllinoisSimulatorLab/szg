//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************
#include "arPrecompiled.h"
#include "arMasterSlaveFramework.h"
#include "arMath.h"
#include "arGlutRenderFuncs.h"
#ifndef AR_USE_WIN_32
#include <sys/types.h>
#include <signal.h>
#endif

#include <list>

const float eraserRadius = 3. / 12.; // 3 inches

class stroke {
public:
  int iColor; // later, material instead of color
  list <arVector3> l;

  stroke() { clear(); }
  void clear() { iColor = -1; l.clear(); }
  void append(const arVector3& v);
  void save(ofstream& fp) const;
  bool load(ifstream& fp);
};

void stroke::append(const arVector3& v)
{
  if (!l.empty()) {
    const float resolution = 0.75 / 12.; // 0.75 inches
    const arVector3 previous = l.back();
    const float distance = (v - previous).magnitude();
    if (distance < resolution)
      // v is so close to previous that it's redundant.  Ignore it.
      return;

    if (distance > eraserRadius) {
      // Insert points between previous and v, so erasing works better.
      const arVector3 diff = v - previous;
      const float steps = ceil(distance / (eraserRadius * .8)); // .8 fudge factor
      const float dt = 1. / steps;
      for (float t=dt; t<1.; t += dt) {
        const arVector3 vLerp = previous + (t * diff);
        if ((vLerp - v).magnitude() > resolution) {
          // not too close to v
          l.push_back(vLerp);
        }
      }
    }
  }

  // Insert v.
  l.push_back(v);
}

list<stroke> strokes;
list<stroke> fragments;
arLock lockSculpture; // Guards strokes and fragments, when drawing or changing or loading or saving.
stroke sCur;
typedef list <arVector3>::const_iterator citer;
typedef list <arVector3>::iterator iter;

void EraseRibbon(stroke* s, const arVector3& v)
{
  // Remove any points within threshold distance of v.
  // Also split the ribbon into separate ones.

Lagain:
  list <arVector3>& l = s->l;

  // Mark points that need erasing.
  int i = 0;
  for (iter ii = l.begin(); ii != l.end(); ++ii,++i) {
    if ((*ii - v).magnitude() < eraserRadius) {
      // ii needs removing!

      if (ii == l.begin()) {
//                                                        printf("erase head\n");
        // Just lop it off the beginning.
        l.erase(ii);
        goto Lagain;
      }

//                                                        printf("erase nonhead\n");
      iter iDel = ii++;
      l.erase(iDel);
      if (ii == l.end()) {
//                                                        printf("      was tail\n");
        break;
      }

      stroke *t = new stroke;
      t->iColor = s->iColor; // "copy constructor"

      // move [ii, l.end) from l to t->l, at t->l's head.
      t->l.splice(t->l.begin(), l, ii, l.end());

//printf("oldlen %d, newlen %d\n", l.size(), t->l.size());

      if (i <= 3 /*l.size() <= 3*/) {
        // mark old stroke for deletion
//                                                        printf("      prune %d, shortcut %d\n", l.size(), i);
        l.clear();
      }

      if (t->l.size() <= 3) {
//                                                        printf("      was almost-tail\n");
        delete t;
        break;
      }

//                                                        printf("frags += stroke of size %d\n", t->l.size());
      fragments.push_back(*t);

      // The loop would try to march on in t instead of in s,
      // so do it legitimately.  Tail-recursion.
      s = t;
      goto Lagain;
    }
  }
}


// Ribbon drawing routine, originally by Chris Hartman 6/1994
// Modified and encapsulated by Camille Goudeseune 7/1994 - 4/1996
// Modified some more and ported to Syzygy by Camille Goudeseune 4/2003

void DrawRibbon(const list <arVector3>& PT, float zWidth, int wLowpass)
{
  const int cpt = PT.size(); // size() isn't constant time!
  if (cpt < 3)
    return;
  if (wLowpass <= 0)
    return;

  arVector3 bnPrev(0,0,1);
  arVector3 prev1, prev2;

  glBegin(GL_TRIANGLE_STRIP);

  // ;;todo precompute all this (if fDirty) and cache a raw float[][3] to pass to tstrip.

  citer ii = PT.begin(); ++ii;
  for (int i=0; i<cpt-1; ++i, ++ii)
    {
    int jPrev, jNext;
    citer iiPrev = ii;
    const arVector3 PTcur = *ii;
    for (jPrev=0; jPrev<wLowpass && iiPrev != PT.begin(); ++jPrev)
      --iiPrev;
    citer iiNext = ii;
    citer iiMax = PT.end();  --iiMax;
    for (jNext=0; jNext<wLowpass && iiNext != iiMax; ++jNext)
      ++iiNext;
    if (jPrev == 0 && jNext == 0) {
      cerr << "ribbons: internal error.\n";
      break;
    }
    // Fake a PTprev and PTnext at the ends of the ribbon.
    const arVector3 PTprev = jPrev==0 ? (*ii + *ii - *iiNext) : *iiPrev;
    const arVector3 PTnext = jNext==0 ? (*ii + *ii - *iiPrev) : *iiNext;

    arVector3 bn = (PTprev - PTcur) * (PTnext * PTcur); // binormal
LAgain:
    float length = bn.magnitude();
    if (length <= 0.02)
      {
      // No sufficiently unique binormal, so use previous one
      if (i < 3)
        // Unless there isn't a previous one yet.
        continue;
      bn = bnPrev;
      goto LAgain;
      }

    //;; Clamp bn's slew rate.
    arVector3 nn = bn * (PTnext - PTprev); // normal
    bn *= zWidth / length;
    bnPrev = bn;
    length = nn.magnitude();
    if (length >= .01) {
      // This might happen while erasing?
      nn /= length;
      glNormal3fv(nn.v);
    }

    const arVector3 tmp1 = PTcur + bn;
    const arVector3 tmp2 = PTcur - bn;

    /* Prevent ribbon from flipping over, by choosing the
     * order of the next 2 points in the triangle mesh
     * to minimize the sum of the lengths of the edges
     * of the ribbon.
     */
    const float distA = (prev1 - tmp1).magnitude() + (prev2 - tmp2).magnitude();
    const float distB = (prev1 - tmp2).magnitude() + (prev2 - tmp1).magnitude();
    if (distA < distB)
      {
      prev1 = tmp1;
      prev2 = tmp2;
      }
    else
      {
      prev1 = tmp2;
      prev2 = tmp1;
      }
    glVertex3fv(prev1.v);
    glVertex3fv(prev2.v);

    }
  glVertex3fv(prev1.v);
  glVertex3fv(prev2.v);
  glEnd();
}

void initGL( arMasterSlaveFramework&, arGUIWindowInfo*) {
  const float ambient[] = {0.07, 0.07, 0.07, 1.0};
  const float diffuse[4][4] = {
    { 0.0,  0.4,  0.6, 1.0},
    { 0.0,  0.6,  0.4, 1.0},
    { 0.4,  0.6,  0.0, 1.0},
    { 0.6,  0.4,  0.0, 1.0}};
  const float specular[4][4] = {
    { 0.9,  0.0,  0.4, 1.0},
    { 0.4,  0.7,  0.0, 1.0},
    { 0.0,  0.7,  0.4, 1.0},
    { 0.4,  0.0,  0.9, 1.0}};

  const float mat_ambient[] = {0.3, 0.2, 0.1, 1.0};
  const float mat_diffuse[] = {0.4, 0.6, 0.7, 1.0};
  const float mat_specular[] = {0.2, 0.9, 0.7, 1.0};
  const float mat_shininess[] = {22.0}; // 0 5 100 is none low high
  const float mat_emission[] = {0.2, 0.1, 0.1, 0.0};

  const float lmodel_localviewer[] = {0.0}; // ;; 1.0 still diffs on theora-wall.
  const float lmodel_twoside[] = {0.0}; // 0 iff both sides are rendered the same.
  const float lmodel_ambient[] = {.5, .5, .5, 1.0};

  glDepthFunc(GL_LEQUAL);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_COLOR_MATERIAL); // does wild stuff if enabled, rtfm explains.
  glDisable(GL_CULL_FACE);
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_BLEND);

  glDisable(GL_NORMALIZE); // we normalize already
  glShadeModel(GL_SMOOTH);

  for (int i=0; i<=3; ++i) {
    glLightfv(GL_LIGHT0+i, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0+i, GL_DIFFUSE, diffuse[i]);
    glLightfv(GL_LIGHT0+i, GL_SPECULAR, specular[i]);
    glEnable(GL_LIGHT0+i);
  }
  glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_localviewer);
  glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
  glEnable(GL_LIGHTING);

  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
  glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mat_emission);
}

bool init(arMasterSlaveFramework&, arSZGClient&){
  return true;
}

void scaleLights(float scalar) {
  if (scalar <= 0.0)
    return;

  const float slewClamp = 0.96;
  if (scalar > 1.0/slewClamp)
    scalar = 1.0/slewClamp;
  if (scalar < slewClamp)
    scalar = slewClamp;

  static float x = 0.1;
  x *= scalar;
  if (x >= .99)
    x = .99;
  // printf("%.2f\n", x);
  const float mat_ambient[] = {x*.8, x*.9, x   , 1.0};
  const float mat_diffuse[] = {x   , x*.9, x*.8, 1.0};
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
}

void moveLights() {
  // Two lights above, two below, spaced tetrahedrally.
  static float pos[4][4] = {
    { 1.0,  1.0, -1.0, 0.0},
    {-1.0,  1.0,  1.0, 0.0},
    { 1.0, -1.0,  1.0, 0.0},
    {-1.0, -1.0, -1.0, 0.0}};

  // Rotate about vertical axis.
  static float a = 0.; // angle
  a += M_PI*2. / 6000.; // 100fps max means 60 seconds per rotation.
  if (a > M_PI*2.)
    a -= M_PI*2.;
  const float ca = cos(a);
  const float sa = sin(a);
  pos[0][0] = ca;
  pos[0][2] = sa;
  pos[1][0] = -ca;
  pos[1][2] = -sa;
  pos[2][0] = -sa;
  pos[2][2] = ca;
  pos[3][0] = sa;
  pos[3][2] = -ca;

  {
  // Tweak height for more variety.
  static float a = 0.; // angles
  static float b = 0.; // angles
  static float c = 0.; // angles
  static float d = 0.; // angles
  a += M_PI*2. / 4500.;
  b += M_PI*2. / 4500. * sqrt(2.f);
  c += M_PI*2. / 4500. * sqrt(3.f);
  d += M_PI*2. / 4500. / sqrt(5.f);
  if (a > M_PI*2.) a -= M_PI*2.;
  if (b > M_PI*2.) b -= M_PI*2.;
  if (c > M_PI*2.) c -= M_PI*2.;
  if (d > M_PI*2.) d -= M_PI*2.;
  pos[0][1] =  1. + .5 * cos(a);
  pos[1][1] =  1. - .5 * cos(b);
  pos[2][1] = -1. + .5 * sin(c);
  pos[3][1] = -1. - .5 * sin(d);
  }

  // Update OpenGL.
  for (int i=0; i<=3; ++i)
    glLightfv(GL_LIGHT0+i, GL_POSITION, pos[i]);
}

#ifdef UNUSED
  struct onepoint : public unary_function<const arVector3&, void>
  {
    onepoint() { glBegin(GL_LINE_STRIP); }
    void operator()(const arVector3& v) { glVertex3fv(v.v); }
    ~onepoint() { glEnd(); }
  };
  struct drawStrokeAsLine : public unary_function<stroke, void>
  {
    void operator()(const stroke& s) { for_each(s.l.begin(), s.l.end(), onepoint()); }
  }
#endif

// Serialize to/from disk.  4-byte int color, then 4-byte int count of points, then the points.
void stroke::save(ofstream& fp) const
{
  ar_log_debug() << "Writing a stroke with " << l.size() << " points.\n";
  fp.write((const char*)&iColor, sizeof iColor);
  if (!fp) ar_log_error() << "saved stroke is corrupt.\n";
  const int c = l.size();
  fp.write((const char*)&c, sizeof c);
  if (!fp) ar_log_error() << "saved stroke is corrupt.\n";
  for (list<arVector3>::const_iterator it = l.begin(); it != l.end(); ++it) {
    fp.write((const char*)&it->v, sizeof it->v);
    if (!fp) ar_log_error() << "saved stroke is corrupt.\n";
  }
}
bool stroke::load(ifstream& fp)
{
  fp.read((char*)&iColor, sizeof iColor);
  if (fp.eof()) { ar_log_debug() << "Reached EOF while loading strokes.\n"; return false; }
  if (!fp) { ar_log_error() << "Error while loading strokes.\n"; return false; }
  int c;
  fp.read((char*)&c, sizeof c);
  if (fp.eof()) { ar_log_debug() << "Reached EOF while loading strokes.\n"; return false; }
  if (!fp) { ar_log_error() << "Error while loading strokes.\n"; return false; }
  if (c <= 0)
    return false;
  // For sculptures with millions of points, one fread would be faster than this loop.
  arVector3 v;
  for (int i=0; i<c; ++i) {
    assert(sizeof v.v == 3*4); // 3 * sizeof(float)
    fp.read((char*)&v.v, sizeof v.v);
    if (fp.eof()) { ar_log_debug() << "Unexpected EOF while loading strokes.\n"; return false; }
    if (!fp) { ar_log_error() << "Error while loading strokes.\n"; return false; }
    l.push_back(v);
  }
  ar_log_debug() << "Loaded a stroke with " << l.size() << " points.\n";
  return true;
}

struct drawStroke : public unary_function<stroke, void>
{
  void operator()(const stroke& s) { DrawRibbon(s.l, 0.42, 12); }
};

struct eraseStroke : public unary_function<stroke, void>
{
  const arVector3& _v;
  eraseStroke(const arVector3& v) : _v(v) {}
  void operator()(stroke& s) { EraseRibbon(&s, _v); }
};

struct saveStroke : public unary_function<stroke, void>
{
  ofstream& _fp;
  saveStroke(ofstream& fp) : _fp(fp) {}
  void operator()(stroke& s) { s.save(_fp); }
};

// Background, for more brightness and for greater 3Dishness.
void drawStars()
{
  glDisable(GL_LIGHTING);
  glEnable(GL_POINT_SMOOTH);
  glColor3f(1.,1.,1.);
  srand(42);
  glBegin(GL_POINTS);
  for (int i=0; i<1000; ++i) {
    if (i % 100 == 0) {
      glEnd();
      glPointSize(1 + i/300.);
      glBegin(GL_POINTS);
    }
    const float x = (rand() % 10000) / 10000. * 100. - 50;
    const float y = (rand() % 10000) / 10000. * 100. - 50;
    const float z = (rand() % 10000) / 10000. * 100. - 50;
    if (fabs(x)<20. && fabs(z)<20. && y>-15. && y<25.)
      // reject points inside or nearly inside Cube
      continue;
    glVertex3f(x, y, z);
  }
  glEnd();

  glDisable(GL_POINT_SMOOTH);
  glEnable(GL_LIGHTING);
}

void drawSculpture()
{
  glPushMatrix();
    // Draw previously made strokes
    // Set different material here if you like.
    lockSculpture.lock();
      for_each(strokes.begin(), strokes.end(), drawStroke());
    lockSculpture.unlock();

    // Draw stroke in progress, if any.
    // Set different material here if you like.
    static drawStroke tmp; tmp(sCur);
  glPopMatrix();
}

const std::string filename =
#ifdef AR_USE_WIN_32
  "C:\\szg\\ribbons-sculpture.pickle";
#else
  "/tmp/ribbons-sculpture.pickle";
#endif

void LoadSculpture()
{
  ifstream fp("/tmp/ribbons-sculpture.pickle", ios::in | ios::binary);
  if (!fp.is_open()) {
    ar_log_remark() << "No sculpture file to load.  Starting with a blank canvas.\n";
    return;
  }
  lockSculpture.lock();
    // Read the file, one stroke at a time.
    stroke s;
    while (s.load(fp)) {
      ar_log_debug() << "loaded a stroke.\n";
      strokes.push_back(s);
      s.clear();
    }
  lockSculpture.unlock();
  fp.close();
}

void SaveSculpture()
{
  ofstream fp("/tmp/ribbons-sculpture.pickle", ios::out | ios::binary);
  if (!fp.is_open()) {
    ar_log_error() << "Corrupted sculpture file " + filename + ".\n";
    return;
  }
  lockSculpture.lock();
    for_each(strokes.begin(), strokes.end(), saveStroke(fp));
  lockSculpture.unlock();
  fp.close();
}

void cleanup(arMasterSlaveFramework&)
{
  SaveSculpture();
}

#if 0
struct emptyStroke : public unary_function<stroke, void>
{
  bool operator()(const stroke& s) { return s.l.size() <= 4; }
};
#endif
void erase(const arVector3& v)
{
  lockSculpture.lock();
    for_each(strokes.begin(), strokes.end(), eraseStroke(v));

#if 0
    // Unpatched Visual C++ 6 rejects this correct STL.
    // Syzygy should compile to unpatched Visual C++ 6, a gold standard of compatibility.
    strokes.remove_if(emptyStroke());
#else
    // The equivalent, in long-hand.
    for (list<stroke>::iterator it = strokes.begin(); it != strokes.end(); ) {
      if ( (*it).l.size() <= 4 )
        it = strokes.erase(it);
      else
        ++it;
    }
#endif

#ifdef VERBOSE
      {
        printf("\t\tstrokes %lu, fragments %lu\n", strokes.size(), fragments.size());
        list<stroke>::const_iterator i;
        for (i = fragments.begin(); i != strokes.end(); ++i)
        for (; i != fragments.end(); ++i)
          printf("\t\t\t\t\t\tfragsize %lu\n", i->l.size());
        printf("\t\t\t\t\t\t\tstrokesize ");
        for (i = strokes.begin(); i != strokes.end(); ++i)
          printf("%lu ", i->l.size());
        printf("\n");
      }
#endif
    strokes.splice(strokes.begin(), fragments);
  lockSculpture.unlock();
}

static enum { cursorDefault, cursorDraw, cursorErase } cursor;

void drawCursor(const arVector3& v)
{
  glDisable(GL_LIGHTING);
  glLineWidth(2);
  switch (cursor) {

  case cursorDefault:
    // Little ball.
    glColor3f(1,1,1);
    glPushMatrix();
      glTranslatef(v.v[0], v.v[1], v.v[2]);
      ar_glutWireSphere(.1,6,6);
    glPopMatrix();
    break;

  case cursorDraw:
    // Little ball,...
    glColor3f(.7,.7,.7);
    glPushMatrix();
      glTranslatef(v.v[0], v.v[1], v.v[2]);
      ar_glutWireSphere(.1,6,6);
    glPopMatrix();
    // ...skewered by xyz axes.
    glColor3f(1,1,1);
    glBegin(GL_LINES);
      {
      const GLfloat _ = 6.;
      glVertex3f(-_, v.v[1], v.v[2]);
      glVertex3f( _, v.v[1], v.v[2]);
      glVertex3f(v.v[0], 5.-_, v.v[2]);
      glVertex3f(v.v[0], 5.+_, v.v[2]);
      glVertex3f(v.v[0], v.v[1], -_);
      glVertex3f(v.v[0], v.v[1],  _);
      }
    glEnd();
    break;

  case cursorErase:
    // Large wireframe sphere.
    glColor3f(1,1,1);
    glPushMatrix();
      glTranslatef(v.v[0], v.v[1], v.v[2]);
      ar_glutWireSphere(eraserRadius*1.5,8,8);
      // *1.5: compensate for split ribbons' truncated ends
    glPopMatrix();
    break;

  }
  glEnable(GL_LIGHTING);
}

void update(const int* fDown, const arVector3& wand)
{
  static bool fDraw = false;
  static bool fErase = false;
  static int fDownPrev[3] = {0};

  const bool fDrawBgn = fDown[0] && !fDownPrev[0] && !fErase;
  const bool fDrawEnd = !fDown[0] && fDownPrev[0] && !fErase;
  const bool fEraseBgn = fDown[1] && !fDownPrev[1] && !fDraw;
  const bool fEraseEnd = !fDown[1] && fDownPrev[1] && !fDraw;

  if (fDrawBgn) {
    cursor = cursorDraw;
    sCur.clear();
    fDraw = true;
  }
  if (fDraw)
    sCur.append(wand);
  if (fDrawEnd) {
    lockSculpture.lock();
      strokes.push_back(sCur);
    lockSculpture.unlock();
    sCur.clear();
    cursor = cursorDefault;
    fDraw = false;
  }

  if (fEraseBgn) {
    cursor = cursorErase;
    fErase = true;
  }
  if (fErase) {
    // Delete all points within range, and split as needed.
    erase(wand);
  }
  if (fEraseEnd) {
    cursor = cursorDefault;
    fErase = false;
  }

  memcpy(fDownPrev, fDown, sizeof(fDownPrev));
}

void postExchange(arMasterSlaveFramework& fw) {
  const int wandButton[3] = { fw.getButton(0), fw.getButton(1), fw.getButton(2) };
  update(wandButton, fw.getMatrix(1) * arVector3(0,0,-1));
}

// Count nonblack pixels of each brightness.
const int bins = 90;
int histogram[bins];
int cNonblack = 0;

void parseVertex(GLfloat* buf, GLint size, GLint& c) {
  GLfloat tmp[7]; // xyzrgba
  for (int i=0; i<7; ++i)
    tmp[i] = buf[size - c--];

//const float bright = (tmp[3] + tmp[4] + tmp[5]) / 3.; // avg of RGB

  float bright = tmp[3]; // max of RGB
  if (tmp[4] > bright)
    bright = tmp[4];
  if (tmp[5] > bright)
    bright = tmp[5];

  if (bright > 0.) {
    int j = int(floor(bright * (bins+1)));
    if (j < 0)
      j = 0;
    if (j > bins-1)
      j = bins-1;
    histogram[j]++;
    cNonblack++;
  }
}

float parse(GLfloat* buf, GLint size) {

  // clear histogram
  memset(histogram, 0, sizeof(histogram));
  cNonblack = 0;

  // parse
  GLint c = size;
  while (c) {
    switch (int(buf[size - c--])) {
      case GL_POINT_TOKEN:
      case GL_BITMAP_TOKEN:
      case GL_DRAW_PIXEL_TOKEN:
      case GL_COPY_PIXEL_TOKEN:
        c -= 7;
        break;
      case GL_LINE_TOKEN:
      case GL_LINE_RESET_TOKEN:
        c -= 14;
        break;
      case GL_PASS_THROUGH_TOKEN:
        c -= 1;
        break;
      default:
        break;
      case GL_POLYGON_TOKEN:
        int nvertices = int(buf[size - c--]);
        while (nvertices-- > 0)
          parseVertex(buf, size, c);
      break;
    }
  }

  if (cNonblack == 0)
    return -1;

  // find (nearly-)greatest brightness
  int i;
  for (i=bins-1; i>=0; --i) {
    if (histogram[i] > 0)
      break;
  }
  const float brightest = i / float(bins);
  const float scalar = .9 / brightest;

#if 0
  // print histogram
  printf("%5d --- ", cNonblack);
  for (i=0; i<bins; ++i) {
    const int j = histogram[i];
    putchar(j==0 ? '.' : 'A' + int(25. * j / float(cNonblack)));
  }
  printf(" %.2f\n", scalar);
#endif

  return scalar;
}

#define USING_FEEDBACK
#ifdef USING_FEEDBACK
const int feedMax = 10000000; //;;overflow
GLfloat feedBuffer[feedMax];
#endif

void display(arMasterSlaveFramework& fw) {
  moveLights();

  // sculpture
#ifdef USING_FEEDBACK
  //;; todo: use average of all screens' scalars, not each their own.
  glFeedbackBuffer(feedMax, GL_3D_COLOR, feedBuffer); // lit untextured, so save xyzrgba.
  (void)glRenderMode(GL_FEEDBACK);
  drawSculpture();
  const int size = glRenderMode(GL_RENDER);
  const float scalar = parse(feedBuffer, size);
  scaleLights(scalar);
#endif
  drawSculpture();

  // background
  drawStars();

  // wand
  drawCursor(fw.getMatrix(1) * arVector3(0,0,-1));
}

int main(int argc, char** argv){
  arMasterSlaveFramework fw;
  fw.setWindowStartGLCallback(initGL);
  fw.setStartCallback(init);
  fw.setPostExchangeCallback(postExchange);
  fw.setDrawCallback(display);
  fw.setExitCallback(cleanup);
  LoadSculpture();
  return fw.init(argc, argv) && fw.start() ? 0 : 1;
}
