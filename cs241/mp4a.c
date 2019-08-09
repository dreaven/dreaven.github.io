/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"


/**
  Initializes the priqueue_t data structure.
  
  Assumtions
    - You may assume this function will only be called once per instance of priqueue_t
    - You may assume this function will be the first function called using an instance of priqueue_t.
  @param q a pointer to an instance of the priqueue_t data structure
  @param comparer a function pointer that compares two elements.
  See also @ref comparer-page
 */
void priqueue_init(priqueue_t *q, int(*comparer)(const void *, const void *))
{

  q->size = 0;
  q->header =  (node *) malloc(sizeof(node));
  q->header-> next = NULL;
  q->header->data = NULL;
  q->cmp = comparer;

  return;
}


/**
  Inserts the specified element into this priority queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr a pointer to the data to be inserted into the priority queue
  @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr was stored at the front of the priority queue.
 */
int priqueue_offer(priqueue_t *q, void *ptr)
{
  int ind = 0;
  node * t ;
  node * first, *second;
  if (q->size==0)
  {
    t = (node *) malloc(sizeof(node));
    q->header-> next = t;
    t->next = NULL;
    t->data = ptr;
    q->size = 1;
    return 0;
  }
  else
  {
    
    first = q->header;
    second = first->next;
    while(1)
    {
      if(second==NULL || ((q->cmp)(second->data, ptr)) > 0 )
        break;
      else
      {
        first = second;
        second = second->next;
        ind++;
      }
    }

    if(second == NULL)
    {
      t = (node *) malloc(sizeof(node));
      first-> next = t;
      t->next = NULL;
      t->data = ptr;
      q->size += 1;
      return q->size-1;

    }
    else
    {
      t = (node *) malloc(sizeof(node));
      first->next = t;
      t->next = second;
      t->data = ptr;
      q->size += 1;
      return ind;

    }


  }

}


/**
  Retrieves, but does not remove, the head of this queue, returning NULL if
  this queue is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the head of the queue
  @return NULL if the queue is empty
 */
void *priqueue_peek(priqueue_t *q)
{
  if (q->size==0)
    return NULL;
  else
    return q->header->next->data;
}


/**
  Retrieves and removes the head of this queue, or NULL if this queue
  is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the head of this queue
  @return NULL if this queue is empty
 */
void *priqueue_poll(priqueue_t *q)
{
  void *ptr;
  if(q->size==0)
	   return NULL;
  node * t = q->header->next;
  q->header->next = t->next;
  ptr = t->data;
  free(t);
  q->size -=1;
  return ptr;
}


/**
  Returns the element at the specified position in this list, or NULL if
  the queue does not contain an index'th element.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of retrieved element
  @return the index'th element in the queue
  @return NULL if the queue does not contain the index'th element
 */
void *priqueue_at(priqueue_t *q, int index)
{
  int i=0;
  if(index> q->size-1)
	 return NULL;

  node *t = q->header->next;
  for(i=0;i<index;i++)
    t=t->next;

  return t->data;
}


/**
  Removes all instances of ptr from the queue. 
  
  This function should not use the comparer function, but check if the data contained in each element of the queue is equal (==) to ptr.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr address of element to be removed
  @return the number of entries removed
 */
int priqueue_remove(priqueue_t *q, void *ptr)
{
  if (q->size==0)
	   return 0;
  int i = 0;
  node * first= q->header;
  node *second = first->next;
  while(second!=NULL)
  {
    if(second->data==ptr)
    {
      i++; first->next = second->next;
      free(second); second = first->next; q->size-=1;
    }
    else
    {
      while(second!=NULL && second->data!=ptr)
        {first = second;second = second->next;}
    }
  }

  return i;
}


/**
  Removes the specified index from the queue, moving later elements up
  a spot in the queue to fill the gap.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of element to be removed
  @return the element removed from the queue
  @return NULL if the specified index does not exist
 */
void *priqueue_remove_at(priqueue_t *q, int index)
{
  if(index > q->size-1)
	 return NULL;
  node *t = q->header, *b;
  void * tmp;
  int i=0;
  for(i=0;i<index;i++)
    t = t->next;
  b = t->next;
  t->next = b->next;
  tmp = b->data;
  free(b);
  q->size -=1;
  return tmp;


}


/**
  Returns the number of elements in the queue.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the number of elements in the queue
 */
int priqueue_size(priqueue_t *q)
{
	return q->size;
}


/**
  Destroys and frees all the memory associated with q.
  
  @param q a pointer to an instance of the priqueue_t data structure
 */
void priqueue_destroy(priqueue_t *q)
{
  node * t, *b;

  t = q->header;
  b= t->next;
  while(b!=NULL)
    {free(t); t=b;b=b->next;}
  free(t);
  q->header = NULL;
  q->size = 0;

  return;
}
