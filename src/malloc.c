#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)


static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *prev;  /* Pointer to the previous _block of allcated memory   */
   struct _block *next;  /* Pointer to the next _block of allcated memory   */
   bool   free;          /* Is this _block free?                     */
   char   padding[3];
};


struct _block *heapList = NULL; /* Free list to track the _blocks available */
struct _block *temp= NULL;
/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit 
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = heapList;

#if defined FIT && FIT == 0
   /* First fit */
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

#if defined BEST && BEST == 0
   //printf("TODO: Implement best fit here\n");
	struct _block *currBest= NULL;
	size_t bestSize = INT_MAX;
	while(curr)
	{
		*last=curr;
		if(curr->free && (curr->size -size) < bestSize)
		{
			currBest=curr;
			bestSize=(curr->size) -size;
		}
		curr=curr->next;
	}
	if(currBest)
	{
		curr=currBest;
	}
	
#endif

#if defined WORST && WORST == 0
    //printf("TODO: Implement worst fit here\n");
	
	struct _block *currWorst= NULL;
	size_t worstSize = 0;
	while(curr)
	{
		*last=curr;
		if(curr->free && (curr->size -size) > worstSize)
		{
			currWorst=curr;
			worstSize=(curr->size) -size;
		}
		curr=curr->next;
	}
	if(currWorst)
	{
		curr=currWorst;
	}
	
#endif

#if defined NEXT && NEXT == 0
   //printf("TODO: Implement next fit here\n");
   temp=curr;
   if(curr!=NULL)
   {
	  curr=temp->next;
   }
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
	  temp=curr;
   }
#endif

   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size) 
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1) 
   {
      return NULL;
   }

   /* Update heapList if not set */
   if (heapList == NULL) 
   {
      heapList = curr;
   }

   /* Attach new _block to prev _block */
   if (last) 
   {
      last->next = curr;
	  num_blocks++;
   }

   /* Update _block metadata */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;
   
   //num_blocks++;
   num_grows++;
   //max_heap+=size;
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size) 
{
   num_requested+=size;
   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0) 
   {
      return NULL;
   }

   /* Look for free _block */
   struct _block *last = heapList;
   struct _block *next = findFreeBlock(&last, size);

   /* TODO: Split free _block if possible */
   if(next!=NULL)
   {
		num_reuses++;
		if(((next->size)-size)>sizeof(struct _block))
		{
			int oldSize=next->size;
			struct _block *oldNext=next->next;
			uint8_t* ptr=(uint8_t*)next+ next->size;
			next->next=(struct _block*) ptr;
			next->size=size;
			next->next->size=oldSize- size;
			next->next->next=oldNext;
			next->next->free=1;
			num_splits++;
			num_blocks++;
		}
   }

   

   /* Could not find free _block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
	  max_heap+=size;
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }
   
   /* Mark _block as in use */
	next->free = false;
	num_mallocs++;
   /* Return data address associated with _block */
   return BLOCK_DATA(next);
}

//calloc
void *calloc(size_t nmemb, size_t size)
{
	void *ptr=malloc(nmemb*size);
	memset(ptr,0,nmemb);
	return ptr;
}
//realoc
void *realloc(void *ptr, size_t size)
{
	void *newPtr=malloc(size);
	memcpy(newPtr,ptr,size);
	return newPtr;
}
/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr) 
{
   if (ptr == NULL) 
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;
   num_frees++;
   /* TODO: Coalesce free _blocks if needed */
   struct _block *checkCoalesce=heapList;
   while(checkCoalesce)
   {
	   if(checkCoalesce->next!=NULL)
	   { 
		   if(checkCoalesce->free ==1  && checkCoalesce->next->free ==1)
		   {
				checkCoalesce->size=checkCoalesce->size + checkCoalesce->next->size + sizeof(struct _block);
				checkCoalesce->next=checkCoalesce->next->next;
				checkCoalesce->free=1;
				num_coalesces++;
				num_blocks--;
		   }
	   }
	   
	   checkCoalesce=checkCoalesce->next;
   }
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/