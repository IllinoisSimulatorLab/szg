#include "arPrecompiled.h"

#include "matrix.h"

#include "arDataUtilities.h"
#include "arLogStream.h"
#include "arGlut.h"

#include <math.h>
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

typedef struct parameter
{
    double value;
    double speed;
    int steps;
} Parameter;

rcMatrix pos;
Parameter roll, alignment, heading, pitch;
double tot_al = 0.0;
int tot = 0;

const int MAXX = 10000;
GLfloat x[MAXX], y[MAXX], z[MAXX];
GLfloat dx[MAXX], dy[MAXX], dz[MAXX];
GLfloat al[MAXX], rl[MAXX], hd[MAXX], pt[MAXX];
GLfloat strips[27][MAXX][3], normal[27][MAXX][3], bnormal[2][MAXX][3];
int opt[MAXX];
GLfloat r1[MAXX], r2[MAXX], r3[MAXX];

void update_parameters(void);
void update_parameter(Parameter *p);
void init_parameter(Parameter *p);

rcVector strips_tmp[27], normal_tmp[27];

extern string dataPath;

bool calculate_rc(void)
{
    FILE *in;
    int i, j;
    char cmd[256];
    double a;
    rcVector v;
    rcMatrix tmp, t2;
#if 0
    GLfloat nx, ny, nz, ac;
#endif

    const string dataFile("coaster_rc.def");
    in = ar_fileOpen( dataFile, "coaster", dataPath, "rb", "coaster" );
    if (!in)
        return false;

    init_parameter(&roll);
    init_parameter(&pitch);
    init_parameter(&heading);
    init_parameter(&alignment);

    init_matrix(&pos);
    pos.index[3][2] = 0;

    for (i=0;i<27;i++)
    {
        init_vector(&normal_tmp[i]);
        init_vector(&strips_tmp[i]);
    }

    for (i=0;i<8;i++)
    {
        strips_tmp[i].index[2] = 1.0;
        strips_tmp[i+8].index[2] = -1.0;
        strips_tmp[i+16].index[1] = -1.0;
    }

    for (i=0;i<3;i++)
    {
        for (j=0;j<8;j++)
        {
            normal_tmp[i*8+j].index[2] = cos(j*M_PI/4);
            normal_tmp[i*8+j].index[1] = sin(j*M_PI/4);
            strips_tmp[i*8+j].index[2] += cos(j*M_PI/4)/(i==2?2:4);
            strips_tmp[i*8+j].index[1] += sin(j*M_PI/4)/(i==2?2:4);
        }
        strips_tmp[24].index[2] = 1.0;
        strips_tmp[25].index[2] = -1.0;
        strips_tmp[26].index[1] = -1.0;
    }

    while (!feof(in))
    {
        for (i=0;i<256;i++)
        {
            int ch;

            ch = fgetc(in);
            if ((ch == '\n') || (ch == EOF))
            {
                cmd[i] = 0;
                break;
            }
            cmd[i] = ch;
        }
        if (*cmd == '#' || *cmd == '\r' || !*cmd)
            continue;
        if (sscanf(cmd, "pitch %lf %d", &a, &i))
        {
            pitch.speed = (a - pitch.value)/i;
            pitch.steps = i;
        }
        else if (sscanf(cmd, "alignment %lf %d", &a, &i))
        {
            alignment.speed = (a - alignment.value)/i;
            alignment.steps = i;
        }
        else if (sscanf(cmd, "heading %lf %d", &a, &i))
        {
            heading.speed = (a - heading.value)/i;
            heading.steps = i;
        }
        else if (sscanf(cmd, "roll %lf %d", &a, &i))
        {
            roll.speed = (a - roll.value)/i;
            roll.steps = i;
        }
        else if (sscanf(cmd, "wait %d", &i))
        {
            for (;i>=0;i--)
            {
                update_parameters();

                init_vector(&v);
                v.index[0] = 0.15;
                multiply_matrix_vector(&pos, &v);
                for (j=0;j<3;j++)
                    pos.index[3][j] = v.index[j];

                rotate_x(-roll.value*M_PI/(180*50), &pos);
                rotate_y(-heading.value*M_PI/(180*50), &pos);
                rotate_z(-pitch.value*M_PI/(180*50), &pos);

                x[tot] = v.index[0];
                y[tot] = v.index[1];
                z[tot] = v.index[2];
                al[tot] = alignment.value/50.0;
                rl[tot] = roll.value/50.0;
                hd[tot] = heading.value/50.0;
                pt[tot] = pitch.value/50.0;
                opt[tot] = int(100*fabs(rl[tot] - al[tot]) + 100*fabs(hd[tot])+
                    100*fabs(pt[tot]));

                copy_matrix(&tmp, &pos);
                init_vector(&v);
                v.index[1] = 1;
                tot_al += alignment.value*M_PI/180/50;
                rotate_x(-tot_al, &tmp);
                multiply_matrix_vector(&tmp, &v);
                dx[tot] = v.index[0] - tmp.index[3][0];
                dy[tot] = v.index[1] - tmp.index[3][1];
                dz[tot] = v.index[2] - tmp.index[3][2];

                copy_matrix(&tmp, &pos);
                tot_al += alignment.value*M_PI/180/50;
                rotate_x(-tot_al, &tmp);
                copy_matrix(&t2, &tmp);
                for (j=0;j<27;j++)
                {
                    copy_vector(&v, &strips_tmp[j]);
                    multiply_matrix_vector(&t2, &v);
                    
                    strips[j][tot][0] = v.index[0];
                    strips[j][tot][1] = v.index[1];
                    strips[j][tot][2] = v.index[2];

                    copy_vector(&v, &normal_tmp[j]);
                    multiply_matrix_vector(&t2, &v);
                    
                    normal[j][tot][0] = v.index[0] - t2.index[3][0];
                    normal[j][tot][1] = v.index[1] - t2.index[3][1];
                    normal[j][tot][2] = v.index[2] - t2.index[3][2];
                }
                init_vector(&v);
                v.index[0] = -1.0;
                v.index[1] = -1.5;
                multiply_matrix_vector(&pos, &v);
                bnormal[0][tot][0] = v.index[0] - pos.index[3][0];
                bnormal[0][tot][1] = v.index[1] - pos.index[3][1];
                bnormal[0][tot][2] = v.index[2] - pos.index[3][2];

                init_vector(&v);
                v.index[2] = -1.0;
                v.index[1] = 1.5;
                multiply_matrix_vector(&pos, &v);
                bnormal[0][tot][0] = v.index[0] - pos.index[3][0];
                bnormal[0][tot][1] = v.index[1] - pos.index[3][1];
                bnormal[0][tot][2] = v.index[2] - pos.index[3][2];

#if 0
                copy_matrix(&tmp, &pos);
                tmp.index[3][0] = 0.0;
                tmp.index[3][1] = 0.0;
                tmp.index[3][2] = 0.0;

                init_vector(&v);
                v.index[0] = 1.0;
                multiply_matrix_vector(&tmp, &v);

                nx = v.index[0];
                ny = v.index[1];
                nz = v.index[2];

                ac = sqrt(nx*nx+nz*nz);

                if (ac == 0.0)
                    r1[tot] = 0.0;
                else if (nx > 0)
                    r1[tot] = asin(nz/ac);
                else
                    r1[tot] = M_PI-asin(nz/ac);

                r2[tot] = asin(ny);

                rotate_y(-r1[tot], &tmp);
                rotate_z(-r2[tot], &tmp);
                
                init_vector(&v);
                v.index[1] = 1.0;
                multiply_matrix_vector(&tmp, &v);
                nx = v.index[0];
                ny = v.index[1];
                nz = v.index[2];

                ac = sqrt(nz*nz+ny*ny); /* this *should* be 1 */

                if (ac == 0.0)
                    r3[tot] = 0.0;
                else if (nz > 0)
                    r3[tot] = M_PI-asin(ny/ac);
                else
                    r3[tot] = asin(ny/ac);
#endif

                copy_matrix(&tmp, &pos);
                rotate_x(-tot_al, &tmp);
                if (tmp.index[0][0] == 0.0 && tmp.index[0][1] == 0.0) {
                    r1[tot] = atan2(- tmp.index[1][0], - tmp.index[2][0]);
                    r2[tot] = 0.5 * M_PI;
                    r3[tot] = 0.0;
                } else {
                    r1[tot] = atan2(tmp.index[1][2], tmp.index[2][2]);
                    r2[tot] = asin(tmp.index[0][2]);
                    r3[tot] = atan2(tmp.index[0][1], tmp.index[0][0]);
                }
                tot ++;
            }
        }
        else
           cerr << "coaster warning: ignoring command \"" << cmd << "\".\n";
    }
    ar_fileClose(in);
    return true;
}

void update_parameters(void)
{
    update_parameter(&roll);
    update_parameter(&pitch);
    update_parameter(&heading);
    update_parameter(&alignment);
}

void update_parameter(Parameter *p)
{
    if (!p->steps)
        return;
    p->steps--;
    p->value += p->speed;
}

void init_parameter(Parameter *p)
{
    p->value = 0;
    p->speed = 0;
    p->steps = 10;
}
