#include "colnorm.h"

// Allocates memory for the parmeter vector vec. Sets its data field
// to point at a proper amount of memory and sets the len field
// according to parameter len. Returns 0 on success and nonzero
// if len is 0 or negative.
int vector_init(vector_t *vec, long len){
  if(len<=0){
    printf("Invalid length: %ld\n",len);
    return 1;
  }
  vec->data = malloc(sizeof(double) * len);
  vec->len = len;
  return 0;
}

// Allocates memory for the parmeter matrix mat. Sets its data field
// to point at a proper amount of memory and sets the rows,cols fields
// according to parameters rows,cols. Returns 0 on success and nonzero
// if rows,cols are 0 or negative.
//
// NOTE: this function attempts to align each row of doubles at a
// 16-byte boundary by allocating 1 extra entry per row if the number
// of columns is odd. This ensures that certain memory operations can
// assume rows start at addresses that are divisible by 16 which can
// improve performance in some cases.
int matrix_init(matrix_t *mat, long rows, long cols){
  if(rows<=0 || cols<=0){
    printf("Invalid rows or cols: %ld %ld\n",rows,cols);
    return 1;
  }
  mat->rows = rows;
  mat->cols = cols;
  mat->col_space = cols;
  if(cols % 2 == 1){
    mat->col_space++;
    // printf("matrix cols %ld to col_space %ld\n",mat->cols,mat->col_space);
  }
  mat->data = malloc(sizeof(double) * rows * mat->col_space);
  return 0;
}

// copy src matrix to dst; must be both initialized and of equal size
int matrix_copy(matrix_t *dst, matrix_t *src){
  if(dst->rows != src->rows || dst->cols != src->cols || dst->col_space != src->col_space) {
    printf("ERROR: size mismatch, couldn't copy\n");
    return -1;
  }
  memcpy(dst->data, src->data, sizeof(double)*src->rows*src->col_space);
  return 0;
}

// copy src vector to dst; must be both initialized and of equal size
int vector_copy(vector_t *dst, vector_t *src){
  if(dst->len != src->len){
    printf("ERROR: size mismatch, couldn't copy\n");
    return -1;
  }
  memcpy(dst->data, src->data, sizeof(double)*src->len);
  return 0;
}

// Frees memory associated with the data field of vec.
void vector_free_data(vector_t *vec){
  free(vec->data);
  vec->data = NULL;
  vec->len = -1;
}

// Frees memory associated with the data field of mat.
void matrix_free_data(matrix_t *mat){
  free(mat->data);
  mat->data = NULL;
  mat->rows = -1;
  mat->cols = -1;
  mat->col_space = -1;
}

// Reads data from the specified file and initializes specified vector
// with it. Allocated memory and checks for correct dimensions. The
// format of the file is space separated numbers.
// - first long indicates size of vector
// - remaining ints are data in the vector
// Returns 0 on success and non-zero on error.
int vector_read_from_file(char *fname, vector_t *vec_ref){
  FILE *file = fopen(fname,"r");
  if(file == NULL){
    perror("couldn't open vector file");
    return 1;
  }
  long len;
  assert(fscanf(file, "%ld",&len)==1);
  vector_t vec;
  int ret = vector_init(&vec,len);
  if(ret){
    return ret;
  }
  for(int i=0; i<len; i++){
    double x;
    assert(fscanf(file,"%lf",&x)==1);
    VSET(vec,i,x);
  }
  fclose(file);
  *vec_ref = vec;
  return 0;
}

// Reads data from the specified file and initializes specified matrix
// with it. Allocates memory and checks for correct dimensions. The
// format of the file is space separated numbers.
// - first two longs indicate size of matrix
// - remaining ints are data in the matrix
// Returns 0 on success and non-zero on error.
int matrix_read_from_file(char *fname, matrix_t *mat_ref){
  FILE *file = fopen(fname,"r");
  if(file == NULL){
    perror("couldn't open matrix file");
    return 1;
  }
  long rows, cols;
  assert(fscanf(file, "%ld %ld",&rows,&cols) == 2);
  matrix_t mat;
  int ret = matrix_init(&mat,rows,cols);
  if(ret){
    return ret;
  }
  for(int i=0; i<rows; i++){
    for(int j=0; j<cols; j++){
      double x;
      assert(fscanf(file,"%lf",&x)==1);
      MSET(mat,i,j,x);
    }
  }
  fclose(file);
  *mat_ref = mat;
  return 0;
}

// Writes a vector to an open file handle. Prints some dimension
// information followed by index and data on each line. Use with
// stdout to print to the screen.
void vector_write(FILE *file, vector_t vec){
  fprintf(file,"%ld x 1 vector\n",vec.len);
  for(int i=0; i<vec.len; i++){
    fprintf(file,"%4d: ",i);
    fprintf(file,"%6.2f\n", VGET(vec,i));
  }
  return;
}

// Writes a matrix to an open file handle. Prints some dimension
// information followed by index and data on each line. Use with
// stdout to print to the screen.
void matrix_write(FILE *file, matrix_t mat){
  fprintf(file,"%ld x %ld matrix\n",mat.rows,mat.cols);
  for(int i=0; i<mat.rows; i++){
    fprintf(file,"%4d: ",i);
    for(int j=0; j<mat.cols; j++){
      fprintf(file,"%6.2f ", MGET(mat,i,j));
    }
    fprintf(file,"\n");
  }
  return;
}


// Set elements of the given vector to 0,1,2,...,len
void vector_fill_sequential(vector_t vec){
  for(int i=0; i<vec.len; i++){
    VSET(vec,i,i);
  }
}

// Set elements of the given matrix to 0,1,2,...,len. 
void matrix_fill_sequential(matrix_t mat){
  int c = 0;
  for(int i=0; i<mat.rows; i++){
    for(int j=0; j<mat.cols; j++){
      MSET(mat,i,j,c);
      c++;
    }
  }
}

// getter + setters for vectors and matrices
int mget(matrix_t *mat, int i, int j){
  return mat->data[i*mat->col_space + j];
}

void mset(matrix_t *mat, int i, int j, int x){
  mat->data[i*mat->col_space + j] = x;
}

int vget(vector_t *vec, int i){
  return vec->data[i];
}
void vset(vector_t *vec, int i, int x){
  vec->data[i] = x;
}

// state of the random number generator for phase09 
unsigned long state = 1;

// generate a random integer
unsigned int pb_rand() {
  state = state * 1103515245 + 12345;
  return (unsigned int)(state/65536) % 32768;
}

// set seed for pb_rand()
void pb_srand(unsigned long seed){
  state = seed;
}

double pb_rand_double(double lo, double hi){
  int range = ((int) hi) - ((int)lo);
  double r = pb_rand() % range;
  r = r + lo;
  return r;
}

void vector_fill_random(vector_t vec, double lo, double hi){
  for(int i=0; i<vec.len; i++){
    VSET(vec,i,pb_rand_double(lo,hi));
  }
}

void matrix_fill_random(matrix_t mat, double lo, double hi){
  for(int i=0; i<mat.rows; i++){
    for(int j=0; j<mat.cols; j++){
      MSET(mat,i,j,pb_rand_double(lo,hi));
    }
  }
}
