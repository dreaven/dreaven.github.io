/** @file alloc.c */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>



static size_t flag_init = 0;

typedef struct Linklist
{
	size_t blocksize;
	char used;   
	struct Linklist * next;
	struct Linklist * prev;
} Linklist;

Linklist * header = NULL;

/* #define METASIZE sizeof(Linklist) */
static size_t METASIZE = sizeof(Linklist) + 2;

/**
 * Allocate space for array in memory
 * 
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 * 
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */

/*
void printfheap(void)
{
	Linklist *f1;

	f1 = header;

	printf("printf heap function is called\n");

	while(f1!=NULL)
	{
		printf("address: %d, blocksize: %d, used: %d, next address: %d, prev address: %d \n", f1, f1->blocksize, f1->used, f1->next, f1->prev);
		f1 = f1->next;
	}

	return;

}
*/


void *calloc(size_t num, size_t size)
{
	/* Note: This function is complete. You do not need to modify it. */
	void *ptr = malloc(num * size);
	
	if (ptr)
		memset(ptr, 0x00, num * size);

	return ptr;
}


/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size)
{
	void * t, *temp;
	Linklist * inter1, *headernn, *first, *second, *l1;


	if (flag_init == 0)
	{

		flag_init = 1;
		t = sbrk(size + METASIZE);
		if (t==NULL)
			return t;

		header = (Linklist *) t;
		header->used = 1;
		header->prev = NULL;
		header->next = NULL;
		header->blocksize = size;
		

		return ((void *)header + METASIZE );

	}

	else
	{
		/* first seach whether there exists a free block large enough*/
		

	
		if (header->used==0 && header->blocksize>= size)
		{
			if (header->blocksize <= size + METASIZE  )   /* directly allocate the memory */
			{
				header->used = 1;
				return ((void *)header + METASIZE );

			}
			else /*split into two parts*/
			{
				inter1 = (void *)header + METASIZE + size;

				header->used = 1;
				headernn = header->next;
				header->next =  inter1;
				inter1->next = headernn;
				inter1->prev = header;
				inter1->used = 0;
				inter1->blocksize = header->blocksize - size - METASIZE;
				header->blocksize = size;
				if (headernn!=NULL)
					headernn->prev = inter1;

				return ((void *)header + METASIZE );

			}
		}

		else  /*start from header, search through the whole list, and find the first free block*/
		{
			first = header;
			second = first->next;
			while(second!=NULL && !(second->used==0 && second->blocksize>= size))
			{
				second = second->next;
				first = first->next;
			}

			if (second==NULL)  /* there is no free block available */
			{
				if (first->used == 0)
				{
					temp = sbrk(size - first->blocksize);
					if (temp==NULL)
						return NULL;
					first->used = 1;
					first->blocksize = size;
					return ((void *)first + METASIZE );
				}
				else
				{
					temp = sbrk(size + METASIZE);
					if (temp==NULL)
						return NULL;
					l1 = (Linklist *) temp;

					l1->used = 1;
					l1->blocksize = size;
					l1->prev = first;
					l1->next = NULL;

					if (first!=NULL)
						first->next = l1;

					return ((void *)l1 + METASIZE );
				}

			}
			else /* there is   free block available */
			{

				if (second->blocksize <= size + METASIZE )   /* directly allocate the memory */
				{
					second->used = 1;
					return ((void *)second + METASIZE );

				}
				else /*split into two parts*/
				{
					inter1 = (void *)second + METASIZE + size;
					second->used = 1;
					headernn = second->next;
					header->next =  inter1;
					inter1->next = headernn;
					inter1->prev = header;
					inter1->used = 0;
					if (headernn!=NULL)
						headernn->prev = inter1;
					inter1->blocksize = second->blocksize - size - METASIZE;
					second->blocksize = size;

					return ((void *)second + METASIZE );

				}

			}

		}

	}

	
}


/**
 * Deallocate space in memory
 * 
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr)
{



/* "If a null pointer is passed as argument, no action occurs." */

	if (!ptr)
		return;
	ptr = ptr - METASIZE;
	Linklist * t = ptr;
	t->used = 0;

	/* merge t and t->next*/
	if (t->next!=NULL)
	{
		if(t->next->used==0)
		{
			t->blocksize += METASIZE + t->next->blocksize;
			t->next = t->next->next;
			if (t->next !=NULL)
				t->next->prev = t; 
		}
	}


	/* merge t and its previous block*/

	if(t->prev!=NULL)
	{

		if (t->prev->used == 0)
		{
			t->prev->blocksize += METASIZE + t->blocksize;
			t->prev->next = t->next;
			if (t->next!=NULL)
				t->next->prev = t->prev;
		}

	}


	


	return;
}


/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *    
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size)
{

	/*printf("Before realloc is called\n");
		printfheap(); */

	Linklist * t, *temp, *t2, *t3;
	void * newm;
	size_t original_size;
	size_t i;

	 // "In case that ptr is NULL, the function behaves exactly as malloc()"
	if (!ptr)
		return malloc(size);

	 // "In case that the size is 0, the memory previously allocated in ptr
	 //  is deallocated as if a call to free() was made, and a NULL pointer
	 //  is returned."
	if (!size)
	{
		free(ptr);
		return NULL;
	}

	t = (Linklist *) (ptr - METASIZE);
	original_size = t->blocksize;

	if (size <= original_size)
	{
		if (size + METASIZE <=original_size)  // split to two parts, else do nothing
		{
			t->blocksize = size;
			t2 = ((void *) t) + METASIZE + size;
			t2->blocksize = original_size +   - size - METASIZE;
			t2->prev = t;
			t2->used = 0;
			t2->next = t->next;
			if (t->next !=NULL)
				t->next->prev = t2;
			t->next = t2;

			return ptr;
			
		}
		else
		{
			return ptr;
		}
	} 

	else
	{
		if(t->next!=NULL)
		{
			if(t->next->used==0 && t->next->blocksize + METASIZE + t->blocksize >= size)
			{
				size_t wholesize = t->next->blocksize + METASIZE + t->blocksize;

				if (size + METASIZE >= wholesize)  /* jsut merge two blocks*/
				{
					
					t->blocksize = wholesize;
					
					temp = t->next->next;
					t->next= temp;
							
					if (temp!=NULL)
						temp->prev= t;

					return ptr;
				}
				else   // split it by two
				{
					t3 = ((void *)t) + METASIZE + size;
					t3->used= 0;
					t3->next = t->next->next;
					if (t->next->next!=NULL) 
						t->next->next->prev = t3;
					t3->blocksize = wholesize - size - METASIZE;
					t3->prev = t;
						

					return ptr;

				}
			}



		}

		/* allocate a new memory*/

		
		
		newm = malloc(size);
		if (newm==NULL)
			return NULL;
			
		for(i=0;i<t->blocksize;i++)
			*((char*)newm+i) = *((char*)ptr+i);

		free(ptr);
		return newm;
		
	}


}


