// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#ifndef AR_USE_WIN_32
  #include <netinet/in.h>
#else
  #include <windows.h>
#endif
#include <math.h>
#include "arGraphicsHeader.h"
#include "arMath.h"
#include "arMasterSlaveFramework.h"
#ifndef M_PI
#define M_PI 3.14159
#endif

#define MALLOC(x,y) malloc((x))
#define NewN(type, count,y)  (type *)MALLOC((count) * sizeof(type),(y))
/****************************************************************/
/****  optiVerse a.k.a. avn, slevy, 31dec 1997               ****/
/****            equiVert a.k.a. nvn,  August 1997           ****/
/****       Chris Hartman, merged  noosh.c wth vn.c          ****/
/****       co-developers: Alex Bourd, Glenn Chappell        ****/
/****       John Sullivan, and George Francis                ****/
/**** (C) 1997, Board of Trustees, U. Illinois.              ****/
/****************************************************************/

#define MAXTOPES 1200                  /* that fit into the viewer */
#ifdef MAX
#undef MAX
#endif
#ifdef MIN
#undef MIN
#endif
#define  MAX(x,y)       (((x)<(y))?(y):(x))
#define  MIN(x,y)       (((x)>(y))?(y):(x))
#define  ABS(x)         (((x)<0)?-(x):(x))
#define  FOR(a,b,c)     for(a=b;a<c;a++)
#define  DOT(p,q)       ((p).x[0]*(q).x[0]+(p).x[1]*(q).x[1]+(p).x[2]*(q).x[2])
#define	 VZERO(v)	memset(&(v).x[0], 0, sizeof(Point))
#define  NRM(p)         fsqrt(DOT(p,p))
#define  DG            M_PI/180
#define  S(u)          fsin(u*DG)
#define  C(u)          fcos(u*DG)
#define  CLAMP(x,u,v) (x<u? u : (x>v ? v: x))

typedef struct MPOINT { float x[3]; } Point;
typedef float Matrix1[4*4];

typedef struct LOOP {
	struct LOOP *next;
	int closed;
	int nverts;
	Point *verts;
	Point (*xyv)[2]; /* Basis vectors for tube cross section */
} Loop;

typedef struct TOPE {
	char *comment;
	int nverts;
	Point *verts;
	Point min, size;	/* Bounding box; "size" guaranteed nonzero */
	int nfaces;
	Point *norm;		/* norm[nfaces] : facet normals */
	Point *vnorm;		/* vnorm[verts] : vertex normals */
	int *nfv;		/* nfv[nfaces] : number of verts on each face */
	int *fv0;		/* fv0[nfaces] : starting index in fv[] */
	int totfv;		/* sum of nfv over all faces */
	int *fv;		/* fv[totfv] : vertex indices per face */
	unsigned char *fromdl;	/* fromdl[nfaces] : graph distance from double-locus curve */
	int *fadj;		/* fadj[totfv] : faceno adjacent on */
				/*   facevert i..i+1 edge (-1 if none) */

	int nedgeverts;		/* number of verts on self-intersection locus */
	Point *edges;		/* edges[nedgeverts]; each from edges[2*i] to edges[2*i+1] */
	Loop *loops;
} Tope;


//***************************************************************
// added variables for syzygy
//***************************************************************
bool rotateObject = true;

arMatrix4 worldMatrix;

double starttime;
string dataPath;
int animateCounter = 0;

bool animatingTopes = false;
//***************************************************************

int ribbonflag = 0;  
char astring[255];
int phase = 0;

Matrix1 id = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

Matrix1 aff;
int currentTope = 0;
int ntopes;
int rotn;
int nhash;
int* ht;
int *_paired, _tope;  Point *_segs;

TOPE topes[MAXTOPES];

int debug = 0;
int quiet = 1;
char *audfname = "trance.aud";
int nosproc = 0;
float phase0, gap0=.9; /*very bad kludge ... for letting arguments reset default gap value */
int rotn0 = 0;
//&&&&&&&&&&&&another change defining global variables to help display fn
int tope1;
int phase1;
//&&&&&&&&&another change here
int stars_initteld = 0;

const float alfa = 1.0;
const int bwflag = 0;

Point lux = {{1.,2.,3.}};
Point lu;  

void vsub( Point *dst, Point *a, Point *b );
void vadd( Point *dst, Point *a, Point *b );
void vcross( Point *dst, Point *a, Point *b );
float vdot( Point *a, Point *b );
void vscale( Point *dst, float s, Point *src );
void vsadd( Point *dst, Point *a, float sb, Point *b );
void vlerp( Point *dst, float frac, Point *vfrom, Point *vto );
void vcomb( Point *dst, float sa, Point *a, float sb, Point *b );
float vunit( Point *dst, Point *src );
void vproj( Point *along, Point *perp, Point *vec, Point *onto );
float vdist( Point *p1, Point *p2 );
float qdist( Point *q1, Point *q2 );
float tdist( Matrix1 t1, Matrix1 t2 );
float vlength( Point *v );
void vuntfmvector( Point *dst,  Point *src, Matrix1 T );
void vtfmvector( Point *dst,  Point *src, Matrix1 T );
void vtfmpoint( Point *dst,  Point *src, Matrix1 T );
void vgettranslation( Point *dst, Matrix1 T );
void vsettranslation( Matrix1 T, Point *src );
void vrotxy( Point *dst, Point *src, float cs[2] );
void eucinv( Matrix1 dst, const Matrix1 src );
void grotation( Matrix1 Trot, Point *fromaxis, Point *toaxis );
void mcopy( Matrix1 dst, const Matrix1& src );
void mmmul( Matrix1 dst, Matrix1 a, Matrix1 b );
float tfm2quat( Point *iquat, Matrix1 src );
void quat2tfm( Matrix1 dst, Point *iquat );
void quat_lerp( Point *dquat, float frac, Point *qfrom, Point *qto );

void vrotxy( register Point *dst, register Point *src, float cs[2] )
{
   dst->x[0] = src->x[0]*cs[0] - src->x[1]*cs[1];
   dst->x[1] = src->x[0]*cs[1] + src->x[1]*cs[0];
   dst->x[2] = src->x[2];
}

#define VROTXY(dst, src, cs) \
	(dst)->x[0] = (src)->x[0]*cs[0] - (src)->x[1]*cs[1], \
	(dst)->x[1] = (src)->x[0]*cs[1] + (src)->x[1]*cs[0], \
	(dst)->x[2] = (src)->x[2]

#define	VMID(dst, a, b) \
	(dst)->x[0] = .5*((a)->x[0] + (b)->x[0]), \
	(dst)->x[1] = .5*((a)->x[1] + (b)->x[1]), \
	(dst)->x[2] = .5*((a)->x[2] + (b)->x[2])

void vadd( register Point *dst, register Point *a, register Point *b )
{
   dst->x[0] = a->x[0] + b->x[0];
   dst->x[1] = a->x[1] + b->x[1];
   dst->x[2] = a->x[2] + b->x[2];
}
void vsub( register Point *dst, register Point *a, register Point *b )
{
   dst->x[0] = a->x[0] - b->x[0];
   dst->x[1] = a->x[1] - b->x[1];
   dst->x[2] = a->x[2] - b->x[2];
}

void vcross( register Point *dst, register Point *a, register Point *b )
{
   dst->x[0] = a->x[1]*b->x[2] - a->x[2]*b->x[1];
   dst->x[1] = a->x[2]*b->x[0] - a->x[0]*b->x[2];
   dst->x[2] = a->x[0]*b->x[1] - a->x[1]*b->x[0];
}


float vdot( Point *a, Point *b ) {
   return a->x[0]*b->x[0] + a->x[1]*b->x[1] + a->x[2]*b->x[2];
}

void vscale( register Point *dst, float s, register Point *src )
{
   dst->x[0] = s*src->x[0];
   dst->x[1] = s*src->x[1];
   dst->x[2] = s*src->x[2];
}

void vsadd( Point *dst, Point *a, float sb, Point *b )
{
   dst->x[0] = a->x[0] + sb * b->x[0];
   dst->x[1] = a->x[1] + sb * b->x[1];
   dst->x[2] = a->x[2] + sb * b->x[2];
}

float vdist( Point *p1, Point *p2 ) {
   Point d;
   vsub(&d, p1, p2);
   return vlength(&d);
}

float vlength( Point *v ) {
   return sqrt(DOT(*v, *v));
}

float vunit( register Point *dst, register Point *src ) {
   float s = sqrt(DOT(*src, *src));
   float scl = s>0 ? 1/s : 0;
   vscale( dst, scl, src );
   return s;
}

/* along = onto * (vec . onto / onto . onto)
 * perp  = vec - along
 */
void vproj( Point *along, Point *perp, Point *vec, Point *onto ) {
    float mag2 = DOT(*onto, *onto);
    float dot = DOT(*vec, *onto);
    float s = (mag2 > 0) ? dot / mag2 : 0;
    Point talong;
    if(along == NULL) along = &talong;
    vscale( along, s, onto );
    if(perp != NULL)
	vsadd( perp, vec, -1, along );
}

/* gkf: these matrix multiplications are short one dimension
 * for exampel vtfm
 * slevy: yes, that's intentional.
 * vtfmvector() transforms a vector (in homog coords, [x,y,z,0]) by a matrix.
 * vtfmpoint() transforms a point  [x,y,z,1].
 * The difference is that vtfmpoint() includes the matrix's translation part
 * and vtfmvector() doesn't.
 */
/* WHERE */

void vuntfmvector( Point *dst, register Point *src, register Matrix1 T )
{
  int i;
  for(i = 0; i < 3; i++)
    dst->x[i] = src->x[0]*T[4*i] + src->x[1]*T[4*i+1] + src->x[2]*T[4*i+2];
}
void vtfmvector( Point *dst, register Point *src, register Matrix1 T )
{
  int i;
  for(i = 0; i < 3; i++)
    dst->x[i] = src->x[0]*T[i] + src->x[1]*T[i+4] + src->x[2]*T[i+8];
}
void vtfmpoint( Point *dst, register Point *src, register Matrix1 T )
{
  int i;
  for(i = 0; i < 3; i++)
    dst->x[i] = src->x[0]*T[i] + src->x[1]*T[i+4] + src->x[2]*T[i+8] + T[i+12];
}

void vgettranslation( Point *dst, Matrix1 T )
{
  memcpy(dst->x, &T[4*3+0], 3*sizeof(float));
}

void vsettranslation( Matrix1 T, Point *src )
{
  memcpy(&T[4*3+0], src->x, 3*sizeof(float));
}

/* Invert a matrix, assuming it's a Euclidean isometry
 * plus possibly uniform scaling.
 */
void eucinv( Matrix1 dst, const Matrix1 src )
{
  int i, j;
  float s = DOT(*(Point *)src, *(Point *)src);
  Point trans;
  for(i = 0; i < 3; i++) {
    for(j = 0; j < 3; j++)
	dst[i*4+j] = src[j*4+i] / s;
    dst[i*4+3] = 0;
  }
  vtfmvector( &trans, (Point *)&src[4*3+0], dst );
  vscale( (Point *)&dst[3*4+0], -1, &trans );
  dst[3*4+3] = 1;
}

void mcopy( Matrix1 dst, const Matrix1& src )
{
  memcpy( dst, src, sizeof(Matrix1) );
}

void mmmul( Matrix1 dst, Matrix1 a, Matrix1 b )
{
  int i, irow, j;
  for(i = 0; i < 4; i++) {
    irow = i*4;
    for(j = 0; j < 4; j++)
	dst[irow+j] = a[irow]*b[j] + a[irow+1]*b[1*4+j] + a[irow+2]*b[2*4+j] + a[irow+3]*b[3*4+j];
  }
}

/* Construct matrix for geodesic rotation from "a" to "b".
 */
void grotation( Matrix1 Trot, Point *va, Point *vb )
{
  Point a, b, aperp;
  float ab_1, apb;
  int i, j;

  mcopy( Trot, id );
  if(vunit(&a, va) == 0 || vunit(&b, vb) == 0)
    return;
  ab_1 = DOT(a,b) - 1;
  vproj( NULL, &aperp, &b, &a );
  if(vunit(&aperp, &aperp) == 0) {
    if(ab_1 >= -1)
	return;		/* Vectors are identical: no rotation */

    /* Otherwise, vectors are oppositely directed.
     * Rotate in an arbitrary plane which includes them.
     */
    aperp.x[ fabs(a.x[0]) < .7 ? 0 : 1 ] = 1;
    vproj( NULL, &aperp, &aperp, &a );
    vunit(&aperp, &aperp);
  }
  apb = DOT(aperp, b);
  for(i = 0; i < 3; i++) {
    float acoef = a.x[i]*ab_1 - aperp.x[i]*apb;
    float apcoef = aperp.x[i]*ab_1 + a.x[i]*apb;
    for(j = 0; j < 3; j++)
	Trot[i*4+j] += a.x[j]*acoef + aperp.x[j]*apcoef;
  }
}

/*
 * Convert the rotation part of a Euclidean isometry+uniform-scaling matrix
 * into a unit quaternion, with non-negative real part.
 * Returns the real part of the quaternion, with the three imaginary components
 * left in iquat->x[0,1,2].
 */
float tfm2quat( Point *iquat, Matrix1 T )
{
  float mag, sinhalf, trace;
  float scl = vlength((Point *)T);	/* gauge scaling from 1st row */
  Point axis;

#define Tij(i,j) T[(i)*4+(j)]

  trace = scl==0 ? 3 : (T[0*4+0] + T[1*4+1] + T[2*4+2])/scl; /*1 + 2*cos(ang)*/
  if(trace<-1) trace = -1; else if(trace > 3) trace = 3;
  sinhalf = sqrt(3 - trace) / 2;		/* sin(angle/2) */

  axis.x[0] = Tij(1,2) - Tij(2,1);
  axis.x[1] = Tij(2,0) - Tij(0,2);
  axis.x[2] = Tij(0,1) - Tij(1,0);
  if(trace < -.25) {
    /* Angle near pi; sin(angle) is small, so use cos-related elements */
    float c = (trace-1)/2;	/* cos(angle) */
    float v = 1-c;		/* versine(angle) */

    if(Tij(0,0) > c+.5) {		/* large x component */
	axis.x[0] = sqrt((Tij(0,0)-c)/v) * (axis.x[0]<0 ? -1 : 1);
	axis.x[1] = (Tij(0,1)+Tij(1,0)) / (2*v*axis.x[0]);
	axis.x[2] = (Tij(0,2)+Tij(2,0)) / (2*v*axis.x[0]);

    } else if(Tij(1,1) > c+.5) {	/* large Y component */
	axis.x[1] = sqrt((Tij(1,1)-c)/v) * (axis.x[1]<0 ? -1 : 1);
	axis.x[0] = (Tij(0,1)+Tij(1,0)) / (2*v*axis.x[1]);
	axis.x[2] = (Tij(2,1)+Tij(1,2)) / (2*v*axis.x[1]);

    } else if(Tij(2,2) > c+.5) {	/* large Z component */
	axis.x[2] = sqrt((Tij(2,2)-c)/v) * (axis.x[2]<0 ? -1 : 1);
	axis.x[0] = (Tij(0,2)+Tij(2,0)) / (2*v*axis.x[2]);
	axis.x[1] = (Tij(2,1)+Tij(1,2)) / (2*v*axis.x[2]);
    } else {
	int i;
	fprintf(stderr, "Hey, tfm2quat() got a non-rotation matrix!\n");
	fprintf(stderr, "Check this out:\n");
	for(i=0;i<4;i++)
	  fprintf(stderr, "%12.8g %12.8g %12.8g %12.8g\n", aff[4*i],aff[4*i+1],aff[4*i+2],aff[4*i+3]);
    }
  }
  mag = vlength(&axis);
  /* The imaginary part is a vector pointing along the axis of rotation,
   * of magnitude sin(angle/2).  So normalize & scale the axis,
   * but don't fail if its magnitude was zero (i.e. no rotation).
   */
  vscale( iquat, mag==0 ? 0 : sinhalf/mag, &axis );
  return sqrt(1 + trace) / 2;
}

void quat2tfm( Matrix1 dst, Point *iquat )
{
  Point axis;
  float s, c, v, coshalf;
  int i, j;
  float sinhalf = vlength(iquat);

  mcopy( dst, id );
  if(sinhalf == 0)
    return;
  if(sinhalf > 1) {
    fprintf(stderr, "quat2tfm: Yikes, clamping quat to length 1 (was 1+%g)\n", sinhalf-1);
    sinhalf = 1;
  }

  vscale(&axis, 1/sinhalf, iquat);

  coshalf = sqrt(1 - sinhalf*sinhalf);
  s = 2*sinhalf*coshalf;	/* sin(angle) */
  v = 2*sinhalf*sinhalf;	/* versine: 1 - cos(angle) */
  c = 1 - v;			/* cos(angle) */

  for(i = 0; i < 3; i++) {
    for(j = 0; j < i; j++)
	dst[4*i+j] = dst[4*j+i] = axis.x[i]*axis.x[j]*v;
    dst[4*i+i] = axis.x[i]*axis.x[i]*v + c;
  }
  dst[4*0+1] += axis.x[2]*s;  dst[4*1+0] -= axis.x[2]*s;
  dst[4*2+0] += axis.x[1]*s;  dst[4*0+2] -= axis.x[1]*s;
  dst[4*1+2] += axis.x[0]*s;  dst[4*2+1] -= axis.x[0]*s;
}

float qdist( Point *q1, Point *q2 ) {
  Point nq2;
  float s, sneg;
  vscale(&nq2, -1, q2);
  s = vdist(q1,q2);
  sneg = vdist(q1, &nq2);
  return (s < sneg) ? s : sneg;
}

float tdist( Matrix1 t1, Matrix1 t2 ) {
  float s = 0;
  int i;
  for(i=0; i<12; i++)
    s += (t1[i]-t2[i])*(t1[i]-t2[i]);
  return sqrt(s);
}

void quat_lerp( Point *qdst, float frac, Point *qfrom, Point *qto ) {
  Point dst;
  Point tto = *qto;
  float s, rdst;
  float ifrom2 = DOT(*qfrom,*qfrom);
  float rfrom = ifrom2>1 ? 0 : sqrt(1 - ifrom2);	/* Real part */
  float ito2 = DOT(tto,tto);
  float rto = ito2>1 ? 0 : sqrt(1 - ito2);
  float dot = DOT(*qfrom,tto);
  if(dot < 0) {
	/* quaternions are in opposite hemispheres: negate "tto" */
	rto = -rto;
	vscale( &tto, -1, &tto );
  }
  /* Use linear interpolation between the quaternions.  This isn't right,
   * but shouldn't be far off if they don't differ by much.
   */
  rdst = (1-frac)*rfrom + frac*rto;
  vlerp( &dst, frac, qfrom, &tto );
  s = 1/sqrt(rdst*rdst + DOT(dst,dst));
  if(rdst < 0) s = -s;
  vscale( qdst, s, &dst );
}

void vlerp( Point *dst, float frac, Point *vfrom, Point *vto )
{
  dst->x[0] = (1-frac)*vfrom->x[0] + frac*vto->x[0];
  dst->x[1] = (1-frac)*vfrom->x[1] + frac*vto->x[1];
  dst->x[2] = (1-frac)*vfrom->x[2] + frac*vto->x[2];
}

/* Linear combination: dst = sa*a + sb*b */
void vcomb( Point *dst, float sa, Point *a, float sb, Point *b )
{
  dst->x[0] = sa*a->x[0] + sb*b->x[0];
  dst->x[1] = sa*a->x[1] + sb*b->x[1];
  dst->x[2] = sa*a->x[2] + sb*b->x[2];
}

/* We need another package to keep track of multiply-used vertices
 */
struct multvert {
  int nvnos, vroom;
  int *vnos;
};

struct multvert *multverts;
int nmultverts, multvertroom;

int evhash( Point *p ) {
  int x = int(p->x[0] * 10000);
  int y = int(p->x[1] * 10000);
  int z = int(p->x[2] * 10000);
  return ((unsigned int)(x + y*11 + z*37)) % nhash;
}

int evhash2( Point *p, int dp[3] ) {
  int x = (int)(p->x[0] * 10000) + dp[0];
  int y = (int)(p->x[1] * 10000) + dp[1];
  int z = (int)(p->x[2] * 10000) + dp[2];
  return ((unsigned int)(x + y*11 + z*37)) % nhash;
}

#define EVTINYSEG  .02

#define EVEPS  .00002
int evmatch( Point *a, Point *b ) {
  return (fabs(a->x[0] - b->x[0]) < EVEPS
    && fabs(a->x[1] - b->x[1]) < EVEPS
    && fabs(a->x[2] - b->x[2]) < EVEPS);
}

#define  MULTPAIR(mvno)	  (-3 - (mvno))
#define  MVNO(multpair)   (-3 - (multpair))

void evcollision( FILE *f, int i, Point /*segs*/[] ) {
  if(i != -1)
    fprintf(f, /*" %d[%g %g %g]"*/" %d", i /* , segs[i].x[0], segs[i].x[1], segs[i].x[2] */);
}

void pseg( int i ) {
  printf("(%4d) %4d %7.5f %7.5f %7.5f : %7.5f %7.5f %7.5f %d (%d)\n",
	_paired[i], i, _segs[i].x[0], _segs[i].x[1], _segs[i].x[2],
	_segs[i^1].x[0], _segs[i^1].x[1], _segs[i^1].x[2], i^1, _paired[i^1]);
}

void evmultvert( int vno, int mvno ) {
  struct multvert *mv = &multverts[mvno];
  int i;

  if(vno < 0) return;
  for(i = 0; i < mv->nvnos; i++)
    if(vno == mv->vnos[i])
	return;
  if(mv->nvnos >= mv->vroom)
    mv->vnos = (int *)realloc(mv->vnos, (mv->vroom *= 2)*sizeof(int));
  mv->vnos[mv->nvnos++] = vno;
}

void evmultclear() {
  int i;
  for(i = 0; i < nmultverts; i++)
    free(multverts[i].vnos);
  nmultverts = 0;
}

/*
 * seg-vertex "i" appears as the end of more than two segments.
 */
int evmultiple( int i, int hent, Point *segs, int paired[] ) {
  int mvno;
  if(ht[hent] < -1) {
    mvno = MVNO( ht[hent] );
  } else {
    /* It's a new one */
    if(nmultverts >= multvertroom)
	multverts = (struct multvert *)
		( multvertroom>0
		? realloc(multverts, (multvertroom *= 2)*sizeof(*multverts))
		: malloc((multvertroom = 30)*sizeof(*multverts)));
    mvno = nmultverts++;
    multverts[mvno].vnos = (int *)malloc( (multverts[mvno].vroom = 10) * sizeof(int) );
    multverts[mvno].nvnos = 0;
  }
  if(debug) printf("multvert %d: %d (%d) %d (%d) ((%d)) %g %g %g\n",
	MULTPAIR(mvno), i, paired[i], ht[hent],
		paired[ht[hent]], paired[paired[ht[hent]]],
	segs[i].x[0], segs[i].x[1], segs[i].x[2]);
  evmultvert( i, mvno );
  evmultvert( paired[i], mvno );
  if(paired[i] >= 0) {
    evmultvert( paired[paired[i]], mvno );
    paired[paired[i]] = MULTPAIR(mvno);
  }
  if(ht[hent] >= 0) {
    evmultvert( ht[hent], mvno );
    evmultvert( paired[ht[hent]], mvno );
    if(paired[ht[hent]] >= 0)
	paired[paired[ht[hent]]] = MULTPAIR(mvno);
    paired[ht[hent]] = MULTPAIR(mvno);
  }
  paired[i] = MULTPAIR(mvno);
  ht[hent] = MULTPAIR(mvno);
  return mvno;
}

  
int evsearch( int i, Point segs[], int paired[] ) {
  Point *p = &segs[i];
  int h, hv;

  for(h = evhash(p); ht[h] != -1; ++h >= nhash ? (h=0) : 0) {
    hv = (ht[h] >= 0) ? ht[h] : multverts[MVNO(ht[h])].vnos[0];
    if(evmatch(p, &segs[hv])) {

	if( ht[h] < 0 || paired[hv] != -1 ) {
	    return evmultiple( i, h, segs, paired );
	}

	paired[i] = ht[h];
	paired[ht[h]] = i;
	return ht[h];
    }
  }
  ht[h] = i;
  return -1;
}

int evsearch2( int i, int dp[], Point segs[], int paired[] ) {
  Point *p = &segs[i];
  int h = evhash2( p, dp );
  int hv;

  for( ; ht[h] != -1; ++h >= nhash ? (h=0) : 0) {
    hv = (ht[h] >= 0) ? ht[h] : multverts[MVNO(ht[h])].vnos[0];
    if(i != ht[h] && evmatch(p, &segs[hv])) {
	if(paired[hv] != -1) {
	    return evmultiple( i, h, segs, paired );
	}
	paired[i] = ht[h];
	paired[ht[h]] = i;
	return ht[h];
    }
  }
  return -1;
}

int evfollow( int start, int paired[], char used[], Point *segs, int *countp, int name ) {
  int ni, i = start;
  int count = 0;

  if(start < 0)
    return -1;

  if((name & 0xFE) == 0) name += 2;
  while((ni = paired[i]) != -1) {
    if(ni < -1) {
	/* Struck a higher-order vertex.  By now we know everything
	 * attached to this vertex, so figure out what's next.
	 * We want to join it with a segment which:
	 * - is not already used
	 * - is either very short, or nearly parallel to this segment
	 */
	int k, bestk = -1, oi;
	int mvno = MVNO(ni);
	struct multvert *mv = &multverts[mvno];
	Point way, oway;
	float dot, bestdot = -1.1;

	vsub( &way, &segs[i], &segs[i^1] );
	vunit( &way, &way );
	for(k = 0; k < mv->nvnos; k++) {
	    oi = mv->vnos[k];
	    if(used[oi])
		continue;
	    vsub( &oway, &segs[oi], &segs[oi^1] );
	    vunit( &oway, &oway );
	    dot = fabs(DOT(way, oway));
	    if(bestdot <= dot)
		bestdot = dot, bestk = k;
	}
	if(bestk >= 0) {
	    /* Good enough, use it. */
	    ni = paired[i] = mv->vnos[bestk];
	    paired[ni] = i;
	} else {
	    fprintf(stdout, "Tope %d: %d-fold vertex [%d] %d: %g %g %g\n",
		_tope, mv->nvnos, ni, i,
		segs[i].x[0], segs[i].x[1], segs[i].x[2]);
	    break;
	}
    }
    if(paired[ni] != i) {
	fprintf(stderr, "Broken back-link? ");
	evcollision( stderr, i, segs );
	fprintf(stderr, " => ");
	evcollision( stderr, ni, segs );
	fprintf(stderr, " but %d => ", ni);
	evcollision( stderr, paired[ni], segs );
	fprintf(stderr, "\n");
	paired[ni] = i;
    }
    used[i] = used[i^1] = name;
    count++;
    if(used[ni] != 0)
	break;
    i = ni^1;		/* Switch to other vertex of new segment */
  }
  /* One last tweak.  Why is this necessary? */
  if(paired[i]>=0 && used[paired[i]] == 0) {
    i = paired[i]^1;
    used[i] = used[i^1] = name;
    count++;
  }
  if(used[i] == 0) {
    used[i] = used[i^1] = name;	/* Another hack */
    count++;
  }
  if(countp != NULL)
    *countp += count;
  return i;
}

void tubebasis( Loop *l )
{
  Point prevtanv, prevsegv, nextsegv;
  static Point xv0 = {{1,0,0}}, yv0 = {{0,1,0}}, zv0 = {{0,0,1}};
  Point xv, yv, tanv, tanv0, t;
  Matrix1 Trot;
  int i;
  float arclen, turn;

  l->xyv = (Point (*)[2])NewN(Point, l->nverts * 2,16);

  /* Two arbitrary orthogonal unit vectors, basis for the cross-section.
   * They remain orth unit vectors through the following loop.
   */
   
  xv = xv0;  yv = yv0;  tanv = zv0;
  arclen = 0;

  if(l->closed) {
    vsub( &nextsegv, &l->verts[0], &l->verts[l->nverts-1] );
  } else {
    VZERO(nextsegv);
  }

  VZERO(prevtanv);

  arclen += vunit( &nextsegv, &nextsegv );
  for(i = 0; i < l->nverts; i++) {
    /* Compute tangent vector to core curve at this point */
    prevtanv = tanv;
    prevsegv = nextsegv;
    if(i == l->nverts-1) {
	if(l->closed) {		/* If open, leave nextsegv == prevsegv */
	    vsub( &nextsegv, &l->verts[0], &l->verts[i] );
	}
    } else {
	vsub( &nextsegv, &l->verts[i+1], &l->verts[i] );
    }
    arclen += vunit( &nextsegv, &nextsegv );
    vadd( &tanv, &prevsegv, &nextsegv );

    if(fabs(DOT(tanv, tanv)) < .01) {		/* sigh */
	vsub( &tanv, &prevsegv, &nextsegv );
	if(fabs(DOT(tanv, tanv)) < .01)
	    tanv = prevsegv;
    }

    grotation( Trot, &prevtanv, &tanv );
    vtfmvector( &t, &xv, Trot );  xv = t;
    vtfmvector( &t, &yv, Trot );  yv = t;

    vproj( NULL, &xv, &xv, &tanv );	/* Re-orthogonalize.  Should we need to do this? */
    vunit( &xv, &xv );
    vcross( &yv, &tanv, &xv );
    vunit(&yv, &yv);
    if(fabs(DOT(xv,yv)) > .1 || DOT(yv,yv)<.9||DOT(xv,xv)<.9) {
	i++;i--;	/*debug stop*/
    }

    l->xyv[i][0] = xv;
    l->xyv[i][1] = yv;
    if(i == 0)
	tanv0 = tanv;
  }
  if(l->closed && arclen > 0) {
    /* Correct for holonomy. */
    float fturn, cs[2];
    float arcnow = 0;

    grotation( Trot, &tanv, &tanv0 );
    vtfmvector( &t, &xv, Trot );  xv = t;
    vtfmvector( &t, &yv, Trot );  yv = t;

vtfmvector( &t, &tanv, Trot );
    turn = atan2( DOT(l->xyv[0][0], yv), DOT(l->xyv[0][0], xv) );

    for(i = 1; i < l->nverts; i++) {
	arcnow += vdist( &l->verts[i-1], &l->verts[i] );
	fturn = turn * arcnow / arclen;
	cs[0] = cos(fturn);
	cs[1] = sin(fturn);
	vcomb( &t,             cs[0], &l->xyv[i][0],  cs[1], &l->xyv[i][1] );
	vcomb( &l->xyv[i][1], -cs[1], &l->xyv[i][0],  cs[0], &l->xyv[i][1] );
	l->xyv[i][0] = t;
    }
    i++;
  }
}

void
swabl(int *dst, int *src, int nwds)
{
  if(1 != ntohl(1) || dst != src) {
    while(--nwds >= 0) {
	*dst++ = ntohl(*src++);
    }
  }
}

int fgetint(FILE *f, int ascii)
{
  int v;

  if(ascii) {
    fscanf(f, "%d", &v);
    return v;
  }
  if(fread(&v, sizeof(int), 1, f) <= 0)
    return -1;
  return ntohl(v);
}

int fgetfloats(float *fp, int nfloats, FILE *f, int ascii) {
  int k;
  if(ascii) {
    for(k = 0; k < nfloats; k++)
	if(fscanf(f, "%f", &fp[k]) <= 0)
	    break;
    return k;
  }
  k = fread(fp, sizeof(float), nfloats, f);
  swabl((int *)fp, (int *)fp, k);
  return k;
}

int
readanoff( FILE *geom, register Tope *tp, int ascii )
{
  long base = ftell(geom);
  int fno, ncolor;
  float acolor[4];
  int nfv, k;
  int *fv, fvroom;
  int totfv;
  //unused:  int optimize = (getenv("NOSIMP") == NULL);

  memset(tp, 0, sizeof(Tope));
  tp->nverts = fgetint(geom, ascii);
  tp->nfaces = fgetint(geom, ascii);
  (void) fgetint(geom, ascii);
  if(tp->nverts < 0 || tp->nverts > 100000000 || tp->nfaces < 0 || tp->nfaces > 100000000) {
    fprintf(stderr, "Implausible binary-OFF header in tope %d (file offset %ld): nverts %d, nfaces %d\n",
	ntopes, base, tp->nverts, tp->nfaces);
    return 1;
  }

  tp->verts = NewN(Point, tp->nverts,13);

  if(fgetfloats((float *)tp->verts, 3*tp->nverts, geom, ascii) < 3*tp->nverts) {
    fprintf(stderr, "failed to read %d verts in tope %d (offset %ld)\n", int(tp->verts), ntopes, base);
    return 1;
  }

  tp->norm = (Point *) MALLOC( tp->nfaces * sizeof(Point) ,4 );
  tp->nfv = (int *) MALLOC( tp->nfaces * sizeof(int),4 );
  tp->fv0 = (int *) MALLOC( tp->nfaces * sizeof(int),4 );
  tp->fromdl = (unsigned char *) MALLOC( tp->nfaces * sizeof(char),4 );
  memset(tp->fromdl, 255, tp->nfaces*sizeof(unsigned char));

  fvroom = 4 * tp->nfaces + 1000;
  // change from alloca
  fv = (int *)malloc( fvroom*sizeof(int) );
  totfv = 0;

  for(fno = 0; fno < tp->nfaces; fno++) {
    nfv = fgetint(geom, ascii);
    if(nfv <= 0 || nfv > 100000) {
	fprintf(stderr, "Unreasonable number of verts (%d) on face %d of tope %d (file offset %ld)\n",
		nfv, fno, ntopes, base);
	return 1;
    }
    tp->nfv[fno] = nfv;		/* Number of verts on this face */
    tp->fv0[fno] = totfv;	/* Table-index of 1st vert of this face */
    if(nfv + totfv > fvroom) {	  /* need more room in fv[] table? */
      // change from alloca
	int *newfv = (int *)malloc( 2*fvroom*sizeof(int) );
	fvroom *= 2;
	memcpy( newfv, fv, totfv*sizeof(int) );
	fv = newfv;
    }
    for(k = 0; k < nfv; k++)
	fv[totfv++] = fgetint(geom, ascii);

    if(ascii) {
	while((k = getc(geom)) != '\n' && k != EOF)
	    ;
    } else {
	ncolor = fgetint(geom, ascii);		/* Read and ignore any colors */
	if(ncolor > 4 || ncolor < 0) {
	    fprintf(stderr, "Unreasonable number of face-color components (%d) on face %d of tope %d (file offset %ld)\n",
		    ncolor, fno, ntopes, base);
	    return 1;
	}
	fread(&acolor,ncolor*sizeof(float),1,geom);
    }
  }

  tp->fv = (int *)MALLOC( totfv * sizeof(int),5 );
  tp->totfv = totfv;
  memcpy(tp->fv, fv, totfv*sizeof(int));

  /* Compute normals */
  for(fno = 0; fno < tp->nfaces; fno++) {
    Point v01, v02;
    int fv0 = tp->fv0[fno];
    vsub(&v01, &tp->verts[tp->fv[fv0]], &tp->verts[tp->fv[fv0+1]]);
    vsub(&v02, &tp->verts[tp->fv[fv0]], &tp->verts[tp->fv[fv0+2]]);
    vcross(&tp->norm[fno], &v01, &v02);
    vunit(&tp->norm[fno], &tp->norm[fno]);
  }
  return 0;
}

void loopdl( Tope *tp )
{
  int nv = tp->nedgeverts;	
  int trotn = (rotn > 0) ? rotn : 1;
  int rnv = trotn * nv;
  Point *segs = NewN( Point, rnv ,1);
  int *paired;
  char *used;
  float cs[2];
  int i, r, k, m, loopno;
  Loop *l;
  FILE *flink;
  char flname[32];

  nhash = rnv * 3 + 1;
  ht = NewN( int, nhash,1);
  memset(ht, -1, nhash*sizeof(int) );

  paired = NewN( int, rnv ,1);
  memset( paired, -1, rnv*sizeof(int) );

  used = NewN( char, rnv ,1);
  memset(used, 0, rnv*sizeof(char) );

  _paired = paired; _segs = segs; _tope = tp - topes; /* for debug only */

  for(i = 0; i < nv; i++) {
    segs[i] = tp->edges[i];
    if((i&1)==0 && memcmp(&tp->edges[i], &tp->edges[i+1], sizeof(Point))==0) {
	used[i] = used[i^1] = 1;	/* Hack to avoid wasting time on junk */
	i++;
	continue;
    }
    evsearch( i, segs, paired );
  }

  k = nv;
  for(r = 1; r < trotn; r++) {
    cs[0] = cos(2*M_PI*r/trotn);
    cs[1] = sin(2*M_PI*r/trotn);
    for(i = 0; i < nv; i++) {

	vrotxy( &segs[k+i], &segs[i], cs );
	if((i&1)==0 && evmatch( &segs[k+i], &segs[i^1] )) {
	    used[k+i] = used[(k+i)^1] = 1;
	    i++;      /* This segment is its own image under rotation! */
	    continue; /* Skip segment: don't enter duplicate into hash table. */
	}
	evsearch( k+i, segs, paired );
    }
    k += nv;
  }

  /* Now paired[] contains all the vert-to-vert matches we could find easily. */

  for(i = 0; i < rnv; i++) {
    
    if(paired[i] == -1 && used[i] == 0) {
	int dp[3];
	for(dp[0] = -1; dp[0] <= 1; dp[0]++) {
	  for(dp[1] = -1; dp[1] <= 1; dp[1]++) {
	    for(dp[2] = -1; dp[2] <= 1; dp[2]++) {
	      if(evsearch2( i, dp, segs, paired ) >= 0) goto found;
	    }
	  }
	}
	if(debug) {
	    printf("Tope %d: Unmatched vertex %d\n", _tope, i);
	    pseg(i);
	}
    found: ;
    }
  }

  if(debug) printf("Tope %d loops: ", _tope);
  for(i = 0, loopno = 1; i < rnv; i += 2) {
    int end0, end1;
    int count = 0;
  
    if(used[i])
	continue;
    end0 = evfollow( i, paired, used, segs, &count, loopno*2 );
    if(paired[i^1] == end0)
	end1 = i^1;
    else {
	count++;
	end1 = evfollow( i^1, paired, used, segs, &count, loopno*2+1 );
    }
    if(used[i] == 0) {
	/* Hack to pick up isolated segments */
	end0 = i;
	end1 = i^1;
	used[i] = used[i^1] = loopno*2;
	count = 1;
    }

    if(count > 0) {
	k = 0;
	l = NewN( Loop, 1,17 );
	memset(l, 0, sizeof(Loop));
	l->closed = ((paired[end0] == end1) || ((end0^end1) == 1)) && (count > 2);
	l->verts = NewN( Point, count+1,18 );
	for(i = 0, m = 0, k = end0; m < count && k >= 0; m++) {
	    l->verts[i] = segs[k];
	    if(i == 0 || vdist(&l->verts[i], &l->verts[i-1]) > EVTINYSEG)
		i++;
	    k = paired[k ^ 1];
	}
	if(!l->closed && i>0 && vdist(&l->verts[i-1], &segs[end1]) > .0005)
	    l->verts[i++] = segs[end1];
	l->nverts = i;
	if(i <= 1) {
	    free(l->verts);  free(l);
	    continue;
	}
	if(debug) printf(" %d", l->closed ? -i : i);
	l->next = tp->loops;
	tp->loops = l;
	tubebasis( l );
	loopno++;
    }
  }
  if(debug) {
    for(i=0; i < rnv; i += 2) {
	if(used[i] == 0) {
	    loopno++;	/* for debugging */
	}
    }
  }

  if(debug>1) {
    sprintf(flname, "loop%03d.lnk", tp - topes);
    flink = fopen(flname, "wb");
    fprintf(flink, "LINK\n%d\n", loopno-1);
    for(l = tp->loops; l != NULL; l = l->next)
	fprintf(flink, "%d\n", l->nverts);
    for(l = tp->loops; l != NULL; l = l->next) {
	fprintf(flink, "\n");
	for(k = 0; k < l->nverts; k++)
	    fprintf(flink, "%g %g %g\n", l->verts[k].x[0],
				l->verts[k].x[1], l->verts[k].x[2]);
    }
    fclose(flink);
  }
  
  if(debug) printf("\n");

  evmultclear();
}

#define facet_has_vert(f,v) \
    ( tp->fv[tp->fv0[f]]==(v) || tp->fv[tp->fv0[f]+1]==(v) || \
      tp->fv[tp->fv0[f]+2]==(v) )

void
searchv(Tope *tp, int f0, int flast, int f, int v,
		int thresh, int next[], int *tail)
{
    int e;
    for(e = tp->nfv[f]; --e >= 0; ) {
	int adj = tp->fadj[tp->fv0[f] + e];
	if(adj >= 0 && adj < tp->nfaces && adj != f0 && adj != flast &&
		facet_has_vert(adj,v) && tp->fromdl[adj] >= thresh) {
/* here we need to search even through faces already set = thresh,
   but we need somehow to avoid infinite loops */
	    if (tp->fromdl[adj]>thresh) {
	        tp->fromdl[adj] = thresh;
		next[*tail] = adj;
		next[adj] = -1;
		*tail = adj;
	    }
	    searchv(tp,f0,f,adj,v,thresh,next,tail);
	}
    }
}

void
findfromdl( register Tope *tp )
{
  int *next = NewN(int, tp->nfaces,1);
  int head, tail;
  int f, e, v;

  /* Breadth-first search for facets at given distance from the
   * double locus.  Start with distance-0 facets.
   */
  head = tail = -1;
  for(f = 0; f < tp->nfaces; f++) {
    if(tp->fromdl[f] == 0) {
	if(head < 0) head = tail = f;
	else {
	    next[tail] = f;
	    next[f] = -1;
	    tail = f;
	}
    }
  }
  /* Now scan the list, adding facet neighbors. */
  for(f = head; f >= 0; f = next[f]) {
    int thresh = tp->fromdl[f] + 1;
    if(thresh > 255) thresh = 255;
    if(!ribbonflag) {
      for(e = tp->nfv[f]; --e >= 0; ) {
	int adj = tp->fadj[tp->fv0[f] + e];
	if(adj >= 0 && adj < tp->nfaces && tp->fromdl[adj] > thresh) {
	    tp->fromdl[adj] = thresh;
	    next[tail] = adj;
	    next[adj] = -1;
	    tail = adj;
	}
      }
    }
    else {
      for(v = tp->nfv[f]; --v >= 0; )
	searchv(tp,f,f,f,tp->fv[tp->fv0[f]+v],thresh,next,&tail);
    }
  }
}

void
readdl( FILE *dlf, Tope *tp )
{
  int i;
  int v = -1;
  char line[256];

  tp->fromdl = NewN(unsigned char, tp->nfaces,14);
  memset(tp->fromdl, -1, tp->nfaces*sizeof(unsigned char));

  /* We expect:
   *  a list of the faces which lie on the double locus,
   *  then -1,
   *  then a vertex count and list of 3-D vertex pairs which trace the locus.
   */
  while(fgets(line, sizeof(line), dlf) != NULL) {
    if(sscanf(line, "%d", &v) <= 0)	/* Ignore blank lines, comments */
	continue;
    if(v < 0)				/* Reached "-1" */
	break;
    if(v < tp->nfaces)
	tp->fromdl[v] = 0;
  }
  fgets(line, sizeof(line), dlf);
  if(sscanf(line, "%d", &tp->nedgeverts) > 0 && tp->nedgeverts > 0) {
     tp->edges = NewN(Point, tp->nedgeverts,15);
     memset(tp->edges, 0, tp->nedgeverts*sizeof(Point));
     for(i = 0; i < tp->nedgeverts; i++) {
	line[0] = '\0';
	fgets(line, sizeof(line), dlf);
	if(3 != sscanf(line,"%f %f %f",&tp->edges[i].x[0],
	       &tp->edges[i].x[1], &tp->edges[i].x[2])) {
	    fprintf(stderr, "\nExpected vert %d/%d of double locus for %d, got: %s",
		i, tp->nedgeverts, ntopes, line);
	    break;
	}
     }
  }

  findfromdl(tp);
}

/// \todo hardcoded name of data file
const char* filename = "h2d45.mov";

bool dotrans()
{
  FILE* topefile = ar_fileOpen(string(filename), "avn", dataPath, "rb");
  if (topefile == NULL) {
    cerr << "avn error: failed to open " << string(filename) << endl;
    return false;
  }
  printf("avn remark: reading %s\n", filename);
  char* filename2 = (char *)malloc(strlen(filename) + 4);
  sprintf(filename2, "%s.dl", filename);
  FILE* dlfile = ar_fileOpen(string(filename2), "avn", dataPath, "rb");

  /*
  if(dlfile==NULL)
     printf("avn remark: Double locus file <%s> not present.\n",filename2);
     // But it's ok to run without the double locus file.
  */

  //fprintf(stderr,"Getting topes%s: ", dlfile ? " and double-loci" : "");

  for(;;) {
      astring[0] = 0;

      if(fgets(astring, sizeof(astring), topefile) == NULL) {
	break;				/* We've read all there is... */
      }
	/* Expect something like
	 *  (geometry blah { OFF BINARY
	 *   ... binary OFF data ...
	 *  })
	 *  # comment ...
	 *  (geometry blah2 { OFF BINARY
	 *   ... more binary OFF data ...
	 *  })
	 *  ...
	 *  (exit)
	 */
      if(strncmp(astring, "(exit)", 6) == 0)	/* Done! */
	break;
      if(astring[0] == '#') {
	char *nlp = strchr(astring, '\n');
	if(nlp) *nlp = '\0';
	if(ntopes > 0) {
	    topes[ntopes-1].comment = (char *)MALLOC(strlen(astring + 1) + 1,3);
	    strcpy(topes[ntopes-1].comment, astring+1);
	}
	nlp = strstr(astring, "Rotn=");
	if(nlp && rotn == 0) {
	    sscanf(nlp, "Rotn=%d", &rotn);
	    if(rotn & 1) phase |= 2;
	}
      }
      else if(strstr(astring, "OFF") != NULL) {
	if( readanoff( topefile, &topes[ntopes], strstr(astring,"BINARY")==NULL ) )
	  break;	/* If readanoff() returns nonzero, we're hosed */
			/* otherwise, we've got a new tope */

	//printf("%3d \b\b\b\b", ntopes);
	//fflush(stdout);

	if(dlfile != NULL) {
	    readdl( dlfile, &topes[ntopes] );
	    if(rotn > 0)
		loopdl( &topes[ntopes] );
	    if(ntopes == 1 && topes[0].loops == NULL) {
		/* Now that we know the rotational symmetry ... */
		loopdl( &topes[0] );
	    }
	}
	ntopes++;
    }
  }

  if(topefile != stdin)
    fclose(topefile);

  printf("avn remark: read %d topes.\n", ntopes);
  return true;
}

inline void calculite()
{
  vuntfmvector( &lu,  &lux, aff );
}


int paint(int front, float lmb, float dog)
{
#define AMB (.15)	/* was .1 */
#define PWR (10.)	/* was 16. */
   lmb = (lmb<0?-lmb:lmb);
   lmb = 255*MAX(lmb,AMB);
   int rr = int(lmb * (bwflag? (front? .7:.2) : (front? .78 : .02+.96*(1-dog))));
   int gg = int(lmb * (bwflag? (front? .7:.2) : (.02 + .96*dog)));
   int bb = int(lmb * (bwflag? (front? .7:.2) : (front ? .1 : .78)));
   int spec = int(255 - PWR*255 + PWR*lmb); /* Ray Idaszak, 1988 */
   if(spec > 225) spec = 225;

   rr = MAX(rr,spec);
   gg = MAX(gg,spec);
   bb = MAX(bb,spec);

   // Compressed dynamic range for Cube projectors.
   const float dist = (1. - (rr+gg+bb) / (256.*3.));
   const float scalar = 1. + (1.7 * dist);
   rr = int(rr*scalar);
   gg = int(gg*scalar);
   bb = int(bb*scalar);

   rr = MIN(rr,255);
   gg = MIN(gg,255);
   bb = MIN(bb,255);

#define LITTLEENDIAN
   /* byte-order dependent */
#ifdef LITTLEENDIAN
   return (rr) | (gg<<8) | (bb<<16) | (int)(alfa*255)<<24;
#else /*BIGENDIAN*/
   return (rr<<24) | (gg<<16) | (bb<<8) | (int)(alfa*255);
#endif
}

void
do_tri(Point *a,Point *b,Point *c, float ggap, int rgba)
{
    float p[3],q[3],r[3];
    register float g, midpart;

    if(ggap == -1 || ggap == 0)
	return;
    if(ggap == 1) {
	glColor4ubv( (GLubyte *)&rgba );
	glVertex3fv((GLfloat *)a);
	glVertex3fv((GLfloat *)b);
	glVertex3fv((GLfloat *)c);
	return;
    }

    g = fabs(ggap);

#define	INTERP(dst, src, i)  dst[i] = src->x[i]*g + midpart
#define INTERPpqr(i)  midpart = (1-g)*(a->x[i] + b->x[i] + c->x[i])/3, \
			INTERP(p,a,i),INTERP(q,b,i), INTERP(r,c,i)

	/* Unroll the loop for speed */
    INTERPpqr(0);
    INTERPpqr(1);
    INTERPpqr(2);

    glColor4ubv( (GLubyte *)&rgba );
    if (ggap>0)
    {
	glVertex3fv(p); glVertex3fv(q); glVertex3fv(r);
    }
    else
    {
	/* Make a strip from
	 *   a -- b -- c -- a
	 *   |    |    |    |
	 *   p -- q -- r -- p
	 */
	glBegin(GL_QUAD_STRIP);
	glVertex3fv(a->x); glVertex3fv(p);
	glVertex3fv(b->x); glVertex3fv(q);
	glVertex3fv(c->x); glVertex3fv(r);
	glVertex3fv(a->x); glVertex3fv(p);
	glEnd();
    }
} 

int topeof(float rtope, int *lphase)
{
  // no rounding in native Win32 (removing rint function)
  int t = (int)(rtope+.001);
  /* Round to nearest integer, with bias for exact half-integers */
  if(t < 0) {
    *lphase |= 1;
    t = -t;
  } else {
    *lphase &= ~1;
  }
  return t;
}


void draw(){ 
  glClearColor(0,0,0,0);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  TOPE* tp = &(topes[currentTope]);

  int ii,jj;
  Point* n = NULL;
  Point* a = NULL;
  int trotn = rotn;
  float dog;
  //************************************************************
  // set the phase
  const int lphase = phase;
  const int flip = (lphase & 1) ? 1 : 0;
  float gap = 0.75;
  Point* eye = NULL;
  Point* light = NULL;
  Point llu;
  float symrotn = 0; 

  Matrix1 w2c, c2w;
  static Point origin = {{0,0,0}};
  float (*rotcs)[2] = NULL;
  float (*rotcs2)[2] = NULL;
  // All the rotcs2 code is added by Camille Goudeseune,
  // who didn't know what he was doing but this got the lights to
  // stop flipping a quarter turn when the eversion passed through
  // the halfway model (tope number zero, "forward" changes state).
  static int vrottope = -1, vrotn, vrotroom;
  int ltope = currentTope;

  static Point *vrot = NULL;

  eye = NewN(Point, trotn, 1);   /* allocates a local variable */
  light = NewN(Point, trotn, 1);/* so that there are trotn eyes and lights */

  if(flip) {
    if (!(trotn%2)) {  /*trotn is even*/
      float cs[2] = { cos(M_PI/trotn), -sin(M_PI/trotn) };
      vrotxy( &llu, &lu, cs );
      symrotn = 180./trotn;
      glRotatef(symrotn, 0,0,1);
    }
  }

  glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)w2c);
/*gkf: I would prefer w2c <- aff here */

  for (ii = 0; ii <16; ii++){
    w2c[ii] = worldMatrix[ii];
  }

  eucinv(c2w, w2c); /*overkill, there's no scaling to worry about */

  vtfmpoint( &eye[0], &origin, c2w ); 
/* the eyes are at the same place but rotated by the symmetry */

  vtfmvector( &light[0], &lux, c2w );
  vunit( &light[0], &light[0] );

  if(trotn > 1) {
    // change from alloca
    rotcs = (float (*)[2])malloc(2*trotn * sizeof(float));
    rotcs2 = (float (*)[2])malloc(2*trotn * sizeof(float));
    for(ii = 0; ii < trotn; ii++) {
	rotcs[ii][0] = cos(2*M_PI*ii / trotn);
	rotcs[ii][1] = sin(2*M_PI*ii / trotn);
	rotcs2[ii][0] = cos(2*M_PI*ii / trotn - M_PI/2);
	rotcs2[ii][1] = sin(2*M_PI*ii / trotn - M_PI/2);
    }
    for(ii = 1; ii < trotn; ii++) {
	vrotxy( &eye[ii], &eye[0], rotcs[trotn-ii] );
	vrotxy( &light[ii], &light[0], (flip ? rotcs2 : rotcs)[trotn-ii] );
    }
    if (flip)
      vrotxy( &light[0], &light[0], rotcs2[trotn-ii] );

    /* Transform all the vertices, if we haven't already got em cached */
    if(vrottope != ltope || vrotn != trotn) {
	Point *vp, *rp;
	if(vrotroom < tp->nverts*(trotn-1)) {
	    vrotroom = tp->nverts*trotn;	/* Leave some extra space */
	    if(vrot) free(vrot);
	    vrot = (Point *)malloc(vrotroom*sizeof(Point));  /* non-shared */
	}
	for(rp = vrot, ii = 1; ii < trotn; ii++) {
	    float *cs = rotcs[ii];
	    for(vp = tp->verts, jj = 0; jj < tp->nverts; jj++, vp++, rp++) {
		VROTXY(rp, vp, cs);
	    }
	}
	vrottope = ltope;
	vrotn = trotn;
    }
  }

  glBegin(GL_TRIANGLES);
  for(ii=0; ii < tp->nfaces; ii++) {
    /*int fv0 = tp->fv0[ii];
    int va = tp->fv[fv0], vb = tp->fv[fv0+1], vc = tp->fv[fv0+2];
    glColor3f(1,1,1);
    glVertex3fv((float*)&(tp->verts[va]));
    glVertex3fv((float*)&(tp->verts[vb]));
    glVertex3fv((float*)&(tp->verts[vc]));*/

    int fv0 = tp->fv0[ii];
      int va = tp->fv[fv0], vb = tp->fv[fv0+1], vc = tp->fv[fv0+2];
      n = &tp->norm[ ii ];
      a = &tp->verts[ va ];
      float adotn = DOT(*a, *n);
      dog = (n->x[2])/2. + .5;  /* use normal, not height */
      if (flip) dog = 1-dog;
      dog = dog<0? 0:( dog>1? 1:dog);

      int rgba = paint((DOT(eye[0],*n) <= adotn) ^ flip, DOT(light[0],*n), dog);
      do_tri(a, &tp->verts[vb], &tp->verts[vc], gap, rgba);
      for(jj = 1; jj < trotn; jj++) {
	Point *rverts = &vrot[tp->nverts*(jj-1)];
	rgba = paint((DOT(eye[jj],*n) <= adotn) ^ flip, DOT(light[jj],*n), dog);
	do_tri(&rverts[va], &rverts[vb], &rverts[vc], gap, rgba);
      }
  }
  glEnd();
}

void incrementAnimation(){
  static bool forward = true;
  if (forward){
    if (++currentTope>=ntopes){
      currentTope = ntopes-1;
      forward = false;
    }
  }
  else{
    if (--currentTope<0){
      currentTope = 0;
      forward = true;
      phase = 1 - phase;
    }
  }
}

const int cidAudio = 4+12+13;
int rgidAudio[cidAudio];
const char* rgszAudioFile[cidAudio] = {
  "avn_cricket.mp3",
  "avn_dingchurch.mp3",
  "avn_dongchurch.mp3",
  "avn_kablam.mp3",
  "avn-12.mp3",
  "avn-11.mp3",
  "avn-10.mp3",
  "avn-9.mp3",
  "avn-8.mp3",
  "avn-7.mp3",
  "avn-6.mp3",
  "avn-5.mp3",
  "avn-4.mp3",
  "avn-3.mp3",
  "avn-2.mp3",
  "avn-1.mp3",
  "avn0.mp3",
  "avn1.mp3",
  "avn2.mp3",
  "avn3.mp3",
  "avn4.mp3",
  "avn5.mp3",
  "avn6.mp3",
  "avn7.mp3",
  "avn8.mp3",
  "avn9.mp3",
  "avn10.mp3",
  "avn11.mp3",
  "avn12.mp3"
};

void audio_loop(int i, float ampl){
  (void)dsLoop(rgidAudio[i], rgszAudioFile[i], 1, ampl, arVector3(0,0,0)); // loop
}

void audio_beep(int i, float ampl){
  (void)dsLoop(rgidAudio[i], rgszAudioFile[i], -1, ampl, arVector3(0,0,0)); // trigger
  (void)dsLoop(rgidAudio[i], rgszAudioFile[i], 0, ampl, arVector3(0,0,0)); // reset
}

// pitch set for stochastic stuff
static int rgPS[12][5];
static int iPS = 0;

void init_sounds(){
  for (int i=0; i<cidAudio; ++i)
    rgidAudio[i] = dsLoop(rgszAudioFile[i], "root", rgszAudioFile[i], 0, 0.1, arVector3());
  audio_loop(0, 0.009);

  for (iPS = 0; iPS < 11; iPS++) {
    // mysterious secret formula
    rgPS[iPS][0] = (0 + 7*iPS) % 12;
    rgPS[iPS][1] = (2 + 7*iPS) % 12;
    rgPS[iPS][2] = (5 + 7*iPS) % 12;
    rgPS[iPS][3] = (7 + 7*iPS) % 12;
    rgPS[iPS][4] = (9 + 7*iPS) % 12;
    }
  iPS = 0;
}

// --------- stochastic audio stuff -------------------

#ifdef AR_USE_WIN_32
inline float drand48() {
  return float(rand()) / float(RAND_MAX);
}
#endif

static float tPrev = 1.; // setup time
static float zDuration = 0.;
static bool fRecomputeDuration = false;

// Compute the next duration with Goudeseune's optimization
// of the Xenakis-Poisson algorithm in Myhill's article in
// Computer Music Journal 3(3):12-14.
void RecomputeDuration(const float zIrreg, const float zAlpha){
  if (zIrreg <= 0)
    zDuration = zAlpha;
  else {
    const double r = exp(zIrreg);
    const double xMin = pow(r, r/(1.-r));
    const double xMax = xMin * r;
    zDuration = -zAlpha * log(drand48() * (xMax - xMin) + xMin);
    }
  fRecomputeDuration = 0;
}

// density is how many events per second.
// irregularity: 0 = steady pulse, larger = wilder.
void doPentatonicSounds(const float zIrreg, const float density){
  if (density <= 0.)
    return;

  const float zAlpha = 1./density;
  ar_timeval now(ar_time());
  double tNow = now.sec + double(now.usec) / 1e6;
  float timeSinceLastChime = tNow - tPrev;
  if (fRecomputeDuration)
    RecomputeDuration(zIrreg, zAlpha);
  if (timeSinceLastChime < zDuration)
    return;

  tPrev = tNow;
  static bool fSkipFirstTime = false;
  if (!fSkipFirstTime) {
    fSkipFirstTime = true;
    return;
  }

  RecomputeDuration(zIrreg, zAlpha);

  // choose a pitch uniformly from 2 octaves, in the current pitch set
  static int iPitchPrev = -1;
  int iPitch;
  for (;;) {
    iPitch = rand() % 25;
    if (iPitch == iPitchPrev)
      // avoid nervous repetition
      continue;
    const int i = iPitch % 12;
    if (i==rgPS[iPS][0] ||
        i==rgPS[iPS][1] ||
	i==rgPS[iPS][2] ||
	i==rgPS[iPS][3] ||
	i==rgPS[iPS][4] )
      break;
  }

  float ampl = .1 + .25 * drand48();
  if (density < 1.)
    ampl *= density * sqrt(density);
  audio_beep(4 + iPitch, ampl);

}

void doSounds(bool /*fAnimating*/, int tope){
  static float density = .2;

  // 0 <= tope < ntopes == 149
  static int topePrev = -42;
  if (tope != topePrev) {

    // Increase density of background.
    if (density < 1.) {
      density = 1.;
      tPrev -= 10.; // accelerate playing of sounds.
    }
    if ((density *= 1.027) > 6.)
      density = 6.;

    // Change pitch set of background.
    iPS = 11 - (tope * 7 / (149+1));

    // Signal special topological events with chimes.
    switch (tope) {
    case 81:
      audio_beep(1, 0.3); break;
    case 57:
      audio_beep(2, 0.3); break;
    case 0:
      audio_beep(3, 0.3); break;
    }

    topePrev = tope;
  }
  else {
    // Decrease density of background.
    if ((density /= 1.013) < .3)
      density = .3;
  }

  const float irreg = density>4. ? 4. : density>2. ? 1.5 : 0.5;
  doPentatonicSounds(irreg, density);
}

// --------- end of stochastic audio stuff -------------------


bool init(arMasterSlaveFramework& fw, arSZGClient& cli){
  dataPath = cli.getAttribute("SZG_DATA","path");
  if (dataPath == "NULL"){
    cerr << cli.getLabel() << " error: SZG_DATA/path undefined.\n";
    return false;
  }

  if (!dotrans())
    return false;

  vunit(&lux, &lux);
  starttime = (double)glutGet(GLUT_ELAPSED_TIME) / 1000.0;
  worldMatrix = ar_scaleMatrix(3.);

  init_sounds();

  // Data to be shared over the network.
  fw.addTransferField("worldRotation",worldMatrix.v,AR_FLOAT,16);
  fw.addTransferField("currentTope",&currentTope,AR_INT,1);
  fw.addTransferField("phase",&phase,AR_INT,1);
  return true;
}

#ifdef AR_USE_WIN_32
#include <float.h>
#define copysign(a,b) _copysign(a,b)
#endif


void preExchange(arMasterSlaveFramework& fw){
  ++animateCounter;
  if (!fw.getMaster())
    return;

  static bool fUpdate = true;
  static bool fIncrement = true;
  static double ramp = 0.; // acceleration for navigation
  const double rampDuration = 3.; // seconds
  const double rampMin = 2.;
  const double rampMax = 30.;

  const double timeDelta = fw.getLastFrameTime()/1000.0; // in seconds
  const double speed = fw.getAxis(1);
  if (fabs(speed) > .5) {
    const arVector3 wandDirection(ar_vectorToNavCoords(
      ar_extractRotationMatrix(fw.getMatrix(1)) * arVector3(0,0,-1)));

    ramp += (rampMax-rampMin) * timeDelta / rampDuration;
    if (ramp > rampMax)
      ramp = rampMax;

    ar_navTranslate(wandDirection * copysign(ramp*timeDelta, speed));
  }
  else
    ramp = rampMin;

#ifdef DISABLED
  if (fw.getAxis(0) > 0.5){
    ar_navRotate(arVector3(0,1,0),30*timeDelta);
  }
  else if (fw.getAxis(0) < -0.5){
    ar_navRotate(arVector3(0,1,0),-30*timeDelta);
  }
#endif

  // Button 0 toggles animation.
  if (!animatingTopes && fw.getButton(0) && fUpdate){
    animatingTopes = true;
    fUpdate = false;
  }
  if (animatingTopes && fw.getButton(0) && fUpdate){
    animatingTopes = false;
    fUpdate = false;
  }
  if (!fUpdate && !fw.getButton(0)){
    fUpdate = true;
  }

  // Button 2 toggles incrementing (single-stepping).
  if (fIncrement && fw.getButton(2)){
    incrementAnimation();
    fIncrement = false;
  }
  if (!fIncrement && !fw.getButton(2)){
    fIncrement = true;
  }

  // finally, let's do the animation
  // if (animatingTopes && animateCounter%4 == 0)
  if (animatingTopes && animateCounter%2 == 0) // double speed
//  if (animatingTopes)                        // quadruple speed
    incrementAnimation();

  doSounds(animatingTopes, currentTope);
}

void drawCallback(arMasterSlaveFramework& fw){
  fw.loadNavMatrix();

  // anti-aliasing
  glEnable(GL_POLYGON_SMOOTH);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);

  glScalef(8.,8.,8.);
  glPushMatrix();
  glMultMatrixf(worldMatrix.v);
  draw();
  glPopMatrix();
}

int main(int argc, char** argv){
  arMasterSlaveFramework framework;
  if (!framework.init(argc, argv))
    return 1;

  framework.setClipPlanes(0.15, 500);
  framework.setStartCallback(init);
  framework.setPreExchangeCallback(preExchange);
  framework.setDrawCallback(drawCallback);
  framework.setEyeSpacing(6/(12*2.54));
  return framework.start() ? 0 : 1;
}
