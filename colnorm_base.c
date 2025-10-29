// colnorm_base.c: baseline version of column normalization
#include "colnorm.h"

// Baseline version which normalizes each column of a matrix to have
// average 0.0 and standard deviation 1.0. During the computation, the
// vectors avg/std are set to the average and standard deviation of
// the original matrix. Elements in mat are modified so that each
// column is normalized.
int colnorm_BASE_1(matrix_t *mat_ptr, vector_t *avg_ptr, vector_t *std_ptr) {
  matrix_t mat = *mat_ptr;                   // deref structs to get local copies
  vector_t avg = *avg_ptr;                   // which may speed up some operations
  vector_t std = *std_ptr;

  // for(int i=0; i<avg.len; i++){           // initialize avg/std to all 0s
  //   VSET(avg, i, 0.0);                    // not necessary here but needed
  //   VSET(std, i, 0.0);                    // for some algorithms; memset()
  // }                                       // may also be used here

  for(int j=0; j<mat.cols; j++){             // for each column in matrix

    double sum_j = 0.0;                      // PASS 1: Compute column average
    for(int i=0; i<mat.rows; i++){                 
      sum_j += MGET(mat,i,j);
    }
    double avg_j = sum_j / mat.rows;
    VSET(avg,j,avg_j);
    sum_j = 0.0;

    for(int i=0; i<mat.rows; i++){           // PASS 2: Compute standard deviation
      double diff = MGET(mat,i,j) - avg_j;
      sum_j += diff*diff;
    };
    double std_j = sqrt(sum_j / mat.rows);
    VSET(std,j,std_j);

    for(int i=0; i<mat.rows; i++){           // PASS 3: Normalize matrix column
      double mij = MGET(mat,i,j);
      mij = (mij - avg_j) / std_j;
      MSET(mat,i,j,mij);
    }
  }
  return 0;
}
// END COLNORM_BASE_1

// Debugging version which prints what it's doing at each step
int colnorm_BASE_DEBUG(matrix_t *mat_ptr, vector_t *avg_ptr, vector_t *std_ptr) {
  matrix_t mat = *mat_ptr;                   // deref structs to get local copies
  vector_t avg = *avg_ptr;                   // which may speed up some operations
  vector_t std = *std_ptr;

  if(avg.len != mat.cols || std.len != mat.cols){
    printf("colnorm_base: bad sizes\n");
    return 1;
  }
  // for(int i=0; i<avg.len; i++){                    // initialize avg/std to all 0s
  //   VSET(avg, i, 0.0);                             // may not be necessary for some 
  //   VSET(std, i, 0.0);                             // algorithmic approaches
  // }

  printf("Beginning main loop over columns\n");
  for(int j=0; j<mat.cols; j++){
    double sum_j = 0.0;                             // PASS 1: Compute column average
    for(int i=0; i<mat.rows; i++){                 
      sum_j += MGET(mat,i,j);
    }
    double avg_j = sum_j / mat.rows;
    printf("Setting average for col %d to %f\n",j,avg_j);
    VSET(avg,j,avg_j);
    sum_j = 0.0;
    for(int i=0; i<mat.rows; i++){                 // PASS 2: Compute column standard deviation
      double diff = MGET(mat,i,j) - avg_j;
      sum_j += diff*diff;
    };
    double std_j = sqrt(sum_j);
    printf("Setting std dev for col %d to %f\n",j,std_j);
    VSET(std,j,std_j);
    for(int i=0; i<mat.rows; i++){                 // PASS 3: Normalize matrix column
      double mij = MGET(mat,i,j);
      mij = (mij - avg_j) / std_j;
      MSET(mat,i,j,mij);
    }
    printf("Column %d is normalized\n",j);
  }
  return 0;
}

int colnorm_BASE(matrix_t *mat_ptr, vector_t *avg_ptr, vector_t *std_ptr) {
  if(avg_ptr->len != mat_ptr->cols || std_ptr->len != mat_ptr->cols){
    printf("colnorm_base: bad sizes\n");
    return 1;
  }
  return colnorm_BASE_1(mat_ptr, avg_ptr, std_ptr);
}
