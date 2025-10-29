// optimized version of matrix column normalization
#include "colnorm.h"

////////////////////////////////////////////////////////////////////////////////
// REQUIRED: Paste a copy of your sumdiag_benchmark from an ODD grace
// node below.
//
// -------REPLACE WITH YOUR RUN + TABLE --------
// 
// grace3:~/216-sync/p5-code: ./colnorm_benchmark
// ==== Matrix Column Normalization Benchmark Version 1.1 ====
// Running with REPEATS: 2 and WARMUP: 1
// Running with 4 sizes and 4 thread_counts (max 4)
//   ROWS   COLS   BASE  T   OPTM SPDUP POINT TOTAL 
//   1111   2223  0.030  1  0.017  1.70  0.76  0.76 
//                       2  0.019  1.54  0.62  1.39 
//                       3  0.016  1.90  0.93  2.31 
//                       4  0.014  2.09  1.06  3.38 
//   2049   4098  0.175  1  0.059  2.97  1.57  4.95 
//                       2  0.064  2.73  1.45  6.39 
//                       3  0.054  3.27  1.71  8.10 
//                       4  0.050  3.50  1.81  9.91 
//   4099   8197  2.388  1  0.240  9.96  3.32 13.23 
//                       2  0.264  9.03  3.18 16.40 
//                       3  0.213 11.20  3.49 19.89 
//                       4  0.199 12.01  3.59 23.48 
//   6001  12003  5.518  1  0.505 10.93  3.45 26.93 
//                       2  0.468 11.79  3.56 30.49 
//                       3  0.416 13.25  3.73 34.21 
//                       4  0.408 13.51  3.76 37.97 
// RAW POINTS: 37.97
// TOTAL POINTS: 35 / 35
// -------REPLACE WITH YOUR RUN + TABLE --------


// You can write several different versions of your optimized function
// in this file and call one of them in the last function.

int cn_verA(matrix_t *mat_ptr, vector_t *avg_ptr, vector_t *std_ptr, int thread_count) {
  // locally defined struct that contains the context
  // for the normalization including a thread id, param thread count,
  // a matrix struct, shared avg and std, and a shared lock
  typedef struct {
    int thread_id;
    int thread_count;
    matrix_t mat;
    vector_t *avg;
    vector_t *std;
    pthread_mutex_t *lock;  
  } norm_ctx_t;

  // worker function that is locally defined
  void *norm_worker(void *arg) {
    
    // extract the parameters / "context" via a caste
    // convert back from pointer into the struct
    norm_ctx_t *ctx = (norm_ctx_t *)arg;
    
    // initialize the matrix, cols, and row vars
    matrix_t mat = ctx->mat;
    long cols = mat.cols;
    long rows = mat.rows;

    // calculate how much work this thread should do and where its
    // begin/end rows are located. Leftover rows are handled by the last
    // thread.
    long rows_per_thread = rows / ctx->thread_count;
    long start_row = ctx->thread_id * rows_per_thread;
    long end_row = (ctx->thread_id == ctx->thread_count - 1) ? rows : start_row + rows_per_thread;

    // allocate memory for the thread to locally compute its results
    // avoiding the need to lock a mutex every time a computation is done
    // local_sum and then local_sumsq for the sum of the squares
    double *local_sum = malloc(cols * sizeof(double));
    double *local_sumsq = malloc(cols * sizeof(double));

    // initialize the arrays to 0
    for(int i = 0; i < cols; i++){
      local_sum[i] = 0;
      local_sumsq[i] = 0;
    }

    // iterate over the matrix using row-wise traversal since C is
    // a row-major language to optimize cache usage
    for (long i = start_row; i < end_row; i++) {
      for (long j = 0; j < cols; j++) {
        double val = MGET(mat, i, j);
        local_sum[j] += val;
        local_sumsq[j] += val * val;
      }
    }

    // lock the mutex to get controlled access to the shared
    // results in order to prevent potential corruption
    pthread_mutex_lock(ctx->lock);

    // begin computations
    for (long j = 0; j < cols; j++) {
      double prev_sum = VGET(*ctx->avg, j);
      double prev_sumsq = VGET(*ctx->std, j);  
      VSET(*ctx->avg, j, prev_sum + local_sum[j]);
      VSET(*ctx->std, j, prev_sumsq + local_sumsq[j]);
    }
    
    // unlock the mutex before exiting the worker
    pthread_mutex_unlock(ctx->lock);

    // free the allocated memory for the shared results
    // before exiting the worker and return NULL
    free(local_sum);
    free(local_sumsq);
    return NULL;
  }

  // initialize both the avg and std vectors using macro VSET
  for (long i = 0; i < mat_ptr->cols; i++) {
    VSET(*avg_ptr, i, 0.0);
    VSET(*std_ptr, i, 0.0);
  }

  // define the lock and initialize it with NULL values
  pthread_mutex_t vec_lock;
  pthread_mutex_init(&vec_lock, NULL);

  // track each thread and create a context struct for each thread
  // which will hold information dependent upon the thread and the 
  // work that it is assigned via the worker function
  pthread_t threads[thread_count];
  norm_ctx_t ctxs[thread_count];

  // loop to create threads
  // initialize the contexts and ensure that each thread has
  // a unique id
  for (int i = 0; i < thread_count; i++) {
    ctxs[i].thread_id = i;
    ctxs[i].thread_count = thread_count;
    ctxs[i].mat = *mat_ptr;
    ctxs[i].avg = avg_ptr;
    ctxs[i].std = std_ptr;
    ctxs[i].lock = &vec_lock;

    pthread_create(&threads[i], NULL, norm_worker, &ctxs[i]);
  }
  
  // loop to join the threads
  for (int i = 0; i < thread_count; i++) {
    pthread_join(threads[i], NULL);
  }

  // get rid of the lock to avoid a memory leak
  pthread_mutex_destroy(&vec_lock);

  // finalize computations
  for (long j = 0; j < mat_ptr->cols; j++) {
    double sum = VGET(*avg_ptr, j);
    double sumsq = VGET(*std_ptr, j);
    double mean = sum / mat_ptr->rows;
    double variance = (sumsq / mat_ptr->rows) - (mean * mean);
    double stddev = sqrt(variance);
    VSET(*avg_ptr, j, mean);
    VSET(*std_ptr, j, stddev);
  }

  // finaly normalize the matrix via row-wise traversal for efficiency 
  for (long i = 0; i < mat_ptr->rows; i++) {
    for (long j = 0; j < mat_ptr->cols; j++) {
      double val = MGET(*mat_ptr, i, j);
      double mean = VGET(*avg_ptr, j);
      double stddev = VGET(*std_ptr, j);
      MSET(*mat_ptr, i, j, (val - mean) / stddev);
    }
  }

  // now the matrix should be normalized via concurrency
  // so return 0
  return 0;
}

int cn_verB(matrix_t *mat_ptr, vector_t *avg_ptr, vector_t *std_ptr, int thread_count) {
  // OPTIMIZED CODE HERE
  return 0;
}


int colnorm_OPTM(matrix_t *mat_ptr, vector_t *avg_ptr, vector_t *std_ptr, int thread_count){
  // call version A of the function
  return cn_verA(mat_ptr, avg_ptr, std_ptr, thread_count);
}

////////////////////////////////////////////////////////////////////////////////
// REQUIRED: DON'T FORGET TO PASTE YOUR TIMING RESULTS FOR
// sumdiag_benchmark FROM A GRACE NODE AT THE TOP OF THIS FILE
////////////////////////////////////////////////////////////////////////////////
