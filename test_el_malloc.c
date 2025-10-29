// el_malloc.c test program
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdlib.h>
#include "el_malloc.h"

#define HEAP_SIZE 1024
#define MAX_BLOCKS 10           // max blocks used during testing


char sysbuf[1024];              // used for system calls during tests
int RUNALL = 0;                 // 1 for run all tests
int nrun = 0;                   // number of tests run

// macro to set up a test with given name, print the source of the
// test; very hacky, fragile, but useful
#define IF_TEST(TNAME)                                                  \
  if( RUNALL ) { printf("\n"); }                                        \
  if( RUNALL || strcmp( TNAME, test_name)==0 ) {                        \
    sprintf(sysbuf,"awk 'NR==(%d){P=1;gsub(\"^ *\",\"\");} P==1 && /ENDTEST/{P=0; print \"}\\n---OUTPUT---\"} P==1{print}' %s", __LINE__, __FILE__); \
    system(sysbuf); nrun++;                                             \
  }                                                                     \
  if( RUNALL || strcmp( TNAME, test_name)==0 )

void print_ptr(char *str, void *ptr){
  if(ptr == NULL){
    printf("%s: (nil)\n", str);
  }
  else{
    printf("%s: %p\n", str, ptr);
  }
}
void print_ptrs(void *ptr[], int len){
  char buf[128];
  for(int i=0; i<len; i++){
    snprintf(buf,128,"ptr[%2d]",i);
    print_ptr(buf,ptr[i]);
  }
}

el_blockhead_t *heads[MAX_BLOCKS]; // globals used in some tests to set up 
el_blockfoot_t *foots[MAX_BLOCKS]; // testing heap properties
int nblocks = 0;

void make_test_heap1(){
  nblocks = 5;
  int sizes[MAX_BLOCKS] = { 100, 200, 1000, 500, -1};
  char state[MAX_BLOCKS] = {EL_USED, EL_AVAILABLE, EL_USED, EL_USED, EL_AVAILABLE};
  size_t bytes_rem = el_ctl->heap_bytes;
  for(int i=0; i<nblocks; i++){
    heads[i] = (i==0) ? el_ctl->heap_start : el_block_above(heads[i-1]);
    heads[i]->size = (i!=nblocks-1) ? sizes[i] : bytes_rem - EL_BLOCK_OVERHEAD;
    heads[i]->state = state[i];
    foots[i] = el_get_footer(heads[i]);
    foots[i]->size = heads[i]->size;
    bytes_rem -= sizes[i] + EL_BLOCK_OVERHEAD;
  }      
}

int main(int argc, char *argv[]){
  if(argc < 2){
    printf("usage: %s <test_name>\n", argv[0]);
    printf("       %s ALL\n", argv[0]);
    return 1;
  }
  char *test_name = argv[1];
  char sysbuf[1024];

  RUNALL = strcmp(test_name,"ALL")==0; // possibly run all tests

  system("mkdir -p test-results"); // ensure a subdirectory for data is present

  el_init(EL_HEAP_DEFAULT_SIZE);
  
  IF_TEST("el_get_header") {
    // establish some test blocks and check that the el_get_header()
    // function works on each of them; uses global variables for the
    // foots[] array and nblocks
    make_test_heap1();
    printf("--Heap Blocks--\n");
    el_print_heap_blocks();
    printf("\n--Calls to el_get_header()--\n");
    for(int i=0; i<nblocks; i++){
      printf("el_get_header(foots[%d]): %p\n",i,el_get_header(foots[i]));
    }
  } // ENDTEST

  IF_TEST("el_block_below") {
    // establish some test blocks and check that the el_block_below()
    // function works on each of them; uses global variables for the
    // heads[] array and nblocks
    make_test_heap1();
    printf("--Heap Blocks--\n");
    el_print_heap_blocks();
    printf("\n--Calls to el_block_below()--\n");
    for(int i=0; i<nblocks; i++){
      printf("el_block_below(heads[%d]): %p\n",i,el_block_below(heads[i]));
    }
  } // ENDTEST

  IF_TEST("el_block_above") {
    // establish some test blocks and check that the el_block_above()
    // function works on each of them; uses global variables for the
    // heads[] array and nblocks. This function is provided so this
    // test is not run for students.
    make_test_heap1();
    printf("--Heap Blocks--\n");
    el_print_heap_blocks();
    printf("\n--Calls to el_block_above()--\n");
    for(int i=0; i<nblocks; i++){
      printf("el_block_above(heads[%d]): %p\n",i,el_block_above(heads[i]));
    }
  } // ENDTEST

  IF_TEST("Single Allocation" ) {
    // Tests that an allocation correctly splits the first single
    // block into 2 blocks, one used and one available

    printf("BEFORE MALLOC 0\n"); el_print_stats(); printf("\n");

    void *p0 = el_malloc(128);
    printf("AFTER MALLOC 0\n"); el_print_stats(); printf("\n");

    printf("POINTERS\n");
    print_ptr("p0",p0);
  } // ENDTEST

  IF_TEST("Three Allocs" ) {
    // Tests that 3 allocations in a row correctly split off the main
    // block leaving 4 blocks, Used x 3 + Available x 1

    void *ptr[16] = {};
    int len = 0;

    ptr[len++] = el_malloc(128);
    printf("\nMALLOC 0\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(200);
    printf("\nMALLOC 1\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(64);
    printf("\nMALLOC 2\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);
  } // ENDTEST

  IF_TEST("Required Basics" ) {
    // Allocates 3 blocks then tests that required functions like
    // el_get_header() and el_block_below() correctly calculate the
    // positions of block parts and adjacent blocks.

    void *ptr[16] = {};
    int len = 0;

    ptr[len++] = el_malloc(128);
    ptr[len++] = el_malloc(200);
    ptr[len++] = el_malloc(64);

    printf("Heap after 3 allocations\n");
    el_print_stats();

    el_blockhead_t *head = el_ctl->used->beg->next;
    el_blockfoot_t *foot;

    foot = el_get_footer(head);
    head = el_get_header(foot);
    print_ptr("used head 0",head);
    print_ptr("used foot 0",foot);

    head = el_block_below(head);
    foot = el_get_footer(head);
    head = el_get_header(foot);
    print_ptr("used head 1",head);
    print_ptr("used foot 1",foot);

    head = el_block_below(head);
    foot = el_get_footer(head);
    head = el_get_header(foot);
    print_ptr("used head 2",head);
    print_ptr("used foot 2",foot);

    head = el_block_below(head);
    printf("used head below 2 is: %p\n",head);

    printf("POINTERS\n"); print_ptrs(ptr, len);
  } // ENDTEST

  IF_TEST("One Free No Merge" ) {
    // Tests that 2 allocations in a row correctly split off the main
    // block leaving 3 blocks and that free()'ing the first block will
    // move it back to the available list. No merging is needed in
    // this case.

    void *ptr[16] = {};
    int len = 0;

    ptr[len++] = el_malloc(500);
    printf("\nMALLOC 0\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(400);
    printf("\nMALLOC 1\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    el_free(ptr[0]);
    printf("\nFREE 0\n"); el_print_stats(); printf("\n");
  } // ENDTEST

  IF_TEST("Several Free No Merge" ) {
    // Tests that several allocations in a row correctly split off the
    // main block that free()'ing alternating blocks will move them
    // back to the available list. No merging is needed in this case.
    // For reference, the last digit of the size for each block
    // corresponds to its allocation order (e.g. size 100 is the 0th
    // block to be malloc()'d and size 105 is the 5th block to be
    // malloc()'d.
    void *ptr[16] = {};
    int len = 0;

    ptr[len++] = el_malloc(100);
    printf("\nMALLOC 0\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(101);
    printf("\nMALLOC 1\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    el_free(ptr[0]);
    printf("\nFREE 0\n"); el_print_stats(); printf("\n");

    ptr[len++] = el_malloc(102);
    printf("\nMALLOC 2\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(103);
    printf("\nMALLOC 3\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(104);
    printf("\nMALLOC 4\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    el_free(ptr[3]);
    printf("\nFREE 3\n"); el_print_stats(); printf("\n");

    ptr[len++] = el_malloc(105);
    printf("\nMALLOC 5\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(106);
    printf("\nMALLOC 6n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    el_free(ptr[5]);
    printf("\nFREE 5\n"); el_print_stats(); printf("\n");
  } // ENDTEST

  IF_TEST("Single Allocate/Free" ) {
    // Tests a single allocate followed by freeing that block. The
    // free()'d block should be merged with the other available block
    // into a single large available block. Checks that a block will
    // merge with a block above it.
    void *ptr[16] = {};
    int len = 0;

    ptr[len++] = el_malloc(128);
    printf("\nMALLOC 0\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    el_free(ptr[0]);
    printf("\nFREE 0\n"); el_print_stats(); printf("\n");
  } // ENDTEST

  IF_TEST("Merge Below" ) {
    // mallocs()'s sevearl blocks then issues two free()'s. The 2nd
    // free()'d block should be merged with the block below it into a
    // single block.
    void *ptr[16] = {};
    int len = 0;

    ptr[len++] = el_malloc(500);
    printf("\nMALLOC 0\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(501);
    printf("\nMALLOC 1\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(501);
    printf("\nMALLOC 2\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    el_free(ptr[0]);
    printf("\nFREE 0\n"); el_print_stats(); printf("\n");

    el_free(ptr[1]);
    printf("\nFREE 1\n"); el_print_stats(); printf("\n");
  } // ENDTEST

  IF_TEST("Four Allocs Free Ver1" ) {
    // Allocates 4 times the frees 4 times in the order that the
    // allocation occured in. Looks for merging of blocks.

    void *ptr[16] = {};
    int len = 0;

    ptr[len++] = el_malloc(128);
    printf("\nMALLOC 0\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(200);
    printf("\nMALLOC 1\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(64);
    printf("\nMALLOC 2\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(312);
    printf("\nMALLOC 3\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    el_free(ptr[0]);
    printf("\nFREE 0\n"); el_print_stats(); printf("\n");

    el_free(ptr[1]);
    printf("\nFREE 1\n"); el_print_stats(); printf("\n");

    el_free(ptr[2]);
    printf("\nFREE 2\n"); el_print_stats(); printf("\n");

    el_free(ptr[3]);
    printf("\nFREE 3\n"); el_print_stats(); printf("\n");
  } // ENDTEST

  IF_TEST("Four Allocs Free Ver2" ) {
    // Allocates 4 times again but free()'s in a different order. This
    // prevents some merging and requires different cases of merging
    // above/below.

    void *ptr[16] = {};
    int len = 0;

    ptr[len++] = el_malloc(128);
    printf("\nMALLOC 0\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(200);
    printf("\nMALLOC 1\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(64);
    printf("\nMALLOC 2\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(312);
    printf("\nMALLOC 3\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    el_free(ptr[1]);
    printf("\nFREE 1\n"); el_print_stats(); printf("\n");

    el_free(ptr[0]);
    printf("\nFREE 0\n"); el_print_stats(); printf("\n");

    el_free(ptr[3]);
    printf("\nFREE 3\n"); el_print_stats(); printf("\n");

    el_free(ptr[2]);
    printf("\nFREE 2\n"); el_print_stats(); printf("\n");
  } // ENDTEST

  IF_TEST("Four Allocs Free Ver3" ) {
    // Another variation of allocating 4 times then free()'ing blocks
    // in a different order to examine if mergin works properly.

    void *ptr[16] = {};
    int len = 0;

    ptr[len++] = el_malloc(128);
    printf("\nMALLOC 0\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(200);
    printf("\nMALLOC 1\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(64);
    printf("\nMALLOC 2\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(312);
    printf("\nMALLOC 3\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    el_free(ptr[3]);
    printf("\nFREE 3\n"); el_print_stats(); printf("\n");

    el_free(ptr[0]);
    printf("\nFREE 0\n"); el_print_stats(); printf("\n");

    el_free(ptr[2]);
    printf("\nFREE 2\n"); el_print_stats(); printf("\n");

    el_free(ptr[1]);
    printf("\nFREE 1\n"); el_print_stats(); printf("\n");
  } // ENDTEST

  IF_TEST("Alloc Fails" ) {
    // Allocates 4 times which each succeed. Then attempts to allocate
    // again for a large block which cannot be allocated. el_malloc()
    // should return NULL in this case and the heap remains unchanged.

    void *ptr[16] = {};
    int len = 0;

    ptr[len++] = el_malloc(1000);
    ptr[len++] = el_malloc(1000);
    ptr[len++] = el_malloc(1000);
    ptr[len++] = el_malloc(800);
    printf("\nMALLOC 4\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(512);
    printf("\nMALLOC 5\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);
    printf("should be (nil)\n");
  } // ENDTEST


  IF_TEST("No Split" ) {
    // Checks that allocation of blocks close to the size available in
    // the block does not split the block.
    void *ptr[16] = {}; int len = 0;

    ptr[len++] = el_malloc(2008); // split
    ptr[len++] = el_malloc(2008); // NOT split
    printf("POINTERS\n"); print_ptrs(ptr, len);
    printf("\nUSED BLOCKS 1\n"); el_print_stats(); printf("\n");

    el_free(ptr[0]); ptr[0] = NULL;
    ptr[len++] = el_malloc(1008); // split, 1000 left
    ptr[len++] = el_malloc(980);  // NOT split
    printf("POINTERS\n"); print_ptrs(ptr, len);
    printf("\nUSED BLOCKS 2\n"); el_print_stats(); printf("\n");
  } // ENDTEST

  IF_TEST("Free Fails" ) {
    // Checks that basic error checking is done during el_free():
    // blocks that have already been free()'d report an error and
    // blocks that don't seem to have the right properties are also
    // not free()'d
    void *ptr[16] = {}; int len = 0;
    ptr[len++] = el_malloc(100); 
    ptr[len++] = el_malloc(200); 
    ptr[len++] = el_malloc(300); 
    ptr[len++] = el_malloc(400); 
    printf("POINTERS\n"); print_ptrs(ptr, len);
    printf("\n==USED BEFORE FREES==\n"); el_print_stats(); printf("\n");
    el_free(ptr[1]); 
    printf("\n==AFTER 1st FREE WHICH IS FINE==\n"); el_print_stats(); printf("\n");
    printf("\n==ATTEMPTING 2nd FREE WHICH IS AN ERROR==\n"); 
    el_free(ptr[1]); ptr[1] = NULL;
    printf("\n==AFTER 3rd FREE WHICH IS FINE==\n"); el_print_stats(); printf("\n");
    el_free(ptr[3]); 
    printf("\n==ATTEMPTING 4th FREE WHICH IS AN ERROR==\n"); 
    el_free(ptr[3]); ptr[3] = NULL;
    el_blockhead_t dummies[2] = {
      {.size = 0, .state ='\0', .next = NULL, .prev = NULL},
      {},
    };
    printf("\n==ATTEMPTING 5th FREE ON DUMMY WHICH IS AN ERROR==\n"); 
    el_free(&dummies[1]); 
    printf("\n==ATTEMPTING 6th FREE ON NULL WHICH IS AN ERROR==\n"); 
    el_free(NULL); 
  } // ENDTEST


  IF_TEST("Old EL Demo" ) {
    // Recreates the behavior of the el_demo.c program and checks that
    // allocations/mergins are occurring correctly.

    printf("INITIAL\n"); el_print_stats(); printf("\n");

    void *p1 = el_malloc(128);
    void *p2 = el_malloc(48);
    void *p3 = el_malloc(156);
    printf("MALLOC 3\n"); el_print_stats(); printf("\n");

    printf("POINTERS\n");
    print_ptr("p3",p3);
    print_ptr("p2",p2);
    print_ptr("p1",p1);
    printf("\n");

    void *p4 = el_malloc(22);
    void *p5 = el_malloc(64);
    printf("MALLOC 5\n"); el_print_stats(); printf("\n");

    printf("POINTERS\n");
    print_ptr("p5",p5);
    print_ptr("p4",p4);
    print_ptr("p3",p3);
    print_ptr("p2",p2);
    print_ptr("p1",p1);
    printf("\n");

    el_free(p1);
    printf("FREE 1\n"); el_print_stats(); printf("\n");

    el_free(p3);
    printf("FREE 3\n"); el_print_stats(); printf("\n");

    p3 = el_malloc(32);
    p1 = el_malloc(200);
  
    printf("RE-ALLOC 3,1\n"); el_print_stats(); printf("\n");

    printf("POINTERS\n");
    print_ptr("p1",p1);
    print_ptr("p3",p3);
    print_ptr("p5",p5);
    print_ptr("p4",p4);
    print_ptr("p2",p2);
    printf("\n");

    el_free(p1);

    printf("FREE'D 1\n"); el_print_stats(); printf("\n");

    el_free(p2);

    printf("FREE'D 2\n"); el_print_stats(); printf("\n");

    el_free(p3);
    el_free(p4);
    el_free(p5);

    printf("FREE'D 3,4,5\n"); el_print_stats(); printf("\n");
  } // ENDTEST


  IF_TEST("Stress 1" ) {
    // Stress testing with many malloc()'s / free()'s. More thoroughly
    // tests combinations of calls and sequences. Implementations that
    // may have appeared correct in simple tests may buckle under such
    // added stress.

    void *ptr[16] = {};
    int len = 0;

    ptr[len++] = el_malloc(128);
    ptr[len++] = el_malloc(256);
    ptr[len++] = el_malloc(64);
    ptr[len++] = el_malloc(200);
    printf("MALLOC 1-4\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    el_free(ptr[2]);    ptr[2] = NULL;
    printf("\nFREE 2\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(64);
    printf("\nMALLOC 5\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    el_free(ptr[1]);    ptr[1] = NULL;
    printf("\nFREE 1\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(50);
    ptr[len++] = el_malloc(50);
    printf("\nMALLOC 6-7\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(100);
    printf("\nMALLOC 8\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    el_free(ptr[5]);   ptr[5] = NULL;
    el_free(ptr[0]);   ptr[0] = NULL;
    el_free(ptr[6]);   ptr[6] = NULL;
    printf("\nFREE 5,0,6\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);

    ptr[len++] = el_malloc(200);
    ptr[len++] = el_malloc(512);
    ptr[len++] = el_malloc(16);
    ptr[len++] = el_malloc(32);
    printf("\nMALLOC 9,10,11\n"); el_print_stats(); printf("\n");
    printf("POINTERS\n"); print_ptrs(ptr, len);
  } // ENDTEST

  IF_TEST("Append Pages 1" ) {
    // Tests if el_append_pages_to_heap(1) works correctly. The heap
    // has 1 used block after malloc()'ing and no extra
    // space. Expanding by 1 page gives a new available block and a
    // larger heap size.
    void *ptr = el_malloc(EL_PAGE_BYTES-EL_BLOCK_OVERHEAD);
    printf("HEAP AFTER 1 MALLOC()\n");
    el_print_stats(); printf("\n");

    int ret = el_append_pages_to_heap(1);
    printf("EXPANDED HEAP, ret: %d\n",ret);
    el_print_stats(); printf("\n");

    el_free(ptr);
    printf("EXPANDED HEAP AFTER FREE\n");
    el_print_stats(); printf("\n");
  } // ENDTEST

  IF_TEST("Append Pages 2" ) {
    // Tests if el_append_pages_to_heap(2) works correctly. Since no
    // space is allocated in the heap initially, the block for the new
    // pages should be merged with the existing block.
    printf("INITIAL HEAP\n");
    el_print_stats(); printf("\n");

    int ret = el_append_pages_to_heap(2);
    printf("EXPANDED HEAP, ret: %d\n",ret);
    el_print_stats(); printf("\n");
  } // ENDTEST

  IF_TEST("Append Pages 3" ) {
    // Tests if heap expansion allows a large-ish malloc that fails
    // initially to succeed after expansion.
    void *p1, *p2;
    p1 = el_malloc(EL_PAGE_BYTES/2); // succeeds
    p2 = el_malloc(EL_PAGE_BYTES);   // fails
    printf("p1: %p\np2: %p\n",p1,p2);
    printf("HEAP AFTER 2 MALLOCs()\n");
    el_print_stats(); printf("\n");

    int ret = el_append_pages_to_heap(3);
    p2 = el_malloc(EL_PAGE_BYTES);   // succeds now
    printf("p1: %p\np2: %p\n",p1,p2);
    printf("EXPANDED HEAP, ret: %d\n",ret);
    el_print_stats(); printf("\n");

    el_free(p1);
    printf("EXPANDED HEAP AFTER 1st FREE\n");
    el_print_stats(); printf("\n");
    el_free(p2);
    printf("EXPANDED HEAP AFTER 2nd FREE\n");
    el_print_stats(); printf("\n");
  } // ENDTEST

  IF_TEST("Append Pages Fails" ) {
    // Tests if el_append_pages_to_heap(1) works correctly. The heap
    // has 1 used block after malloc()'ing and no extra
    // space. Expanding by 1 page gives a new available block and a
    // larger heap size.
    int ret;
    ret = el_append_pages_to_heap(-1);
    printf("EXPANSION FAILS 1, ret: %d\n",ret);
    el_print_stats(); printf("\n");

    ret = el_append_pages_to_heap(1<<30);
    printf("EXPANSION FAILS 2, ret: %d\n",ret);
    el_print_stats(); printf("\n");

    void *conflict =   // intentionally block heap expansion
      mmap(el_ctl->heap_end, 4096,
           PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS,
           -1, 0);
    ret = el_append_pages_to_heap(3);
    printf("conflict: %p\n",conflict);
    printf("EXPANSION FAILS 3, ret: %d\n",ret);
    el_print_stats(); printf("\n");
  } // ENDTEST


  ////////////////////////////////////////////////////////////////////////////////
  // END OF ALL TESTS

  if(nrun == 0){                // check that at least one test was run
    printf("No test named '%s' found\n",test_name);
    return 1;
  }

  el_cleanup();
  return 0;
}
