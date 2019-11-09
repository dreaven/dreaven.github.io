/** @file libwfg.c */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "libwfg.h"
#include "queue.h"
#include <pthread.h>

#define D(x) 
/**
 * Initializes the wait-for graph data structure.
 *
 * This function must always be called first, before any other libwfg
 * functions.
 *
 * @param wfg
 *   The wait-for graph data structure.
 */
void wfg_init(wfg_t *wfg)
{
	int i,j;
	for (i=0;i<MAXSIZE;i++)
		for (j=0;j<MAXSIZE;j++)
			wfg->data[i][j] = 0;

	wfg->size = 0;

	pthread_mutex_init(&wfg->m,NULL);

	return;

}


/**
 * Adds a "wait" edge to the wait-for graph.
 *
 * This function adds a directed edge from a thread to a resource in the
 * wait-for graph.  If the thread is already waiting on any resource
 * (including the resource) requested, this function should fail.
 *
 * @param wfg
 *   The wait-for graph data structure.
 * @param t_id
 *   A unique identifier to a thread.  A caller to this function may
 *   want to use the thread ID used by the system, by using the
 *   pthread_self() function.
 * @param r_id
 *   A unique identifier to a resource.
 *
 * @return
 *   If successful, this function should return 0.  Otherwise, this
 *   function returns a non-zero value.
 */
int wfg_add_wait_edge(wfg_t *wfg, unsigned long t_id, unsigned long r_id)
{
	unsigned long i,j;
	unsigned long t_index, r_index;
	unsigned long flag_t = 0, flag_r = 0;
	unsigned long flag_exist = 0;

	pthread_mutex_lock(&wfg->m);

	for(i=0;i<wfg->size;i++)
	{
		if(wfg->dataID[i] == t_id && wfg->dataType[i]==0)
			{	t_index = i; flag_t = 1; }
		if(wfg->dataID[i] == r_id && wfg->dataType[i]==1)
			{	r_index = i; flag_r = 1; }
	}

	if (flag_t == 0)
	{
		wfg->size++;
		wfg->dataID[wfg->size -1] = t_id;
		t_index = wfg->size -1;
		wfg->dataType[t_index] = 0;
		assert(wfg->size < MAXSIZE);
	}

	if (flag_r == 0)
	{
		wfg->size++;
		wfg->dataID[wfg->size-1] = r_id;
		r_index = wfg->size -1;
		wfg->dataType[r_index] = 1;
		assert(wfg->size < MAXSIZE);
	}	

	for(i=0;i<wfg->size;i++)
		if( wfg->dataType[i] == 1 && wfg->data[t_index][i] == 1)
			{ flag_exist = 1; break; }

	if (flag_exist == 1)
	{
		pthread_mutex_unlock(&wfg->m);
		D(printf("The thread %u is already waiting for the resource %u \n",t_id, wfg->dataID[i]);)
		return 1;
	}
	else
	{
		wfg->data[t_index][r_index] = 1;
		pthread_mutex_unlock(&wfg->m);
		return 0;
	}


}


/**
 * Replaces a "wait" edge with a "hold" edge on a wait-for graph.
 *
 * This function replaces an edge directed from a thread to a resource (a
 * "wait" edge) with an edge directed from the resource to the thread (a
 * "hold" edge).  If the thread does not contain a directed edge to the
 * resource when this function is called, this function should fail.
 * This function should also fail if the resource is already "held"
 * by another thread.
 *
 * @param wfg
 *   The wait-for graph data structure.
 * @param t_id
 *   A unique identifier to a thread.  A caller to this function may
 *   want to use the thread ID used by the system, by using the
 *   pthread_self() function.
 * @param r_id
 *   A unique identifier to a resource.
 *
 * @return
 *   If successful, this function should return 0.  Otherwise, this
 *   function returns a non-zero value.
 */
int wfg_add_hold_edge(wfg_t *wfg, unsigned long t_id, unsigned long r_id)
{

	unsigned long i,j;
	unsigned long t_index, r_index;
	unsigned long flag_t = 0, flag_r = 0;
	pthread_mutex_lock(&wfg->m);
	for(i=0;i<wfg->size;i++)
	{
		if(wfg->dataID[i] == t_id && wfg->dataType[i]==0)
			{	t_index = i; flag_t = 1; }
		if(wfg->dataID[i] == r_id && wfg->dataType[i]==1)
			{	r_index = i; flag_r = 1; }
	}

	if (flag_t == 0 || flag_r == 0)
		{pthread_mutex_unlock(&wfg->m);
		return 1;}

	if (wfg->data[t_index][r_index] != 1)
		{pthread_mutex_unlock(&wfg->m);
		return 1;}
	for (i = 0;i<wfg->size;i++)
		if (wfg->data[r_index][i] == 1)
			{pthread_mutex_unlock(&wfg->m); return 1;}
	wfg->data[t_index][r_index] = 0;
	wfg->data[r_index][t_index] = 1;

	pthread_mutex_unlock(&wfg->m);
	return 0;
}


/**
 * Removes an edge on the wait-for graph.
 *
 * If any edge exists between the thread and resource, this function
 * removes that edge (either a "hold" edge or a "wait" edge).
 *
 * @param wfg
 *   The wait-for graph data structure.
 * @param t_id
 *   A unique identifier to a thread.  A caller to this function may
 *   want to use the thread ID used by the system, by using the
 *   pthread_self() function.
 * @param r_id
 *   A unique identifier to a resource.
 *
 * @return
 * - 0, if an edge was removed
 * - non-zero, if no edge was removed
 */
int wfg_remove_edge(wfg_t *wfg, unsigned long t_id, unsigned long r_id)
{

	unsigned long i,j;
	unsigned long t_index, r_index;
	unsigned long flag_t = 0, flag_r = 0;

	pthread_mutex_lock(&wfg->m);
	for(i=0;i<wfg->size;i++)
	{
		if(wfg->dataID[i] == t_id && wfg->dataType[i]==0)
			{	t_index = i; flag_t = 1; }
		if(wfg->dataID[i] == r_id && wfg->dataType[i]==1)
			{	r_index = i; flag_r = 1; }
	}

	if (flag_t == 0 || flag_r == 0)
		{pthread_mutex_unlock(&wfg->m);
		return 1;}

	if (wfg->data[t_index][r_index] == 0 && wfg->data[r_index][t_index] == 0)
		{pthread_mutex_unlock(&wfg->m);return 1;}

	wfg->data[t_index][r_index] = 0;
	wfg->data[r_index][t_index] = 0;


	pthread_mutex_unlock(&wfg->m);
	return 0;	
	
}


/**
 * Returns the numebr of threads and the identifiers of each thread that is
 * contained in a cycle on the wait-for graph.
 *
 * If the wait-for graph contains a cycle, this function allocates an array
 * of unsigned ints equal to the size of the cycle, populates the array with
 * the thread identifiers of the threads in the cycle, modifies the value
 * of <tt>cycle</tt> to point to this newly created array, and returns
 * the length of the array.
 *
 * For example, if a cycle exists such that:
 *    <tt>t(id=1) => r(id=100) => t(id=2) => r(id=101) => t(id=1) (cycle)</tt>
 * ...the array will be of length two and contain the elements: [1, 2].
 *
 * This function only returns a single cycle and may return the same cycle
 * each time the function is called until the cycle has been removed.  This
 * function can return ANY cycle, but it must contain only threads in the
 * cycle itself.
 *
 * It is up to the user of this library to free the memory allocated by this
 * function.
 *
 * If no cycle exists, this function must not allocate any memory and will
 * return 0.
 *
 *
 * @param wfg
 *   The wait-for graph data structure.
 * @param cycle
 *   A pointer to an (unsigned int *), used by the function to return the
 *   cycle in the wait-for graph if a cycle exists.
 *   
 * @return
 *   The number of threads in the identified cycle in the wait-for graph,
 *   or 0 if no cycle exists.
 */
int wfg_get_cycle(wfg_t *wfg, unsigned long ** cycle)
{
	unsigned long i,j,k;
	unsigned long flag = 0, start = 0;

	unsigned long a[MAXSIZE][MAXSIZE];
	unsigned long r[MAXSIZE];
	unsigned long num = 0;
	unsigned long *c;
	pthread_mutex_lock(&wfg->m);

	for(i=0;i< wfg->size;i++)
		for(j=0;j< wfg->size;j++)
			a[i][j] = wfg->data[i][j];


	for(k=0;k<wfg->size;k++)
		for(i=0;i<wfg->size;i++)
			for(j=0;j<wfg->size;j++)
				if(a[i][k]==1 && a[k][j]==1)
						a[i][j] = 1;



	for(i=0;i<wfg->size;i++)
		if(a[i][i]==1 && wfg->dataType[i]==0)
			{ flag = 1; start = i; break;}
	

	if(flag == 0)
		{pthread_mutex_unlock(&wfg->m);
		return 0;}

	num = 1;
	r[0] = start;


	while(1)
	{
		for(i=0;i<wfg->size;i++)
			if( wfg->data[start][i] ==1 && wfg->dataType[i]==1)
				{start = i; break;}
		for(i=0;i<wfg->size;i++)
			if( wfg->data[start][i] ==1 && wfg->dataType[i]==0)
				{start = i; break;}
		if(start != r[0])
			{ num++; r[num-1] = start;}
		else
			break;
	}

	/*if(num<2)
	{
		for(i=0;i<wfg->size;i++)
			printf("%d",wfg->dataType[i]);
		printf("\n");

		for(i=0;i<wfg->size;i++)
		{
			
			for(j=0;j<wfg->size;j++)
				printf("%d ", wfg->data[i][j]);
			printf("\n");
		}

		for(i=0;i<wfg->size;i++)
		{
			
			for(j=0;j<wfg->size;j++)
				printf("%d ", a[i][j]);
			printf("\n");
		}
	}*/
	assert(num>=2);

	c = (unsigned long *) malloc(sizeof(unsigned long)*num);
	for(i=0;i<num;i++)
		c[i] = wfg->dataID[r[i]];

	*cycle = c;

	pthread_mutex_unlock(&wfg->m);

	return num;
}


/**
 * Prints all wait-for relations between threads in the wait-for graph.
 *
 * In a wait-for graph, a thread is "waiting for" another thread if a
 * thread has a "wait" edge on a resource that is being "held" by another
 * process.  This function prints all of the edges to stdout in a comma-
 * seperated list in the following format:
 *    <tt>[thread]=>[thread], [thread]=>[thread]</tt>
 *
 * If t(id=1) and t(id=2) are both waiting for r(id=100), but (r=100)
 * is held by t(id=3), the printout should be:
 *    <tt>1=>3, 2=>3</tt>
 *
 * When printing out the wait-for relations:
 * - this function may list the relations in any order,
 * - all relations must be printed on one line,
 * - all relations must be seperated by comma and a space (see above),
 * - a newline should be placed at the end of the list, and
 * - you may have an extra trailing ", " if it's easier for your program
 *   (not required, but should save you a bit of coding).
 *
 * @param wfg
 *   The wait-for graph data structure.
 */
void wfg_print_graph(wfg_t *wfg)
{
	unsigned long i,j,k;
	unsigned long flag = 0, start = 0;

	D(printf("graph size: %d\n", wfg->size);)
	pthread_mutex_lock(&wfg->m);

	for(i=0;i<wfg->size;i++)
		for(j=0;j<wfg->size;j++)
		{
			if(i==j || wfg->dataType[i]!=0 || wfg->dataType[j]!=0)
				continue;

			for(k=0;k<wfg->size;k++)
				if (wfg->dataType[k]!=1)
					continue;
				else
					if(  wfg->data[i][k] && wfg->data[k][j])
						printf("%lu=>%lu, ",wfg->dataID[i],wfg->dataID[j]);
		}

	printf("\n");
	pthread_mutex_unlock(&wfg->m);
	return;
	

}


/**
 * Destroys the wait-for graph data structure.
 *
 * This function must always be called last, after all other libwfg functions.
 *
 * @param wfg
 *   The wait-for graph data structure.
 */
void wfg_destroy(wfg_t *wfg)
{
	unsigned long i,j;
	pthread_mutex_lock(&wfg->m);
	for(i=0;i<MAXSIZE;i++)
		for(j=0;j<MAXSIZE;j++)
			wfg->data[i][j] = 0;

	wfg->size = 0;
	pthread_mutex_unlock(&wfg->m);

	pthread_mutex_destroy(&wfg->m);
	return;

}

