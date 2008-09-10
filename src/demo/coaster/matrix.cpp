// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "matrix.h"
#include "math.h"

void init_matrix(rcMatrix *m)
{
    int i,j;
    for (i=0;i<4;i++)
        for (j=0;j<4;j++)
            m->index[i][j] = (i==j?1:0);
}

void init_vector(rcVector *v)
{
    int i;
    for (i=0;i<3;i++)
        v->index[i] = 0;
    v->index[3] = 1;
}

void copy_vector(rcVector *v1, rcVector *v2)
{
    int i;
    for (i=0;i<4;i++)
        v1->index[i] = v2->index[i];
}

void copy_matrix(rcMatrix *m1, rcMatrix *m2)
{
    int i,j;
    for (i=0;i<4;i++)
        for (j=0;j<4;j++)
            m1->index[i][j] = m2->index[i][j];
}

void multiply_vector_matrix(rcMatrix *m, rcVector *v)
{
    int i,j;
    rcVector t;
    for (i=0;i<4;i++)
    {
        t.index[i] = 0;
        for (j=0;j<4;j++)
            t.index[i] += m->index[i][j] * v->index[j];
    }
    copy_vector(v, &t);
}

void multiply_matrix_vector(rcMatrix *m, rcVector *v)
{
    int i,j;
    rcVector t;
    for (i=0;i<4;i++)
    {
        t.index[i] = 0;
        for (j=0;j<4;j++)
            t.index[i] += m->index[j][i] * v->index[j];
    }
    copy_vector(v, &t);
}

void multiply_matrix(rcMatrix *m2, rcMatrix *m1)
{
    int i,j,k;
    rcMatrix m;
    for (i=0;i<4;i++)
    {
        for (j=0;j<4;j++)
        {
            m.index[i][j] = 0;
            for (k=0;k<4;k++)
                m.index[i][j] += m1->index[i][k]* m2->index[k][j];
        }
    }
    copy_matrix(m2, &m);
}

void rotate_x(double angle, rcMatrix *m)
{
    rcMatrix r;
    double c = cos(angle), s = sin(angle);
    init_matrix(&r);
    r.index[1][1] = c;
    r.index[1][2] = s;
    r.index[2][1] = -s;
    r.index[2][2] = c;
    multiply_matrix(m, &r);
}

void rotate_y(double angle, rcMatrix *m)
{
    rcMatrix r;
    double c = cos(angle), s = sin(angle);
    init_matrix(&r);
    r.index[0][0] = c;
    r.index[0][2] = -s;
    r.index[2][0] = s;
    r.index[2][2] = c;
    multiply_matrix(m, &r);
}

void rotate_z(double angle, rcMatrix *m)
{
    rcMatrix r;
    double c = cos(angle), s = sin(angle);
    init_matrix(&r);
    r.index[0][0] = c;
    r.index[0][1] = s;
    r.index[1][0] = -s;
    r.index[1][1] = c;
    multiply_matrix(m, &r);
}

void mcount() {}
