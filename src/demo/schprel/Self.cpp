/* the self functions for Schprel */
/* Mark Flider, 2001 */
// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "Self.h"
#include "SpecialRelativityMath.h"

#include "arGraphicsHeader.h"

#define highquality 0

Self::Self(void)
{

/* head: */
   self[0][0] = -.2347; self[0][1] = 5.5432; self[0][2] = -.2856;
   self[1][0] = -.2347; self[1][1] = 5.5432; self[1][2] = 0.1167;
   self[2][0] = 0.2347; self[2][1] = 5.5432; self[2][2] = 0.1167;
   self[3][0] = 0.2347; self[3][1] = 5.5432; self[3][2] = -.2856;
   self[4][0] = -.1509; self[4][1] = 4.9230; self[4][2] = -.2856;
   self[5][0] = -.2179; self[5][1] = 5.2247; self[5][2] = 0.1167;
   self[6][0] = 0.2179; self[6][1] = 5.2247; self[6][2] = 0.1167;
   self[7][0] = 0.1509; self[7][1] = 4.9230; self[7][2] = -.2856;
/* upper torso: */   
   self[8][0]  = -.4506; self[8][1]  = 4.5423; self[8][2]  = -.2236;
   self[9][0]  = -.4506; self[9][1]  = 4.5423; self[9][2]  = 0.3172;
   self[10][0] = 0.4506; self[10][1] = 4.5423; self[10][2] = 0.3172;
   self[11][0] = 0.4506; self[11][1] = 4.5423; self[11][2] = -.2236;
   self[12][0] = -.3966; self[12][1] = 3.7492; self[12][2] = -.2596;
   self[13][0] = -.3966; self[13][1] = 3.7492; self[13][2] = 0.2811;
   self[14][0] = 0.3966; self[14][1] = 3.7492; self[14][2] = 0.2811;
   self[15][0] = 0.3966; self[15][1] = 3.7492; self[15][2] = -.2596;
/* lower torso: */   
   self[16][0] = -.3605; self[16][1] = 3.3527; self[16][2] = -.2056;
   self[17][0] = -.3605; self[17][1] = 3.3527; self[17][2] = 0.2270;
   self[18][0] = 0.3605; self[18][1] = 3.3527; self[18][2] = 0.2270;
   self[19][0] = 0.3605; self[19][1] = 3.3527; self[19][2] = -.2056;
   self[20][0] = -.3605; self[20][1] = 2.9561; self[20][2] = -.2056;
   self[21][0] = -.3605; self[21][1] = 2.9561; self[21][2] = 0.2270;
   self[22][0] = 0.3605; self[22][1] = 2.9561; self[22][2] = 0.2270;
   self[23][0] = 0.3605; self[23][1] = 2.9561; self[23][2] = -.2056;
/* upper right arm */   
   self[24][0] = 0.5223; self[24][1] = 4.5468; self[24][2] = -.0974;
   self[25][0] = 0.5223; self[25][1] = 4.5468; self[25][2] = 0.1549;
   self[26][0] = 0.7348; self[26][1] = 4.5005; self[26][2] = 0.1549;
   self[27][0] = 0.7348; self[27][1] = 4.5005; self[27][2] = -.0974;
   self[28][0] = 0.6656; self[28][1] = 3.7106; self[28][2] = -.0974;
   self[29][0] = 0.6656; self[29][1] = 3.7106; self[29][2] = 0.1549;
   self[30][0] = 0.9141; self[30][1] = 3.7544; self[30][2] = 0.1549;
   self[31][0] = 0.9141; self[31][1] = 3.7544; self[31][2] = -.0974;
/* lower right arm */   
   self[32][0] = 0.6875; self[32][1] = 3.5961; self[32][2] = -.1094;
   self[33][0] = 0.6875; self[33][1] = 3.4084; self[33][2] = 0.0079;
   self[34][0] = 0.9005; self[34][1] = 3.4283; self[34][2] = 0.0398;
   self[35][0] = 0.9005; self[35][1] = 3.6160; self[35][2] = -.0775;
   self[36][0] = 0.7994; self[36][1] = 3.2959; self[36][2] = -.5430;
   self[37][0] = 0.7994; self[37][1] = 3.1262; self[37][2] = -.4437;
   self[38][0] = 0.9764; self[38][1] = 3.1461; self[38][2] = -.4118;
   self[39][0] = 0.9764; self[39][1] = 3.3158; self[39][2] = -.5111;
/* right hand */   
   self[40][0] = 0.7291; self[40][1] = 3.3184; self[40][2] = -.7041;
   self[41][0] = 0.7291; self[41][1] = 3.0500; self[41][2] = -.5364;
   self[42][0] = 1.0132; self[42][1] = 3.0765; self[42][2] = -.4940;
   self[43][0] = 1.0132; self[43][1] = 3.3449; self[43][2] = -.6617;
   self[44][0] = 0.7824; self[44][1] = 3.1584; self[44][2] = -.9601;
   self[45][0] = 0.7824; self[45][1] = 2.8900; self[45][2] = -.7923;
   self[46][0] = 1.0664; self[46][1] = 2.9166; self[46][2] = -.7499;
   self[47][0] = 1.0664; self[47][1] = 3.1850; self[47][2] = -.9176;
/* right thigh */   
   self[48][0] = 0.1622; self[48][1] = 2.8119; self[48][2] = -.2416;
   self[49][0] = 0.1622; self[49][1] = 2.8119; self[49][2] = 0.2451;
   self[50][0] = 0.5047; self[50][1] = 2.8119; self[50][2] = 0.2451;
   self[51][0] = 0.5047; self[51][1] = 2.8119; self[51][2] = -.2416;
   self[52][0] = 0.2343; self[52][1] = 1.7304; self[52][2] = -.2777;
   self[53][0] = 0.2343; self[53][1] = 1.7304; self[53][2] = 0.1549;
   self[54][0] = 0.5227; self[54][1] = 1.7304; self[54][2] = 0.1549;
   self[55][0] = 0.5227; self[55][1] = 1.7304; self[55][2] = -.2777;
/* right shin */    
   self[56][0] = 0.2163; self[56][1] = 1.5862; self[56][2] = -.2056;
   self[57][0] = 0.2163; self[57][1] = 1.5862; self[57][2] = 0.2090;
   self[58][0] = 0.5948; self[58][1] = 1.5862; self[58][2] = 0.2090;
   self[59][0] = 0.5948; self[59][1] = 1.5862; self[59][2] = -.2056;
   self[60][0] = 0.3425; self[60][1] = 0.4867; self[60][2] = -.2056;
   self[61][0] = 0.3425; self[61][1] = 0.4867; self[61][2] = 0.2090;
   self[62][0] = 0.5768; self[62][1] = 0.4867; self[62][2] = 0.2090;
   self[63][0] = 0.5768; self[63][1] = 0.4867; self[63][2] = -.2056;
/* right foot */   
   self[64][0] = 0.3605; self[64][1] = 0.3425; self[64][2] = 0.1910;
   self[65][0] = 0.3064; self[65][1] = 0.0180; self[65][2] = 0.1910;
   self[66][0] = 0.7390; self[66][1] = 0.0180; self[66][2] = 0.1910;
   self[67][0] = 0.6489; self[67][1] = 0.3425; self[67][2] = 0.1910;
   self[68][0] = 0.3064; self[68][1] = 0.1442; self[68][2] = -.5661;
   self[69][0] = 0.3064; self[69][1] = 0.0180; self[69][2] = -.6382;
   self[70][0] = 0.7390; self[70][1] = 0.0180; self[70][2] = -.6382;
   self[71][0] = 0.7390; self[71][1] = 0.0721; self[71][2] = -.5661;
/* upper left arm */   
   self[72][0] = -1.0*self[24][0]; self[72][1] = self[24][1]; self[72][2] = self[24][2];
   self[73][0] = -1.0*self[25][0]; self[73][1] = self[25][1]; self[73][2] = self[25][2];   
   self[74][0] = -1.0*self[26][0]; self[74][1] = self[26][1]; self[74][2] = self[26][2];
   self[75][0] = -1.0*self[27][0]; self[75][1] = self[27][1]; self[75][2] = self[27][2];
   self[76][0] = -1.0*self[28][0]; self[76][1] = self[28][1]; self[76][2] = self[28][2];
   self[77][0] = -1.0*self[29][0]; self[77][1] = self[29][1]; self[77][2] = self[29][2];
   self[78][0] = -1.0*self[30][0]; self[78][1] = self[30][1]; self[78][2] = self[30][2];
   self[79][0] = -1.0*self[31][0]; self[79][1] = self[31][1]; self[79][2] = self[31][2];
/* lower left arm */
   self[80][0] = -1.0*self[32][0]; self[80][1] = self[32][1]; self[88][2] = self[32][2];
   self[81][0] = -1.0*self[33][0]; self[81][1] = self[33][1]; self[89][2] = self[33][2];   
   self[82][0] = -1.0*self[34][0]; self[82][1] = self[34][1]; self[82][2] = self[34][2];
   self[83][0] = -1.0*self[35][0]; self[83][1] = self[35][1]; self[83][2] = self[35][2];   
   self[84][0] = -1.0*self[36][0]; self[84][1] = self[36][1]; self[84][2] = self[36][2];
   self[85][0] = -1.0*self[37][0]; self[85][1] = self[37][1]; self[85][2] = self[37][2];
   self[86][0] = -1.0*self[38][0]; self[86][1] = self[38][1]; self[86][2] = self[38][2];
   self[87][0] = -1.0*self[39][0]; self[87][1] = self[39][1]; self[87][2] = self[39][2];
/* left hand */
   self[88][0] = -1.0*self[40][0]; self[88][1] = self[40][1]; self[88][2] = self[40][2];
   self[89][0] = -1.0*self[41][0]; self[89][1] = self[41][1]; self[89][2] = self[41][2];
   self[90][0] = -1.0*self[42][0]; self[90][1] = self[42][1]; self[90][2] = self[42][2];
   self[91][0] = -1.0*self[43][0]; self[91][1] = self[43][1]; self[91][2] = self[43][2];   
   self[92][0] = -1.0*self[44][0]; self[92][1] = self[44][1]; self[92][2] = self[44][2];
   self[93][0] = -1.0*self[45][0]; self[93][1] = self[45][1]; self[93][2] = self[45][2];   
   self[94][0] = -1.0*self[46][0]; self[94][1] = self[46][1]; self[94][2] = self[46][2];
   self[95][0] = -1.0*self[47][0]; self[95][1] = self[47][1]; self[95][2] = self[47][2];
/* left thigh */   
   self[96][0]  = -1.0*self[48][0]; self[96][1]  = self[48][1]; self[96][2]  = self[48][2];
   self[97][0]  = -1.0*self[49][0]; self[97][1]  = self[49][1]; self[97][2]  = self[49][2];
   self[98][0]  = -1.0*self[50][0]; self[98][1]  = self[50][1]; self[98][2]  = self[50][2];
   self[99][0]  = -1.0*self[51][0]; self[99][1]  = self[51][1]; self[99][2]  = self[51][2];
   self[100][0] = -1.0*self[52][0]; self[100][1] = self[52][1]; self[100][2] = self[52][2];
   self[101][0] = -1.0*self[53][0]; self[101][1] = self[53][1]; self[101][2] = self[53][2];   
   self[102][0] = -1.0*self[54][0]; self[102][1] = self[54][1]; self[102][2] = self[54][2];
   self[103][0] = -1.0*self[55][0]; self[103][1] = self[55][1]; self[103][2] = self[55][2];
/* left shin */   
   self[104][0] = -1.0*self[56][0]; self[104][1] = self[56][1]; self[104][2] = self[56][2];
   self[105][0] = -1.0*self[57][0]; self[105][1] = self[57][1]; self[105][2] = self[57][2];
   self[106][0] = -1.0*self[58][0]; self[106][1] = self[58][1]; self[106][2] = self[58][2];
   self[107][0] = -1.0*self[59][0]; self[107][1] = self[59][1]; self[107][2] = self[59][2];
   self[108][0] = -1.0*self[60][0]; self[108][1] = self[60][1]; self[108][2] = self[60][2];
   self[109][0] = -1.0*self[61][0]; self[109][1] = self[61][1]; self[109][2] = self[61][2];
   self[110][0] = -1.0*self[62][0]; self[110][1] = self[62][1]; self[110][2] = self[62][2];
   self[111][0] = -1.0*self[63][0]; self[111][1] = self[63][1]; self[111][2] = self[63][2];
/* left foot */      
   self[112][0] = -1.0*self[64][0]; self[112][1] = self[64][1]; self[112][2] = self[64][2];
   self[113][0] = -1.0*self[65][0]; self[113][1] = self[65][1]; self[113][2] = self[65][2];
   self[114][0] = -1.0*self[66][0]; self[114][1] = self[66][1]; self[114][2] = self[66][2];
   self[115][0] = -1.0*self[67][0]; self[115][1] = self[67][1]; self[115][2] = self[67][2];
   self[116][0] = -1.0*self[68][0]; self[116][1] = self[68][1]; self[116][2] = self[68][2];
   self[117][0] = -1.0*self[69][0]; self[117][1] = self[69][1]; self[117][2] = self[69][2];
   self[118][0] = -1.0*self[70][0]; self[118][1] = self[70][1]; self[118][2] = self[70][2];
   self[119][0] = -1.0*self[71][0]; self[119][1] = self[71][1]; self[119][2] = self[71][2];

  for (int i=0; i<120; i++)
    self[i][1] -= 5.;
}

/// Draws everything
void Self::drawAll(void){
  //float transp = .5;
  glColor3f(1,.1,1);/*glColor4f(1,.1,.1,transp);*/
      
  for (int aa=0; aa<119; aa+=8){
    glBegin(GL_TRIANGLE_FAN);
      glVertex3fv(selfdraw[aa+0]);
      glVertex3fv(selfdraw[aa+1]);
      glVertex3fv(selfdraw[aa+2]);
      glVertex3fv(selfdraw[aa+3]);
      glVertex3fv(selfdraw[aa+7]);
      glVertex3fv(selfdraw[aa+4]);
      glVertex3fv(selfdraw[aa+5]);
      glVertex3fv(selfdraw[aa+1]);
    glEnd();
        
    glBegin(GL_TRIANGLE_FAN);
      glVertex3fv(selfdraw[aa+6]);
      glVertex3fv(selfdraw[aa+7]);
      glVertex3fv(selfdraw[aa+3]);
      glVertex3fv(selfdraw[aa+2]);
      glVertex3fv(selfdraw[aa+1]);
      glVertex3fv(selfdraw[aa+5]);
      glVertex3fv(selfdraw[aa+4]);
      glVertex3fv(selfdraw[aa+7]);
    glEnd();
  }
}

/// Applies relativistic transforms to vertices of Self
void Self::updateAll(s_updateValues &updateValues)
{
  for (int aa=0; aa<120; aa++) {
    relativisticTransform(self[aa], selfdraw[aa], updateValues);
    selfdraw[aa][0] = 2.*self[aa][0]-selfdraw[aa][0];
    selfdraw[aa][1] = 2.*self[aa][1]-selfdraw[aa][1];
    selfdraw[aa][2] = 2.*self[aa][2]-selfdraw[aa][2];
  }
}


