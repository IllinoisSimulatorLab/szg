
#ifndef __WINDOWS
#include <alloca.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
/* GLUT for *every* platform! */
#ifdef __sgi
#include <glut.h>	/* SGI glut libs */
#endif
#ifdef __LINUX
#include <GL/glut.h>	/* Linux glut libs */
#endif
#ifdef __MACOSX
#include <GLUT/glut.h>	/*OS X glut libs */
#endif
#ifdef __WINDOWS
#include <gl\glut.h>	/* Windows glut libs */
#endif



/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef __sgi
/* Most math.h's do not define float versions of the trig functions. */
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#define atan2f(y, x) ((float)atan2((y), (x)))

#define fcos(x) cosf(x)
#endif


#define NewA(type, count)  ((type *)alloca((count)*sizeof(type)))
#define NewN(type, count)  ((type *)malloc((count)*sizeof(type)))



typedef float Matrix[4*4];
typedef struct { float x[3]; } Point;

int zoom2 = 0;		/* Flag for "enlarge stars twofold" */

/* extern int    caveyes;	* Hope this is defined somewhere else */
/* extern Matrix starmat;	* Hope this is defined elsewhere too */

void vtfmvector( Point *dst, register Point *src, register Matrix T )
{
  int i;
  for(i = 0; i < 3; i++)
    dst->x[i] = src->x[0]*T[i] + src->x[1]*T[i+4] + src->x[2]*T[i+8];
}


struct starsize {
  float size;
  int base;
  int count;
  int color;
} *starsizes;
int nstarsizes = 0;

float scolors[8][3] = {
	{.5, 1, 1},	/* W */
	{.7, .8, 1},	/* O */
	{.9, .9, 1},	/* B */
	{1, 1, 1},	/* A */
	{1, 1, .9},	/* F */
	{1, 1, .8},	/* G */
	{1, .8, .7},	/* K */
	{1, .7, .5},	/* M */
};

float *starpos;

#define SRADIX  91
#define SZERO   '$'
#define SSIZE   '!'
#define SSIZESCL 8
#define SRASCALE (8192 / (2*M_PI))
#define SDECSCALE (8192 / M_PI)
#define SDECBIAS 4096
#define SCOLORBASE 0
#define SSIZEBASE  8


/* 1128 real stars, to about visual magnitude 4.7 */
/* from  star2illi -m 4.7 -d 1.2 -c 3.5 */
char stardata[] =
  /* Show Pleiades at upper right, Orion at center of field */
"#@ -.98525 .062202 .1594 0  .16485 .095488 .98169 0  .045842 .99349 -.10433  0 0 0 1\n"
"!<'=BH_"
"!:(<$6`"
"!9'iodI!9&7`Lw!9)Z~2Y7mh&!9*YQZ["
"!8&*.4G!8(@eS\\!8+:7Te"
"!7'nLUM!7&Xh2oVCKK!7*5;Y=!7+arCl"
"!6'z/B6q[g`!6&S~33J'W$!6*A,_'Z~2Y"
"!5'@J`{@J`{FY.1!5&>3BTRa1Mes>N8BT58H_A9&PIw'9S!5+R{4M"
"!4'TGm$:OgQ<sY7Dm5_ezW?!4&i&?nWgiap]4^;xH'j|Cr$T_W9gL4Ra1M!4(0jiz>kCn"
"f-;R-Q}`!4*MQouCJ3:c).PGPLb+z\\h&fH&Xy>o[bv-!4+(Ebo!4%9?P'!4$B[9C"
"!3'PBX?S[8S^K^FFo3EV;lPMFm7!3&/ieQ8iPrW;6=[09AZg;y`'EdfJ=Q!3(pIe1$XnZ!3*"
"F<;>&Tm>g@ji+hf5c2?r%_;ruTU{!3+y59W!3%B9=%"
"!2'dJI2P]kssyp@N1[=bCKfkKB*:Pc]+3[J)Eo0!2&?iBE'VoFzXX\\G15QX99ER$H9QU7_"
"]BLA`DG.99?}\\4;LSC.Lec>D!2(8mH-!2)S3E@X4Z7LK8?!2*qwb$JiZy_'T96XaZ?B>N"
"[B^VhoB,n2VA!2+zR_)/HS)"
"!1'V)>_[cH~7:NOueHz]M.`TSd4Hd0Ok[Wz,-bR!1&L60m^N<JQ{3]b;Bv$oX[2^`{28]*"
"n+gY2se&_zCzaCD7?{U2RtHhei8*9.[Z!1(BPDr_e1>+K28ktFR6tfx!1)aSokav[ieak3"
"%[*FbZ`nTvVQ8SF]tgN9/Vkjy7`/vtPr=;]YH]\\vUzEK!1*fPS?i<D?eD5:wb2s=]7cgtAm"
"Q_Ed!1+`wO3;z\\=2tJ;!1%8|N)"
"!0'ZSd3]WtyEdi(dd]K[30S!0&1yhwv1>A;kA}>LE,Ws;jSv.{c;=}OQ1Q\\7;{73e^<s;I"
"dAqtjFCS][<bofP[tWt@k6aD!0(fg<ypBI\\Gnjr6/TQ!0)l)rrEOS~sY`0a3N`!0*Msg<"
"m5_$L\\HwqE9E9z?/G+b8Ge4Pcb5%ddcK7-Eoc\\Udft>P|gwp@,;DhqOW!0+,eOSJvejh\\>]"
"27+u"
"!/'505P=S2'yyI-Q}mUN2XfVyPe4lXz/1<rqo/vJYfO.7R]yW7D!/&e1DR7WHwJE-~KR23"
"eD4n8>On]b:`+2pry.VKWr<3`&=mFL3Sk`NXj_ad3(W;Am6S!/(dT;G=CWNlrRX+*_mJV\\f!/)(09_ACDX3v1jCvoFDyT8]'6{fa^s'*mx]9aay_]A*RI("
"\\Ge7!/*&N`MXwCa]onRkdC4gNL0w6n/JY2Cqqoy(>Kx>EC(NJaW!/+;L\\<\\SDETPRe/XdK"
")T;DnzZidbX7"
"!.'HBU|I|YH_EO?w2T.9cI[?GY==mb$E~hZP./^Xlq7_2Xf_LS;[IQ{!.&i6:&IA5fYf9}"
",U7:4/@.c>>$7rMYDT6Vee2`zHf5.]^_2B](&@k|^bB/gy7{Dg@J67Sn2)]+6HR9!.(?S\\$"
"f.I@hpuG9UEkGb<jPKQuqEX@S\\P>GfpVS\\P>^$_Wu=Ha:>Ixscd(!.)4lZ[bcdOOAA+NOI\\"
"oW/wA']7p1Jg0mUT)eXcHY1i+;76gF_^nfT7!.*k/FNC(U[y]r/1{L0%8L[]]>s*0iBZR`6"
"^YBzRC2m);LxA,>&J4JpTv-9cK;of]0_4:XorM3kzrFK{CR_P1hw:Ol2s+f}!.+0OF0!.%"
"8{U}"
"!-'g}UlaGZZfiREr$L<xOj2m/jtFycJazR$wsPA9e7OJK;zSE8g'Vd<R;PbC\\O*36T$Du9}!-&0{UtqQX|f9h&2&a2XI;{g~_HEM2b^y^3?5C]KWU`hFFO2/]6S(,yS-swXK:[]%8^F`R3_oBHNX5ia;h4"
"Ri7u!-(xCn98r1iv{]c2+f@>S[?HznU^LV=^LV=K;3^DV9bKq8u_lXq0%BS[7N4nVQS!-)"
"tLEmkWF0lCkd?s^wJiZy9|FX59Af|\\h;n=t2;u@>`}13Vd=@b.)VMl3S^PI\\!-**|Kq1@L>"
"AS<e6weWF&9Mg.mM4LYlc03PuE*@C\\/y>m-hLr3YoshG/qgLK1HZLqb/[R)O4lY$i$[v2%0["
"ieLu3t;rg>c^5HIr8d?==uDw$Z:0I)^%/'LWnlbWzx:GH;PL!-+ynM8.al|n9Z=O8sb!-%"
"94Oe"
"!,'(=5Gck`PsiS^5=OC@pL9l]H,%^;3DmZ,NiV=rZe[)d8O.ukH<mGG@s,dV;lO_M@5yK\\m4R@$"
":M;Z=\\@hA$BTQg6o`LFao),U9}dZDS?DEmewFN1qgWROli<cxD;;{dF{+eu9;KMt?@/'o~ht{E3u"
"+RFN.s8*Ew/mK'+~[0>.,YazE3C2VClR`0n?`jG=j7->lb:jpr`6y\\J7}}TM5qOGDx_HQUD^a;7y"
"k:XVkFN1tugnxfP{6|o:P0T=TK4JgrR<q-V`2ybw3Vht3hMZR31$j}f~/oioNYT'Ym>*Z,jzZt8>"
"[:?K\\n:E&{]1@R^Ncg6J*PjE-|Q46TWj=uJ~C3*UDk;bWrXw]E3[s1HHGw3GM:GrNnH3YSN%t2Zw"
"uPnI"
"!+'*)ee,r.a/IE69*U__@Z+-}.s.(=+15WPAB9oKf)dP32IR./&^W/qkr>)ku=Cr;CVxG;0.<i^"
":`Up>QI6H}IYQCUFzB6]iA8Onf<+uW]pwScu|jSo3yi88;M.DE;S^A`dqvDF|lg5<A[/?$PiQL0c"
"_]H^_s^MyH(@&*pP:i\\_DESrb>?GwXM/2C>veJDwy*CP22\\~@+`vYUh)Wr?kYAKtjF[>wNmUyLW-"
"8bS~AnEXb1f91*n~9?P'K;cEz&@g{<LO4Z\\3hntbjNN^jk1wpspSqs`FtJ0A{3M~%;0W.`d41Ui/"
"25EGW\\@P`K>[a}?`buz'nKaOsrdeuciaAZ8{bd*:p/Ji"
"!*'(?|1(Ch^-592.<J+4%7<59V-5?e_CLf[E`VyK7AOZ9vs*ZUY:Rg}R2)CS/ecUR^{X\\Ql`WgP"
"fDJX.@V*0Q;O1EF55GW=7BL^B[9BTD<wU68)]J9*qyY*vxJ+'yT{-<U:1WQ74_\\J5_\\P@|_NK|57"
"PX@)a{Hdt3HZzd;;|&T73Jj53}UN4ZY}6Gr67}JJH+yceOB+f)=cle:[u\\oXviqCw1a[xQ@p|`f_"
"2+];DeReOXP[jxcNorm?'`BBGg\\QS%Hw]lcd5TG4@&UPDwJ9PE1.Z6'3[g;7_&wx`OFXqJPN(LlZ"
"+]RH<QE?DV3/Q61CTB8TYk4n^^;fe*JYhic(BVOSFj4:X\\:7]FAyeKS+uVYcuY@Q/}Zt9V00;O_j"
">,n9BR.cC?*<H*8BZ2:Ah)13&@as&e4=13i&3?\\)63UN7YJU:D?EE'SxRc_1Xu<Jek|Ah*[Jh|2;"
"jAcnjMZ.l>d+mWPFxEhs%|1S&I_a,AUL;U?L>%HPJ$V%mjZ&mpYio$?E|B>-"
"!)'8jZAUGN:cHV-6@N><c6SB2R4M=;ue+FMl0dZoXwtt%6={GLJ{vFb=]66?KDTBXGG\\LR)vG5R"
"{r\\e$/N$+.9r8yUi:2[2>{`1Av8GNH`k[`C&ec^+gJ`.zjve{M@g'X\\f/1<r20J{:}XHMI[.Y{=;"
"ZV7kadGkr$NQxAQ$'(Tm3{M5<tGu[,Wt_3TcpRa.qgXVsubNw~k0$wGU*CSi4'3D;~S@Bu<pDKRb"
"k+/Gm*]D5U<+7TK+@GErBe=6C3>aEi<GKg49LF(gb&F=g]Q`l:ucmHTd.X@m7YRLG?^.PN:JZb_u"
"vv=9y)g2ym@W.AGf0T_U2Dqk6Yku6`Qs=QR8N?O5QS7aae99zcE0;;X/;gnT<<@eF+,dF@jnI}b^"
"M~EZmVj/x90S"
"!('4_Xo=;W]?(9]AV=XDx4\\J$Po\\,NpsaV%wGdt|zIg<WTb@bC[A<D(Gyk(Jn4}L~]GWeYgaSF~"
"s8DRw(@S|vQv~&07(P`).Q_^3)1x5-B/9s4xGY?']f3Co6r{qj7(qrmnsFKBtWF.xKfX%1cI-Are"
"9od[H82CYdJB^rZqe:cZnoCKp$^xzdUf{)ie'.eW.fbV:ROO@Z?TP<[/V42SV<0gh8:'q}c;zSRx"
"%}1S&li2+]tO1/nN7uad<cET@/W%YDjw\\U^Q]&G/]e>X^R:TeKNOfgC,}amj$9HC7'?<><wSGvc8"
"[fZWa@Dxs5hq|,WH2x2@F3d:GwsyX$@RZ1:HuGtb"
"!''97S*GiPJJg5PL92fa[X'h&Bkt^\\pwph>x)ii}BBz1X<s:,^oF<D+H~9ffGu-n[])o5C2&Y9|"
"1y>D8KRW8|NZ@}BmI%l)_\\DC`kC(e~9ijy\\Ckl<ilWqtr=^WxAQ$<@MQDSq5GYO]IoJOM)>LN(2q"
"UT3([CYQ\\.q~]q=^mWDX{W\\t$BN3%*dE41j3B$GnMa1ll]I'q1b^+-R[8hME@bC[DgMHFh>?OK5s"
"TK3K^HK}_Wf:f0L{gVO3k'S.'Un[/7[b/7[b/{do2Itb:h[+AX86MXTbN7\\W_E^(`94.ahLoxrjl"
"!&'6z[m??C*?xi]@OBoSR=%^p:a_MD0gUS6g}UFiO1ri\\;qo,^u(\\]@2\\Dd5/XL6LV,:01N=xW["
"@4ATSw4iWb7>W}qF`88Gc0K\\dsc`h_CPzTj((N[T.8^s3$E$=*U|?1Cp?H>^De[hK845Ky3JQ*TA"
"SNM%Sa8ZW@lEjenelxQ4x-Qd}t]Y*qKa1,ik:uIUFLpk^h?mgSYGrq@tuIj["
"!%'7(Xe=}FzB,P@H3SCa>)_i$LUm>srq8XCu,G=+5/4+kBE4aXF5NX|ET6_G7BZGNEpSs2SViMu"
"gpB8mf`,sJV+u0M,vfOyvf4\\y:GZzuEk"
;

static int slow_stars = 0;

void initstars() {
  char *cp;
  float ra, dec, cd;
  int maxstars = 5000;
  int maxsizes = 32;
  float *stars = NewA(float, maxstars*3);
  float *starp;
  int nstars = 0;
  struct starsize *sizes = NewA(struct starsize, maxsizes);
  struct starsize *sizep;
  int i, nsizes = 0;
  char line[512];
  FILE *inf = fopen("stars.illi", "r");
  float r = caveyes ? 90. : 1.;
/*  const GLubyte *rend = glGetString(GL_RENDERER); */
  Point sp;
  static Matrix startfm = { 0,0,-1,0, -1,0,0,0, 0,1,0,0, 0,0,0,1 };

/*  if(rend==NULL || (rend[0]=='X'||rend[0]=='G'))
    slow_stars = 1; */

  starp = &stars[0];
  sizep = &sizes[0];
  sizep->size = 1;
  sizep->base = 0;
  sizep->color = 3;
  nsizes = 0;
  cp = stardata;
  do {
    if(inf) {
	line[sizeof(line)-1] = '\0';
	if(fgets(line, sizeof(line)-1, inf) == NULL)
	    break;
	cp = line;
    }
    for(;;) {
      if(*cp >= '$') {
	if(nstars >= maxstars-1) {
	    maxstars *= 3;
	    starp = NewA(float, 3*maxstars);
	    memcpy(starp, stars, nstars*3*sizeof(float));
	    stars = starp;
	    starp = &stars[nstars*3];
	}
	ra = ((cp[0] - SZERO)*SRADIX + (cp[1] - SZERO)) / SRASCALE;
	dec = ((cp[2] - SZERO)*SRADIX + (cp[3] - SZERO) - SDECBIAS) / SDECSCALE;
	cd = fcos(dec);
	sp.x[0] /*starp[2]*/ = -r * cd * cosf(ra);
	sp.x[1] /*starp[0]*/ = -r * cd * sinf(ra);
	sp.x[2] /*starp[1]*/ = r * sinf(dec);
	vtfmvector( (Point *)starp, &sp, startfm );

	cp += 4;
	starp += 3;
	nstars++;
      } else if(*cp == '#') {
	/* Comments.
	 * Also, "#@" hack allows specifying star transformation.
	 */
	if(cp[1] == '@') {
	  cp += 2;
	  for(i = 0; i<16; i++)
	    startfm[i] = strtod(cp, &cp);
	  cp--;
	}
	while(*++cp != '\n' && *cp != '\0')
	  ;
      } else if(*cp == '!') {
	sizep->count = nstars - sizep->base;
	if(sizep->count > 0)
	    nsizes++, sizep++;
	if(nsizes >= maxsizes) {
	    maxsizes *= 3;
	    sizep = NewA(struct starsize, maxsizes);
	    memcpy(sizep, sizes, nsizes*sizeof(struct starsize));
	    sizes = sizep;
	    sizep = &sizes[nsizes];
	}
	sizep->size = (cp[1] - SZERO) / (float)SSIZESCL;
	sizep->color = (cp[2] - SZERO);
	sizep->base = nstars;
	cp += 3;
      } else if(*cp == '\0') {
	break;
      } else {
	cp++;
      }
    }
  } while(inf || *cp != '\0');
  sizep->count = nstars - sizep->base;
  nstarsizes = nsizes+1;
  starsizes = NewN(struct starsize, nstarsizes);
  memcpy(starsizes, sizes, nstarsizes*sizeof(struct starsize));

  starpos = NewN(float, 3*nstars);
  memcpy(starpos, stars, nstars*3*sizeof(float));
}

#if 1

void drawstars(void) {
  int s, i;
  float v, size;
  struct starsize *sp;
  float *starp;

  if(getenv("NOSTARS"))
    return;
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  if(!slow_stars || zoom2)
    glEnable(GL_POINT_SMOOTH);
  glDisable(GL_DEPTH_TEST);
  glMultMatrixf(starmat);
  for(i = nstarsizes; --i >= 0; ) {
    sp = &starsizes[i];
    size = zoom2 ? 2*sp->size : sp->size;
    glPointSize( size );
    v = size < 1 ? size : 1 - (ceil(size) - size) * .25;
    glColor3f(	v*scolors[sp->color][0],
		v*scolors[sp->color][1],
		v*scolors[sp->color][2] );
    glBegin(GL_POINTS);
    for(s = sp->count, starp = &starpos[3*sp->base]; --s >= 0; starp += 3)
	glVertex3fv( starp );
    glEnd();
  }
  glPopMatrix();
  glDisable(GL_POINT_SMOOTH);
  glEnable(GL_DEPTH_TEST);
} 

#endif

