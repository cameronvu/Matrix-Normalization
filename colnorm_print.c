// colnorm_print.c: print out results of column normalization for
// testing / debugging.

#include "colnorm.h"

int main(int argc, char *argv[]){
  if(argc < 4){
    printf("usage: %s <rows> <cols> <thread_count>\n",argv[0]);
    exit(1);
  }

  printf("==== Matrix Column Normalization Print ====\n");
  long rows = atoi(argv[1]);
  long cols = atoi(argv[2]);
  long thread_count = atoi(argv[3]);

  printf("rows: %ld  cols: %ld  threads: %ld\n",rows,cols,thread_count);
  pb_srand(1234567);

  matrix_t mat_BASE, mat_OPTM;
  vector_t avg_BASE, std_BASE, avg_OPTM, std_OPTM;
  matrix_init(&mat_BASE, rows, cols);
  vector_init(&avg_BASE, cols);
  vector_init(&std_BASE, cols);
  matrix_fill_random(mat_BASE, -10,+10);                 // random values -10 to +10
  memset(avg_BASE.data, -1, sizeof(double)*avg_BASE.len); // init vectors to -1
  memset(std_BASE.data, -1, sizeof(double)*std_BASE.len); // must reset vals to 0 when summing

  matrix_init(&mat_OPTM, rows, cols);
  vector_init(&avg_OPTM, cols);
  vector_init(&std_OPTM, cols);
  matrix_copy(&mat_OPTM, &mat_BASE);
  vector_copy(&avg_OPTM, &avg_BASE);
  vector_copy(&std_OPTM, &std_BASE);

  printf("Matrix:\n");
  matrix_write(stdout, mat_BASE);
  printf("\n");

  colnorm_BASE(&mat_BASE,&avg_BASE,&std_BASE);              // call baseline algorithm
  colnorm_OPTM(&mat_OPTM,&avg_OPTM,&std_OPTM,thread_count); // call optimized algorithm

  printf("========== avg ==========\n");
  printf("[ i]: %8s %8s\n","BASE","OPTM");
  for(int i=0; i<avg_BASE.len; i++){
    double base_i = VGET(avg_BASE,i);
    double optm_i = VGET(avg_OPTM,i);
    double diff = fabs(base_i - optm_i);
    char *sdiff = (isnan(optm_i) || diff > DIFFTOL) ? "***" : "";
    printf("[%2d]: %8.4f %8.4f %s\n",i,base_i,optm_i,sdiff);
  }

  printf("========== std ==========\n");
  printf("[ i]: %8s %8s\n","BASE","OPTM");
  for(int i=0; i<std_BASE.len; i++){
    double base_i = VGET(std_BASE,i);
    double optm_i = VGET(std_OPTM,i);
    double diff = fabs(base_i - optm_i);
    char *sdiff = (isnan(optm_i) || diff > DIFFTOL) ? "***" : "";
    printf("[%2d]: %8.4f %8.4f %s\n",i,base_i,optm_i,sdiff);
  }
    
  printf("========== mat ==========\n");
  printf("[ i][ j]: %8s %8s\n","BASE","OPTM");
  for(int i=0; i<mat_BASE.rows; i++){
    for(int j=0; j<mat_BASE.cols; j++){
      double base_ij = MGET(mat_BASE,i,j);
      double optm_ij = MGET(mat_OPTM,i,j);
      double diff = fabsf(base_ij - optm_ij);
      char *sdiff = (diff > DIFFTOL) ? "***" : "";
      printf("[%2d][%2d]: %8.4f %8.4f %s\n",i,j,base_ij,optm_ij,sdiff);
    }
  }

  matrix_free_data(&mat_BASE);       // clean up data
  vector_free_data(&avg_BASE);
  vector_free_data(&std_BASE);
  matrix_free_data(&mat_OPTM);
  vector_free_data(&avg_OPTM);
  vector_free_data(&std_OPTM);
  return 0;
}
