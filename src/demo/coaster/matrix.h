typedef struct rcmatrix {
    double index[4][4];
} rcMatrix;

typedef struct rcvector {
    double index[4];
} rcVector;

void init_matrix(rcMatrix *m);
void init_vector(rcVector *v);
void copy_vector(rcVector *v1, rcVector *v2);
void copy_matrix(rcMatrix *m1, rcMatrix *m2);
void multiply_vector_matrix(rcMatrix *m, rcVector *v);
void multiply_matrix_vector(rcMatrix *m, rcVector *v);
void multiply_matrix(rcMatrix *m1, rcMatrix *m2);
void rotate_x(double angle, rcMatrix *m);
void rotate_y(double angle, rcMatrix *m);
void rotate_z(double angle, rcMatrix *m);
