#include "colnorm.h"

double total_points = 0;
double actual_score = 0;
double max_score = 35.0;
int host_ok = 0;

void check_hostname();
#include "data.c"

int REPEATS = 2;               // repetitions to average
int WARMUP  = 1;               // warmup iterations to warm cache

int sizes[] = {
  1111, 2223,
  2049, 4098,
  4099, 8197,
  6001,12003,
};

int thread_counts[] = {
  1,
  2,
  3,
  4,
  // 8,
};

// use compile time size of global arrays to determine count of
// elements in them; this only works for global arrays
int nsizes = sizeof(sizes)/sizeof(int);
int nthread_counts = sizeof(thread_counts) / sizeof(int);

void print_result(long rows, long cols,
                  double cpu_time_BASE, int thread_count,
                  double cpu_time_OPTM, double speedup_OPTM,
                  double points, double total)
{
  if(rows == -1){               // print the header
    printf("%6s ","ROWS");
    printf("%6s ","COLS");
    printf("%6s ","BASE");
    printf("%2s ","T");
    printf("%6s ","OPTM");
    printf("%5s ","SPDUP");
    printf("%5s ","POINT");
    printf("%5s ","TOTAL");
  }
  else if(thread_count==1){     // print the normal stats
    printf("%6ld ", rows);
    printf("%6ld ", cols);
    printf("%6.3f ",cpu_time_BASE);
    printf("%2d ",  thread_count);
    printf("%6.3f ",cpu_time_OPTM);
    printf("%5.2f ",speedup_OPTM);
    printf("%5.2f ",points);
    printf("%5.2f ",total);
  }
  else{                         // omit the first two fields
    printf("%6s ", "");
    printf("%6s ", "");
    printf("%6s ","");
    printf("%2d ",  thread_count);
    printf("%6.3f ",cpu_time_OPTM);
    printf("%5.2f ",speedup_OPTM);
    printf("%5.2f ",points);
    printf("%5.2f ",total);
  }
  printf("\n");
}


// Timing data and functions. The Wall time (real world time) is
// returned as this benchmark expects to use
// multi-threading. Multi-threaded computations have the same cpu_time
// as single-threaded but take less Wall time if they are effective.

struct timeval beg_time, end_time;
clock_t begin, end;

void timing_start(){
  // begin = clock();
  gettimeofday(&beg_time, NULL);
}

double timing_stop(){
  // end = clock();
  // double cpu_time = ((double) (end - begin)) / CLOCKS_PER_SEC;

  gettimeofday(&end_time, NULL);
  double wall_time = 
    ((end_time.tv_sec-beg_time.tv_sec)) + 
    ((end_time.tv_usec-beg_time.tv_usec) / 1000000.0);
  return wall_time;             // real-world time
}

int main(int argc, char *argv[]){
  check_hostname();             // complain if not on a odd GRACE node

  if(argc > 1 && strcmp(argv[1],"-test")==0){
    nsizes = 6;                 // for valgrind testing
    REPEATS = 1;
    sizes[0] = 105; sizes[1] =  211;
    sizes[2] = 258; sizes[3] =  516;
    sizes[4] = 511; sizes[5] = 1021;
  }

  printf("==== Matrix Column Normalization Benchmark Version 1.1 ====\n");
  printf("Running with REPEATS: %d and WARMUP: %d\n",REPEATS,WARMUP);
  printf("Running with %d sizes and %d thread_counts (max %d)\n",
         nsizes/2, nthread_counts, thread_counts[nthread_counts-1]);

  print_result(-1,0,0,0,0,0,0,0);  // print header

  pb_srand(1234567);

  // Iterate over different sizes of the matrix
  for(int sidx=0; sidx<nsizes; sidx+=2){
    // long size = sizes[sidx];
    long rows=sizes[sidx+0];
    long cols=sizes[sidx+1];

    matrix_t mat_SRC;
    vector_t vec_SRC;
    matrix_init(&mat_SRC, rows, cols);
    vector_init(&vec_SRC, cols);
    matrix_fill_random(mat_SRC, -10,+10);                 // random values -10 to +10
    memset(vec_SRC.data, -1, sizeof(double)*vec_SRC.len);   // init vectors to -1

    matrix_t mat_BASE, mat_OPTM;
    vector_t avg_BASE, std_BASE, avg_OPTM, std_OPTM;
    matrix_init(&mat_BASE, rows, cols);
    vector_init(&avg_BASE, cols);
    vector_init(&std_BASE, cols);

    matrix_init(&mat_OPTM, rows, cols);
    vector_init(&avg_OPTM, cols);
    vector_init(&std_OPTM, cols);

    // BASELINE PERFORMANCE
    for(int i=0; i<WARMUP; i++){
      matrix_copy(&mat_BASE, &mat_SRC);
      vector_copy(&avg_BASE, &vec_SRC);
      vector_copy(&std_BASE, &vec_SRC);
      colnorm_BASE(&mat_BASE,&avg_BASE,&std_BASE);
    }

    double wall_time_BASE = 0.0;
    for(int i=0; i<REPEATS; i++){
      matrix_copy(&mat_BASE, &mat_SRC);
      vector_copy(&avg_BASE, &vec_SRC);
      vector_copy(&std_BASE, &vec_SRC);
      timing_start();
      colnorm_BASE(&mat_BASE,&avg_BASE,&std_BASE);
      wall_time_BASE += timing_stop();
    }


    // OPTIM PERFORMANCE THREAD LOOP
    for(int tidx = 0; tidx < nthread_counts; tidx++){
      int thread_count = thread_counts[tidx];

      for(int i=0; i<WARMUP; i++){
        matrix_copy(&mat_OPTM, &mat_SRC);
        vector_copy(&avg_OPTM, &vec_SRC);
        vector_copy(&std_OPTM, &vec_SRC);
        colnorm_OPTM(&mat_OPTM,&avg_OPTM,&std_OPTM,thread_count);
      }

      double wall_time_OPTM = 0.0;
      for(int i=0; i<REPEATS; i++){
        matrix_copy(&mat_OPTM, &mat_SRC);
        vector_copy(&avg_OPTM, &vec_SRC);
        vector_copy(&std_OPTM, &vec_SRC);
        timing_start();
        colnorm_OPTM(&mat_OPTM,&avg_OPTM,&std_OPTM,thread_count);
        wall_time_OPTM += timing_stop();
      }

      double speedup_OPTM = (wall_time_BASE / wall_time_OPTM);
      double points = log(speedup_OPTM) / log(2.0); // * size / 4099.0;
      if(points < 0){
        points = 0;
      }

      for(int i=0; i<avg_BASE.len; i++){
        double base_i = VGET(avg_BASE,i);
        double optm_i = VGET(avg_OPTM,i);
        double diff = fabsf(base_i - optm_i);
        if(diff > DIFFTOL){
          printf("ERROR: avg BASE and OPTM versions produced different results\n");
          printf("ERROR: avg[%d]: %8.4f != %8.4f\n",i,base_i,optm_i);
          printf("ERROR: Skipping checks on remaining elements\n");
          printf("ERROR: Try running the 'colnorm_print <size>' program to see all differences\n");
          speedup_OPTM = -1.0;
          points = 0;
          break;
        }
        base_i = VGET(std_BASE,i);
        optm_i = VGET(std_OPTM,i);
        diff = fabsf(base_i - optm_i);
        if(diff > DIFFTOL){
          printf("ERROR: std BASE and OPTM versions produced different results\n");
          printf("ERROR: std[%d]: %8.4f != %8.4f\n",i,base_i,optm_i);
          printf("ERROR: Skipping checks on remaining elements\n");
          printf("ERROR: Try running the 'colnorm_print <size>' program to see all differences\n");
          speedup_OPTM = -1.0;
          points = 0;
          break;
        }
      }

      for(int i=0; i<mat_BASE.rows; i++){
        for(int j=0; j<mat_BASE.cols; j++){
          double base_ij = MGET(mat_BASE,i,j);
          double optm_ij = MGET(mat_OPTM,i,j);
          double diff = fabsf(base_ij-optm_ij);
          if(diff > DIFFTOL){
            printf("ERROR: mat BASE and OPTM versions produced different results\n");
            printf("ERROR: mat[%d][%d]: %8.4f != %8.4f\n",i,j,base_ij,optm_ij);
            printf("ERROR: Skipping checks on remaining elements\n");
            printf("ERROR: Try running the 'colnorm_print <size>' program to see all differences\n");
            speedup_OPTM = -1.0;
            points = 0;
            break;
          }
        }
        if(speedup_OPTM <= -1.0){
          break;
        }
      }

      total_points += points;

      print_result(rows,cols, wall_time_BASE,
                   thread_count, wall_time_OPTM,
                   speedup_OPTM, points, total_points);
    } 
    // END THREAD COUNT LOOP

    matrix_free_data(&mat_BASE);       // clean up data
    vector_free_data(&avg_BASE);
    vector_free_data(&std_BASE);
    matrix_free_data(&mat_OPTM);
    vector_free_data(&avg_OPTM);
    vector_free_data(&std_OPTM);
    matrix_free_data(&mat_SRC);
    vector_free_data(&vec_SRC);

  }


  actual_score = total_points;
  printf("RAW POINTS: %.2f\n",actual_score);

  if(actual_score > max_score){
    actual_score = max_score;
    final_check();
  }

  printf("TOTAL POINTS: %.0f / %.0f\n",actual_score,max_score);

  check_hostname();

  return 0;
}
  
#define MAXHOSTNAMELEN 1024
#define FULL_EXPECT_HOST "grace5.umd.edu" // full expected host name
char *allowed_hosts[] = {
  "grace3.umd.edu",
  "grace4.umd.edu",
  "grace5.umd.edu",
  "grace6.umd.edu",
  "grace7.umd.edu",
  "grace8.umd.edu",
  "grace9.umd.edu",
  "grace10.umd.edu",
  NULL
};

void check_hostname(){
  char actual_host[MAXHOSTNAMELEN];
  if (gethostname(actual_host, MAXHOSTNAMELEN) != 0) {
    printf("WARNING: Couldn't get machine hostname\n");
    return;
  }
  for(int i=0; allowed_hosts[i]!=NULL; i++){
    if(strcmp(allowed_hosts[i], actual_host) == 0){
      host_ok = 1;
      return;
    }
  }
  printf("WARNING: expected host like '%s' but got host '%s'\n",FULL_EXPECT_HOST,actual_host);
  printf("WARNING: ensure you are on a grace node\n");
  printf("WARNING: timing results / scoring will not reflect actual scoring\n");
  printf("WARNING: run on one of the following hosts for accurate results\n");
  for(int i=0; allowed_hosts[i]!=NULL; i++){
    printf("WARNING:   %s\n",allowed_hosts[i]);
  }
  printf("WARNING: while on grace, try `ssh grace5` to log into a specifc node\n");
  return;
}
