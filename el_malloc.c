// el_malloc.c: implementation of explicit list malloc functions.

#include "el_malloc.h"

////////////////////////////////////////////////////////////////////////////////
// Global control functions

// Global control variable for the allocator. Must be initialized in
// el_init().
el_ctl_t *el_ctl = NULL;

// Create an initial block of memory for the heap using
// mmap(). Initialize the el_ctl data structure to point at this
// block. The initializ size/position of the heap for the memory map
// are given in the argument symbol and EL_HEAP_START_ADDRESS.
// Initialize the lists in el_ctl to contain a single large block of
// available memory and no used blocks of memory.
int el_init(uint64_t initial_heap_size){
  el_ctl =
    mmap(EL_CTL_START_ADDRESS,
         EL_PAGE_BYTES,
         PROT_READ | PROT_WRITE,
         MAP_PRIVATE | MAP_ANONYMOUS,
         -1, 0);
  assert(el_ctl == EL_CTL_START_ADDRESS);

  void *heap = 
    mmap(EL_HEAP_START_ADDRESS,
         initial_heap_size,
         PROT_READ | PROT_WRITE,
         MAP_PRIVATE | MAP_ANONYMOUS,
         -1, 0);
  assert(heap == EL_HEAP_START_ADDRESS);

  el_ctl->heap_bytes = initial_heap_size;    // make the heap as big as possible to begin with
  el_ctl->heap_start = heap;                 // set addresses of start and end of heap
  el_ctl->heap_end   = PTR_PLUS_BYTES(heap,el_ctl->heap_bytes);

  if(el_ctl->heap_bytes < EL_BLOCK_OVERHEAD){
    fprintf(stderr,"el_init: heap size %ld to small for a block overhead %ld\n",
            el_ctl->heap_bytes,EL_BLOCK_OVERHEAD);
    return 1;
  }
 
  el_init_blocklist(&el_ctl->avail_actual);
  el_init_blocklist(&el_ctl->used_actual);
  el_ctl->avail = &el_ctl->avail_actual;
  el_ctl->used  = &el_ctl->used_actual;

  // establish the first available block by filling in size in
  // block/foot and null links in head
  size_t size = el_ctl->heap_bytes - EL_BLOCK_OVERHEAD;
  el_blockhead_t *ablock = el_ctl->heap_start;
  ablock->size = size;
  ablock->state = EL_AVAILABLE;
  el_blockfoot_t *afoot = el_get_footer(ablock);
  afoot->size = size;

  // Add initial block to availble list; avoid use of list add
  // functions in case those are buggy which will screw up the heap
  // initialization
  ablock->prev = el_ctl->avail->beg;
  ablock->next = el_ctl->avail->beg->next;
  ablock->prev->next = ablock;
  ablock->next->prev = ablock;
  el_ctl->avail->length++;
  el_ctl->avail->bytes += (ablock->size + EL_BLOCK_OVERHEAD);

  return 0;
}

// Clean up the heap area associated with the system which unmaps all
// pages associated with the heap.
void el_cleanup(){
  munmap(el_ctl->heap_start, el_ctl->heap_bytes);
  munmap(el_ctl, EL_PAGE_BYTES);
}

////////////////////////////////////////////////////////////////////////////////
// Pointer arithmetic functions to access adjacent headers/footers

// Compute the address of the foot for the given head which is at a
// higher address than the head.
el_blockfoot_t *el_get_footer(el_blockhead_t *head){
  size_t size = head->size;
  el_blockfoot_t *foot = PTR_PLUS_BYTES(head, sizeof(el_blockhead_t) + size);
  return foot;
}

// REQUIRED
// Compute the address of the head for the given foot which is at a
// lower address than the foot.
el_blockhead_t *el_get_header(el_blockfoot_t *foot){
  // retrieve the size of the preceeding block of memory
  size_t size = foot->size;
  
  // initialize a pointer to head where the PTR_MINUS_BYTES macro is used
  // and the the total memory between the foot and the head is 
  // subtracted from address of the foot
  el_blockhead_t *head = PTR_MINUS_BYTES(foot, size + sizeof(el_blockhead_t));
  
  // return the ptr to head
  return head;
}

// Return a pointer to the block that is one block higher in memory
// from the given block.  This should be the size of the block plus
// the EL_BLOCK_OVERHEAD which is the space occupied by the header and
// footer. Returns NULL if the block above would be off the heap.
// DOES NOT follow next pointer, looks in adjacent memory.
el_blockhead_t *el_block_above(el_blockhead_t *block){
  el_blockhead_t *higher =
    PTR_PLUS_BYTES(block, block->size + EL_BLOCK_OVERHEAD);
  if((void *) higher >= (void*) el_ctl->heap_end){
    return NULL;
  }
  else{
    return higher;
  }
}

// REQUIRED
// Return a pointer to the block that is one block lower in memory
// from the given block.  Uses the size of the preceding block found
// in its foot. DOES NOT follow block->next pointer, looks in adjacent
// memory. Returns NULL if the block below would be outside the heap.
// 
// WARNING: This function must perform slightly different arithmetic
// than el_block_above(). Take care when implementing it.
el_blockhead_t *el_block_below(el_blockhead_t *block){
  
  // check for heap proximity using the macros and return NULL if proximity is
  // too close to the bottom of the heap
  if ((void *)block <= PTR_PLUS_BYTES(el_ctl->heap_start, sizeof(el_blockfoot_t))) {
    return NULL;
  }

  // retrieve the footer and store it in a pointer
  el_blockfoot_t *foot = PTR_MINUS_BYTES(block, sizeof(el_blockfoot_t));
  size_t size = foot->size;

  // compute the blovk below by using the size or the foot and the given block
  el_blockhead_t *block_below = PTR_MINUS_BYTES(block, size + sizeof(el_blockhead_t) + sizeof(el_blockfoot_t));

  // return the computer lower block
  return block_below;
}

////////////////////////////////////////////////////////////////////////////////
// Block list operations

// Print an entire blocklist. The format appears as follows.
//
// {length:   2  bytes:  3400}
//   [  0] head @ 0x600000000000 {state: a  size:   128}
//   [  1] head @ 0x600000000360 {state: a  size:  3192}
//
// Note that the '@' column uses the actual address of items which
// relies on a consistent mmap() starting point for the heap.
void el_print_blocklist(el_blocklist_t *list){
  printf("{length: %3lu  bytes: %5lu}\n", list->length,list->bytes);
  el_blockhead_t *block = list->beg;
  for(int i=0; i<list->length; i++){
    printf("  ");
    block = block->next;
    printf("[%3d] head @ %p ", i, block);
    printf("{state: %c  size: %5lu}\n", block->state,block->size);
  }
}


// Print a single block during a sequential walk through the heap
void el_print_block(el_blockhead_t *block){
  el_blockfoot_t *foot = el_get_footer(block);
  printf("%p\n", block);
  printf("  state:      %c\n", block->state);
  printf("  size:       %lu (total: 0x%lx)\n", block->size, block->size+EL_BLOCK_OVERHEAD);
  printf("  prev:       %p\n", block->prev);
  printf("  next:       %p\n", block->next);
  printf("  user:       %p\n", PTR_PLUS_BYTES(block,sizeof(el_blockhead_t)));
  printf("  foot:       %p\n", foot);
  printf("  foot->size: %lu\n", foot->size);
}

// Print all blocks in the heap in the order that they appear from
// lowest addrses to highest address
void el_print_heap_blocks(){
  int i = 0;
  el_blockhead_t *cur = el_ctl->heap_start;
  while(cur != NULL){
    printf("[%3d] @ ",i);
    el_print_block(cur);
    cur = el_block_above(cur);
    i++;
  }
}  


// Print out stats on the heap for use in debugging. Shows the
// available and used list along with a linear walk through the heap
// blocks.
void el_print_stats(){
  printf("HEAP STATS (overhead per node: %lu)\n",EL_BLOCK_OVERHEAD);
  printf("heap_start:  %p\n",el_ctl->heap_start); 
  printf("heap_end:    %p\n",el_ctl->heap_end); 
  printf("total_bytes: %lu\n",el_ctl->heap_bytes);
  printf("AVAILABLE LIST: ");
  el_print_blocklist(el_ctl->avail);
  printf("USED LIST: ");
  el_print_blocklist(el_ctl->used);
  printf("HEAP BLOCKS:\n");
  el_print_heap_blocks();
}

// Initialize the specified list to be empty. Sets the beg/end
// pointers to the actual space and initializes those data to be the
// ends of the list.  Initializes length and size to 0.
void el_init_blocklist(el_blocklist_t *list){
  //list->beg        = &(list->beg_actual); 
  list->beg->state = EL_BEGIN_BLOCK;
  list->beg->size  = EL_UNINITIALIZED;
  list->end        = &(list->end_actual); 
  list->end->state = EL_END_BLOCK;
  list->end->size  = EL_UNINITIALIZED;
  list->beg->next  = list->end;
  list->beg->prev  = NULL;
  list->end->next  = NULL;
  list->end->prev  = list->beg;
  list->length     = 0;
  list->bytes      = 0;
}  

// REQUIRED
// Add to the front of list; links for block are adjusted as are links
// within list.  Length is incremented and the bytes for the list are
// updated to include the new block's size and its overhead.
void el_add_block_front(el_blocklist_t *list, el_blockhead_t *block){
  // insert param block right after beginning dummy node
  // since operates like linked list
  block->prev = list->beg;
  block->next = list->beg->next;

  // update the previous first block's prev pointer
  list->beg->next->prev = block;

  // link dummy node to the new block
  list->beg->next = block;

  // add one to the length to reflect the added block
  list->length += 1;

  // update the list bytes to reflect the added block based
  // on it's size
  list->bytes += block->size + EL_BLOCK_OVERHEAD;
}

// REQUIRED
// Unlink block from the list it is in which should be the list
// parameter.  Updates the length and bytes for that list including
// the EL_BLOCK_OVERHEAD bytes associated with header/footer.
void el_remove_block(el_blocklist_t *list, el_blockhead_t *block){
  // change references for the prev and next pointers to remove the specified block
  block->prev->next = block->next;
  block->next->prev = block->prev;

  // update the length of the specified length to specify the removed node
  // and update the bytes associated with EL_BLOCK_OVERHEAD
  list->length -= 1;
  list->bytes -= block->size + EL_BLOCK_OVERHEAD;

}

////////////////////////////////////////////////////////////////////////////////
// Allocation-related functions

// REQUIRED
// Find the first block in the available list with block size of at
// least `size`.  Returns a pointer to the found block or NULL if no
// block of sufficient size is available.
el_blockhead_t *el_find_first_avail(size_t size){
  // go to the next block of the beginning pointer to get
  // to the actual memory block
  el_blockhead_t *curr = el_ctl->avail->beg->next;

  // while curr is not the pointer to avail->end
  while (curr != el_ctl->avail->end) {
    // check if the size of the curr block is sufficient
    if (curr->size >= size) {
      return curr;
    }
    // or advance to the next block
    curr = curr->next;
  }

  // there was no block greater than or equal to
  // the specified size
  return NULL;
}

// REQUIRED
// Set the pointed to block to the given size and add a footer to
// it. Creates another block above it by creating a new header and
// assigning it the remaining space. Ensures that the new block has a
// footer with the correct size. Returns a pointer to the newly
// created block while the parameter block has its size altered to
// parameter size. Does not do any linking of blocks nor changes of
// list membership: this is done elsewhere.  If the parameter block
// does not have sufficient size for a split (at least new_size +
// EL_BLOCK_OVERHEAD for the new header/footer) makes no changes tot
// the block and returns NULL indicating no new block was created.
el_blockhead_t *el_split_block(el_blockhead_t *block, size_t new_size){
  // determine if the size given allows for splitting
  if (block->size < new_size + EL_BLOCK_OVERHEAD) {
    return NULL;
  }

  // calculate the size of the split block to the right
  size_t split_size = block->size - new_size - EL_BLOCK_OVERHEAD;

  // update the size of the given block so the footer may be placed
  // using those metrics
  block->size = new_size;

  // add the footer to the size cutoff of the given block to
  // isolate it from the split
  el_blockfoot_t *foot = el_get_footer(block);
  foot->size = new_size;

  // initialize the new header using pointer arthimetic
  el_blockhead_t *split_block = (el_blockhead_t *) PTR_PLUS_BYTES(foot, sizeof(el_blockfoot_t));
  
  // initialize the fields of the new block of memory
  split_block->size = split_size;
  split_block->state = EL_AVAILABLE;  
  split_block->prev = block;  
  split_block->next = block->next;

  // change the next reference to the new adjacent split block in memory
  block->next = split_block;

  // intiialize the footer of the new split block of memory
  // and initialize it's size using the new split size
  el_blockfoot_t *split_foot = el_get_footer(split_block);
  split_foot->size = split_size;

  return split_block;
}

// REQUIRED
// Return pointer to a block of memory with at least the given size
// for use by the user.  The pointer returned is to the usable space,
// not the block header. Makes use of find_first_avail() to find a
// suitable block and el_split_block() to split it.  Returns NULL if
// no space is available.
void *el_malloc(size_t nbytes){
  // find the first available block based on the provided size requirements
  el_blockhead_t *block = el_find_first_avail(nbytes);
  if (block == NULL) return NULL;

  // make a call to el_remove_block()
  el_remove_block(el_ctl->avail, block);

  // the block is split in order to use only the amount of memory
  // that is specified by the user 
  el_blockhead_t *split = el_split_block(block, nbytes);
  if (split != NULL) {
    el_add_block_front(el_ctl->avail, split);
  }
  
  // if the block was split then reassign the footer and update the size
  el_blockfoot_t *foot = el_get_footer(block);
  foot->size = block->size;

  // change the state to reflect it's used status by the user
  block->state = EL_USED;

  // the specified block is added to the used list to 
  // reflect it's new state
  el_add_block_front(el_ctl->used, block);

  // return the pointer to the updated memory that is usable
  return PTR_PLUS_BYTES(block, sizeof(el_blockhead_t));
}

////////////////////////////////////////////////////////////////////////////////
// De-allocation/free() related functions

// REQUIRED
// Attempt to merge the block lower with the next block in
// memory. Does nothing if lower is null or not EL_AVAILABLE and does
// nothing if the next higher block is null (because lower is the last
// block) or not EL_AVAILABLE.  Otherwise, locates the next block with
// el_block_above() and merges these two into a single block. Adjusts
// the fields of lower to incorporate the size of higher block and the
// reclaimed overhead. Adjusts footer of higher to indicate the two
// blocks are merged.  Removes both lower and higher from the
// available list and re-adds lower to the front of the available
// list.
void el_merge_block_with_above(el_blockhead_t *lower){
  // ensure that param is not NULL and that it's available to merge
  if (lower == NULL || lower->state != EL_AVAILABLE) {
    return;
  }

  // retrieve the upper block and repeat the checks for safety
  el_blockhead_t *upper = el_block_above(lower);
  if (upper == NULL || upper->state != EL_AVAILABLE) {
    return;
  }

  // make calls to remove the upper and lower blocks
  el_remove_block(el_ctl->avail, lower);
  el_remove_block(el_ctl->avail, upper);

  // merge their sizes being sure to utilize the overhead macro
  lower->size = lower->size + upper->size + EL_BLOCK_OVERHEAD;

  // update the footer in order to reflect the merge
  el_blockfoot_t *foot = el_get_footer(lower);
  
  // set the size to the size that was of the lower block
  foot->size = lower->size;

  // append the merged block to the front of the available list
  el_add_block_front(el_ctl->avail, lower);
}

// REQUIRED
// Free the block pointed to by the give ptr.  The area immediately
// preceding the pointer should contain an el_blockhead_t with information
// on the block size. Attempts to merge the free'd block with adjacent
// blocks using el_merge_block_with_above(). If called on a NULL
// pointer or the block is not in state EL_USED, prints the error
// 
//   ERROR: el_free() not called on an EL_USED block
// 
// and returns immediately without further action.
void el_free(void *ptr){
  // complete a NULL check and print the error if triggered before returning
  if (ptr == NULL) {
    printf("ERROR: el_free() not called on an EL_USED block\n");
    return;
  }

  // retrieve the block by referencing the ptr param and 
  // through pointer arithmetic
  el_blockhead_t *block = PTR_MINUS_BYTES(ptr, sizeof(el_blockhead_t));

  // ensure that the state is not used; if so, print the error 
  // before returning from the function
  if (block->state != EL_USED) {
    printf("ERROR: el_free() not called on an EL_USED block\n");
    return;
  }

  // set the state to available
  block->state = EL_AVAILABLE;

  // retrieve the footer through a call to el_get_footer
  // and then set the footer size to that of the block
  el_blockfoot_t *foot = el_get_footer(block);
  foot->size = block->size;

  // remove the block through a call to el_remove_block
  el_remove_block(el_ctl->used, block);

  // add the block to the front through a call to el_add_block_front
  el_add_block_front(el_ctl->avail, block);

  // attempt to merge through a call to el_merge_block_with_above
  el_merge_block_with_above(block);

  // attempt to merge block that is below with the block referenced via the ptr param
  el_blockhead_t *lower = el_block_below(block);
  if (lower != NULL && lower->state == EL_AVAILABLE) {
    el_merge_block_with_above(lower);
  }
}

////////////////////////////////////////////////////////////////////////////////
// HEAP EXPANSION FUNCTIONS

// REQUIRED
// Attempts to append pages of memory to the heap with mmap(). npages
// is how many pages are to be appended with total bytes to be
// appended as npages * EL_PAGE_BYTES. Calls mmap() with similar
// arguments to those used in el_init() however requests the address
// of the pages to be at heap_end so that the heap grows
// contiguously. If this fails, prints the message
// 
//  ERROR: Unable to mmap() additional 3 pages
// 
// and returns 1.  Otherwise, adjusts heap size and end for the
// expanded heap. Creates a new block for the freshly allocated pages
// that is added to the available list. Also attempts to merge this
// block with the block below it. Returns 0 on success.
// 
// NOTE ON mmap() USAGE: mmap() returns one of three things if a
// specific address is requested (its first argument):
// 
// 1. The address requested indicating the memory mapping succeeded
// 
// 2. A different address than the one requested if the requeste
//    address is in use
// 
// 3. The constant MAP_FAILED if the address mapping failed.
//
// #2 and #3 above should trigger this function to immediate print an
// #error message and return 1 as the heap cannot be made continuous
// #in those cases.
int el_append_pages_to_heap(int npages){
  // if the pages is invalid
  if (npages <= 0) {
    // print this error before returning 1
    printf("ERROR: Unable to mmap() additional %d pages\n", npages);
    return 1;
  }

  // calculate the number of bytes that will be added
  size_t bytes_add = npages * EL_PAGE_BYTES;

  // calculate the address of the new block through a call to mmap()
  void *new_block_addr = mmap(el_ctl->heap_end, bytes_add, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);

  // check if the map failed or if it's not able to add to the end of the heap
  // if so, print the error message before returning 1
  if (new_block_addr == MAP_FAILED || new_block_addr != el_ctl->heap_end) {
      printf("ERROR: Unable to mmap() additional %d pages\n", npages);
      return 1;
  }

  // create a new block to reflect the added "page" to the heap
  // and initialize it's fields to the default values
  el_blockhead_t *new_block = (el_blockhead_t *)new_block_addr;
  new_block->size = bytes_add - EL_BLOCK_OVERHEAD;
  new_block->state = EL_AVAILABLE;
  new_block->next = NULL;
  new_block->prev = NULL;

  // get the footer for the new block so the size can be set accordingly
  el_blockfoot_t *new_foot = el_get_footer(new_block);
  new_foot->size = new_block->size;

  // ensure to reflect the added heap space by adding the appropriate number
  // of bytes and by updating the new end of the heap
  el_ctl->heap_bytes += bytes_add;
  el_ctl->heap_end = PTR_PLUS_BYTES(new_block_addr, bytes_add);

  // add the new block to the front of the list
  el_add_block_front(el_ctl->avail, new_block);

  // update the block that lies below the new block and merge
  // if necessary
  el_blockhead_t *below = el_block_below(new_block);
  if (below && below->state == EL_AVAILABLE) {
      el_merge_block_with_above(below);
  }

  return 0;
}
