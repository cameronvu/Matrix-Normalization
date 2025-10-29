// colnorm.h: matrix column normalization header
#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <error.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <pthread.h>            // anticipating threading

#define DIFFTOL 1e-04           // tolerated difference between expect/actual answers

typedef struct {
  long rows;                    // number of rows
  long cols;                    // number of columns
  long col_space;               // actual space for columns to allow for alignment
  double *data;                 // pointer to allocated data
} matrix_t;

typedef struct {
  long len;                     // length of vector
  double *data;                 // data in vector
} vector_t;

#define MGET(mat,i,j) ((mat).data[((i)*((mat).col_space)) + (j)])
#define VGET(vec,i)   ((vec).data[(i)])

#define MSET(mat,i,j,x) ((mat).data[((i)*((mat).col_space)) + (j)] = (x))
#define VSET(vec,i,x)   ((vec).data[(i)] = (x))


// colnorm_util.c
int vector_init(vector_t *vec, long len);
int matrix_init(matrix_t *mat, long rows, long cols);
int matrix_copy(matrix_t *dst, matrix_t *src);
int vector_copy(vector_t *dst, vector_t *src);
void vector_free_data(vector_t *vec);
void matrix_free_data(matrix_t *mat);
int vector_read_from_file(char *fname, vector_t *vec_ref);
int matrix_read_from_file(char *fname, matrix_t *mat_ref);
void vector_write(FILE *file, vector_t vec);
void matrix_write(FILE *file, matrix_t mat);
void vector_fill_sequential(vector_t vec);
void matrix_fill_sequential(matrix_t mat);
int mget(matrix_t *mat, int i, int j);
void mset(matrix_t *mat, int i, int j, int x);
int vget(vector_t *vec, int i);
void vset(vector_t *vec, int i, int x);

void pb_srand(unsigned long seed);
unsigned int pb_rand();
void vector_fill_random(vector_t vec, double lo, double hi);
void matrix_fill_random(matrix_t mat, double lo, double hi);

// colnorm_base.c
int colnorm_BASE(matrix_t *mat_ptr, vector_t *avg_ptr, vector_t *std_ptr);

// colnorm_optm.c
int colnorm_OPTM(matrix_t *mat_ptr, vector_t *avg_ptr, vector_t *std_ptr, int thread_count);

