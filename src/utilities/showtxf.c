
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

/* compile line: cc -o showtxf showtxf.c -I/usr/X11R6/include -L/usr/X11R6/lib -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <GL/glut.h>

/* From here to line 351 is a copy and paste job from the original texfont.c
   so that texfont.c does not have to be committed to the cvs repo */
#define TXF_FORMAT_BYTE		0
#define TXF_FORMAT_BITMAP	1

typedef struct {
  unsigned short c;
  unsigned char width;
  unsigned char height;
  signed char xoffset;
  signed char yoffset;
  signed char advance;
  char dummy;
  short x;
  short y;
} TexGlyphInfo;

typedef struct {
  GLfloat t0[2];
  GLshort v0[2];
  GLfloat t1[2];
  GLshort v1[2];
  GLfloat t2[2];
  GLshort v2[2];
  GLfloat t3[2];
  GLshort v3[2];
  GLfloat advance;
} TexGlyphVertexInfo;

typedef struct {
  GLuint texobj;
  int tex_width;
  int tex_height;
  int max_ascent;
  int max_descent;
  int num_glyphs;
  int min_glyph;
  int range;
  unsigned char *teximage;
  TexGlyphInfo *tgi;
  TexGlyphVertexInfo *tgvi;
  TexGlyphVertexInfo **lut;
} TexFont;

typedef struct {
  short width;
  short height;
  short xoffset;
  short yoffset;
  short advance;
  unsigned char *bitmap;
} PerGlyphInfo, *PerGlyphInfoPtr;

typedef struct {
  int min_char;
  int max_char;
  int max_ascent;
  int max_descent;
  PerGlyphInfo glyph[1];
} FontInfo, *FontInfoPtr;

#ifndef GL_VERSION_1_1
#if defined(GL_EXT_texture_object) && defined(GL_EXT_texture)
#define glGenTextures glGenTexturesEXT
#define glBindTexture glBindTextureEXT
#ifndef GL_INTENSITY4
#define GL_INTENSITY4 GL_INTENSITY4_EXT
#endif
int useLuminanceAlpha = 0;
#else
#define USE_DISPLAY_LISTS
/* Intensity texture format not in OpenGL 1.0; added by the EXT_texture
   extension and now part of OpenGL 1.1. */
int useLuminanceAlpha = 1;
#endif
#else
int useLuminanceAlpha = 0;
#endif

/* byte swap a 32-bit value */
#define SWAPL(x, n) { \
                 n = ((char *) (x))[0];\
                 ((char *) (x))[0] = ((char *) (x))[3];\
                 ((char *) (x))[3] = n;\
                 n = ((char *) (x))[1];\
                 ((char *) (x))[1] = ((char *) (x))[2];\
                 ((char *) (x))[2] = n; }

/* byte swap a short */
#define SWAPS(x, n) { \
                 n = ((char *) (x))[0];\
                 ((char *) (x))[0] = ((char *) (x))[1];\
                 ((char *) (x))[1] = n; }

static char *lastError;

char *
txfErrorString(void)
{
  return lastError;
}

TexFont *
txfLoadFont(char *filename)
{
  TexFont *txf;
  FILE *file;
  GLfloat w, h, xstep, ystep;
  char fileid[4], tmp;
  unsigned char *texbitmap;
  int min_glyph, max_glyph;
  int endianness, swap, format, stride, width, height;
  int i, j, got;

  txf = NULL;
  file = fopen(filename, "rb");
  if (file == NULL) {
    lastError = "file open failed.";
    goto error;
  }
  txf = (TexFont *) malloc(sizeof(TexFont));
  if (txf == NULL) {
    lastError = "out of memory.";
    goto error;
  }
  /* For easy cleanup in error case. */
  txf->tgi = NULL;
  txf->tgvi = NULL;
  txf->lut = NULL;
  txf->teximage = NULL;

  got = fread(fileid, 1, 4, file);
  if (got != 4 || strncmp(fileid, "\377txf", 4)) {
    lastError = "not a texture font file.";
    goto error;
  }
  assert(sizeof(int) == 4);  /* Ensure external file format size. */
  got = fread(&endianness, sizeof(int), 1, file);
  if (got == 1 && endianness == 0x12345678) {
    swap = 0;
  } else if (got == 1 && endianness == 0x78563412) {
    swap = 1;
  } else {
    lastError = "not a texture font file.";
    goto error;
  }
#define EXPECT(n) if (got != n) { lastError = "premature end of file."; goto error; }
  got = fread(&format, sizeof(int), 1, file);
  EXPECT(1);
  got = fread(&txf->tex_width, sizeof(int), 1, file);
  EXPECT(1);
  got = fread(&txf->tex_height, sizeof(int), 1, file);
  EXPECT(1);
  got = fread(&txf->max_ascent, sizeof(int), 1, file);
  EXPECT(1);
  got = fread(&txf->max_descent, sizeof(int), 1, file);
  EXPECT(1);
  got = fread(&txf->num_glyphs, sizeof(int), 1, file);
  EXPECT(1);

  if (swap) {
    SWAPL(&format, tmp);
    SWAPL(&txf->tex_width, tmp);
    SWAPL(&txf->tex_height, tmp);
    SWAPL(&txf->max_ascent, tmp);
    SWAPL(&txf->max_descent, tmp);
    SWAPL(&txf->num_glyphs, tmp);
  }
  txf->tgi = (TexGlyphInfo *) malloc(txf->num_glyphs * sizeof(TexGlyphInfo));
  if (txf->tgi == NULL) {
    lastError = "out of memory.";
    goto error;
  }
  assert(sizeof(TexGlyphInfo) == 12);  /* Ensure external file format size. */
  got = fread(txf->tgi, sizeof(TexGlyphInfo), txf->num_glyphs, file);
  EXPECT(txf->num_glyphs);

  if (swap) {
    for (i = 0; i < txf->num_glyphs; i++) {
      SWAPS(&txf->tgi[i].c, tmp);
      SWAPS(&txf->tgi[i].x, tmp);
      SWAPS(&txf->tgi[i].y, tmp);
    }
  }
  txf->tgvi = (TexGlyphVertexInfo *)
    malloc(txf->num_glyphs * sizeof(TexGlyphVertexInfo));
  if (txf->tgvi == NULL) {
    lastError = "out of memory.";
    goto error;
  }
  w = txf->tex_width;
  h = txf->tex_height;
  xstep = 0.5 / w;
  ystep = 0.5 / h;
  for (i = 0; i < txf->num_glyphs; i++) {
    TexGlyphInfo *tgi;

    tgi = &txf->tgi[i];
    txf->tgvi[i].t0[0] = tgi->x / w + xstep;
    txf->tgvi[i].t0[1] = tgi->y / h + ystep;
    txf->tgvi[i].v0[0] = tgi->xoffset;
    txf->tgvi[i].v0[1] = tgi->yoffset;
    txf->tgvi[i].t1[0] = (tgi->x + tgi->width) / w + xstep;
    txf->tgvi[i].t1[1] = tgi->y / h + ystep;
    txf->tgvi[i].v1[0] = tgi->xoffset + tgi->width;
    txf->tgvi[i].v1[1] = tgi->yoffset;
    txf->tgvi[i].t2[0] = (tgi->x + tgi->width) / w + xstep;
    txf->tgvi[i].t2[1] = (tgi->y + tgi->height) / h + ystep;
    txf->tgvi[i].v2[0] = tgi->xoffset + tgi->width;
    txf->tgvi[i].v2[1] = tgi->yoffset + tgi->height;
    txf->tgvi[i].t3[0] = tgi->x / w + xstep;
    txf->tgvi[i].t3[1] = (tgi->y + tgi->height) / h + ystep;
    txf->tgvi[i].v3[0] = tgi->xoffset;
    txf->tgvi[i].v3[1] = tgi->yoffset + tgi->height;
    txf->tgvi[i].advance = tgi->advance;
  }

  min_glyph = txf->tgi[0].c;
  max_glyph = txf->tgi[0].c;
  for (i = 1; i < txf->num_glyphs; i++) {
    if (txf->tgi[i].c < min_glyph) {
      min_glyph = txf->tgi[i].c;
    }
    if (txf->tgi[i].c > max_glyph) {
      max_glyph = txf->tgi[i].c;
    }
  }
  txf->min_glyph = min_glyph;
  txf->range = max_glyph - min_glyph + 1;

  txf->lut = (TexGlyphVertexInfo **)
    calloc(txf->range, sizeof(TexGlyphVertexInfo *));
  if (txf->lut == NULL) {
    lastError = "out of memory.";
    goto error;
  }
  for (i = 0; i < txf->num_glyphs; i++) {
    txf->lut[txf->tgi[i].c - txf->min_glyph] = &txf->tgvi[i];
  }

  switch (format) {
  case TXF_FORMAT_BYTE:
    if (useLuminanceAlpha) {
      unsigned char *orig;

      orig = (unsigned char *) malloc(txf->tex_width * txf->tex_height);
      if (orig == NULL) {
        lastError = "out of memory.";
        goto error;
      }
      got = fread(orig, 1, txf->tex_width * txf->tex_height, file);
      EXPECT(txf->tex_width * txf->tex_height);
      txf->teximage = (unsigned char *)
        malloc(2 * txf->tex_width * txf->tex_height);
      if (txf->teximage == NULL) {
        lastError = "out of memory.";
        goto error;
      }
      for (i = 0; i < txf->tex_width * txf->tex_height; i++) {
        txf->teximage[i * 2] = orig[i];
        txf->teximage[i * 2 + 1] = orig[i];
      }
      free(orig);
    } else {
      txf->teximage = (unsigned char *)
        malloc(txf->tex_width * txf->tex_height);
      if (txf->teximage == NULL) {
        lastError = "out of memory.";
        goto error;
      }
      got = fread(txf->teximage, 1, txf->tex_width * txf->tex_height, file);
      EXPECT(txf->tex_width * txf->tex_height);
    }
    break;
  case TXF_FORMAT_BITMAP:
    width = txf->tex_width;
    height = txf->tex_height;
    stride = (width + 7) >> 3;
    texbitmap = (unsigned char *) malloc(stride * height);
    if (texbitmap == NULL) {
      lastError = "out of memory.";
      goto error;
    }
    got = fread(texbitmap, 1, stride * height, file);
    EXPECT(stride * height);
    if (useLuminanceAlpha) {
      txf->teximage = (unsigned char *) calloc(width * height * 2, 1);
      if (txf->teximage == NULL) {
        lastError = "out of memory.";
        goto error;
      }
      for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
          if (texbitmap[i * stride + (j >> 3)] & (1 << (j & 7))) {
            txf->teximage[(i * width + j) * 2] = 255;
            txf->teximage[(i * width + j) * 2 + 1] = 255;
          }
        }
      }
    } else {
      txf->teximage = (unsigned char *) calloc(width * height, 1);
      if (txf->teximage == NULL) {
        lastError = "out of memory.";
        goto error;
      }
      for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
          if (texbitmap[i * stride + (j >> 3)] & (1 << (j & 7))) {
            txf->teximage[i * width + j] = 255;
          }
        }
      }
    }
    free(texbitmap);
    break;
  }

  fclose(file);
  return txf;

error:

  if (txf) {
    if (txf->tgi)
      free(txf->tgi);
    if (txf->tgvi)
      free(txf->tgvi);
    if (txf->lut)
      free(txf->lut);
    if (txf->teximage)
      free(txf->teximage);
    free(txf);
  }
  if (file)
    fclose(file);
  return NULL;
}

unsigned char *raster;
int imgwidth, imgheight;
int max_ascent, max_descent;
int len;
int ax = 0, ay = 0;
int doubleBuffer = 1, verbose = 0;
char *filename = "default.txf";
TexFont *txf;

/* If resize is called, enable drawing into the full screen area
   (glViewport). Then setup the modelview and projection matrices to map 2D
   x,y coodinates directly onto pixels in the window (lower left origin).
   Then set the raster position (where the image would be drawn) to be offset
   from the upper left corner, and then offset by the current offset (using a
   null glBitmap). */
void
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, 0, h);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0, h - imgheight, 0);
  glRasterPos2i(0, 0);
  glBitmap(0, 0, 0, 0, ax, -ay, NULL);
}

void
display(void)
{
  /* Clear the color buffer. */
  glClear(GL_COLOR_BUFFER_BIT);

  /* Re-blit the image. */
  glDrawPixels(imgwidth, imgheight,
    GL_LUMINANCE, GL_UNSIGNED_BYTE,
    txf->teximage);

  /* Swap the buffers if necessary. */
  if (doubleBuffer) {
    glutSwapBuffers();
  }
}

static int moving = 0, ox, oy;

void
mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {

      /* Left mouse button press.  Update last seen mouse position. And set
         "moving" true since button is pressed. */
      ox = x;
      oy = y;
      moving = 1;

    } else {

      /* Left mouse button released; unset "moving" since button no longer
         pressed. */
      moving = 0;

    }
  }
}

void
motion(int x, int y)
{
  /* If there is mouse motion with the left button held down... */
  if (moving) {

    /* Figure out the offset from the last mouse position seen. */
    ax += (x - ox);
    ay += (y - oy);

    /* Offset the raster position based on the just calculated mouse position
       delta.  Use a null glBitmap call to offset the raster position in
       window coordinates. */
    glBitmap(0, 0, 0, 0, x - ox, oy - y, NULL);

    /* Request a window redraw. */
    glutPostRedisplay();

    /* Update last seen mouse position. */
    ox = x;
    oy = y;
  }
}

int
main(int argc, char **argv)
{
  int i;

  glutInit(&argc, argv);
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-sb")) {
      doubleBuffer = 0;
    } else if (!strcmp(argv[i], "-v")) {
      verbose = 1;
    } else {
      filename = argv[i];
    }
  }
  if (filename == NULL) {
    fprintf(stderr, "usage: showtxf [GLUT-options] [-sb] [-v] txf-file\n");
    exit(1);
  }

  txf = txfLoadFont(filename);
  if (txf == NULL) {
    fprintf(stderr, "Problem loading %s\n", filename);
    exit(1);
  }

  imgwidth = txf->tex_width;
  imgheight = txf->tex_height;

  if (doubleBuffer) {
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  } else {
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
  }
  glutInitWindowSize(imgwidth, imgheight);
  glutCreateWindow(filename);
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  /* Use a gray background so teximage with black backgrounds will show
     against showtxf's background. */
  glClearColor(0.2, 0.2, 0.2, 1.0);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
