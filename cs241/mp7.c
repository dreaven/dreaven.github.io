/** @file libmapreduce.c */
/* 
 * CS 241
 * The University of Illinois
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <poll.h>

#include "libmapreduce.h"
#include "libdictionary.h"

#define D(x) 


int num ;
int *pfds;
char ** buffer;
int * flagt;
pthread_t th;
pid_t childpid[200];



static const int BUFFER_SIZE = 2048;  /**< Size of the buffer used by read_from_fd(). */


/**
 * Adds the key-value pair to the mapreduce data structure.  This may
 * require a reduce() operation.
 *
 * @param key
 *    The key of the key-value pair.  The key has been malloc()'d by
 *    read_from_fd() and must be free()'d by you at some point.
 * @param value
 *    The value of the key-value pair.  The value has been malloc()'d
 *    by read_from_fd() and must be free()'d by you at some point.
 * @param mr
 *    The pass-through mapreduce data structure (from read_from_fd()).
 */
static void process_key_value(const char *key, const char *value, mapreduce_t *mr)
{

	char *oldvalue;
	char *newvalue;

	D(printf("process_key_value function is called. \n");)
	oldvalue = dictionary_get(mr->dic, key);
	if (oldvalue == NULL)  /* not found the key, value pair*/
	{
		 dictionary_add(mr->dic,  key,  value);
		 return;
	}
	else
	{
		newvalue = mr->reduce(oldvalue, value);

		dictionary_remove_free(mr->dic, key);
		dictionary_add(mr->dic, key, newvalue);
		free(value);

		return;
	}
}


/**
 * Helper function.  Reads up to BUFFER_SIZE from a file descriptor into a
 * buffer and calls process_key_value() when for each and every key-value
 * pair that is read from the file descriptor.
 *
 * Each key-value must be in a "Key: Value" format, identical to MP1, and
 * each pair must be terminated by a newline ('\n').
 *
 * Each unique file descriptor must have a unique buffer and the buffer
 * must be of size (BUFFER_SIZE + 1).  Therefore, if you have two
 * unique file descriptors, you must have two buffers that each have
 * been malloc()'d to size (BUFFER_SIZE + 1).
 *
 * Note that read_from_fd() makes a read() call and will block if the
 * fd does not have data ready to be read.  This function is complete
 * and does not need to be modified as part of this MP.
 *
 * @param fd
 *    File descriptor to read from.
 * @param buffer
 *    A unique buffer associated with the fd.  This buffer may have
 *    a partial key-value pair between calls to read_from_fd() and
 *    must not be modified outside the context of read_from_fd().
 * @param mr
 *    Pass-through mapreduce_t structure (to process_key_value()).
 *
 * @retval 1
 *    Data was available and was read successfully.
 * @retval 0
 *    The file descriptor fd has been closed, no more data to read.
 * @retval -1
 *    The call to read() produced an error.
 */
static int read_from_fd(int fd, char *buffer, mapreduce_t *mr)
{
	D(printf("read_from_fd is called \n");)


	/* Find the end of the string. */
	int offset = strlen(buffer);

	/* Read bytes from the underlying stream. */
	int bytes_read = read(fd, buffer + offset, BUFFER_SIZE - offset);
	if (bytes_read == 0)
		return 0;
	else if(bytes_read < 0)
	{
		fprintf(stderr, "error in read.\n");
		return -1;
	}

	buffer[offset + bytes_read] = '\0';

	/* Loop through each "key: value\n" line from the fd. */
	char *line;
	while ((line = strstr(buffer, "\n")) != NULL)
	{
		*line = '\0';

		/* Find the key/value split. */
		char *split = strstr(buffer, ": ");
		if (split == NULL)
			continue;

		/* Allocate and assign memory */
		char *key = malloc((split - buffer + 1) * sizeof(char));
		char *value = malloc((strlen(split) - 2 + 1) * sizeof(char));

		strncpy(key, buffer, split - buffer);
		key[split - buffer] = '\0';

		strcpy(value, split + 2);

		/* Process the key/value. */
		process_key_value(key, value, mr);

		/* Shift the contents of the buffer to remove the space used by the processed line. */
		memmove(buffer, line + 1, BUFFER_SIZE - ((line + 1) - buffer));
		buffer[BUFFER_SIZE - ((line + 1) - buffer)] = '\0';
	}

	return 1;
}


/**
 * Initialize the mapreduce data structure, given a map and a reduce
 * function pointer.
 */
void mapreduce_init(mapreduce_t *mr, 
                    void (*mymap)(int, const char *), 
                    const char *(*myreduce)(const char *, const char *))
{
	mr->map = mymap;
	mr->reduce = myreduce;
	mr->dic = (dictionary_t * )malloc(sizeof(dictionary_t));
	dictionary_init(mr->dic);

	return;	
}


/* word thread function*/

void * workthread(void * ptr)
{
	mapreduce_t *mr = ptr;
	int maxfd;
	fd_set readset;
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	int i,j;
	int res;
	
	int flag_allread = 0;


	maxfd = 0;
	for(i=0;i<num;i++)
		if (maxfd< pfds[2*i])
			maxfd = pfds[2*i];
	maxfd++;

	j = 1;
	while(j<10000)
	{
		flag_allread = 1;
		for(i=0;i<num;i++)
			if (flagt[i] ==0 )
				flag_allread =0;

		if (flag_allread ==0)
			j = 0;

		j++;
		
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		FD_ZERO(&readset);
		for(i=0;i<num;i++)
			FD_SET(pfds[2*i], &readset);


		res = select(maxfd, &readset, NULL, NULL, &tv);

		D(printf("select function returned value: %d \n", res);)

		if (res == -1)
			perror("select() error! \n");
		else
			if (res>0)
			{
				/* some data is avaliable*/
				D(printf("read function is called into \n ");)
				for(i=0;i<num;i++)
					if(FD_ISSET(pfds[2*i],&readset))
						{ read_from_fd(pfds[2*i],buffer[i],mr); flagt[i] = 1;}

				D(printf("read function is called out \n");)

			}
			else
				break;
	}



	return;


}


/**
 * Starts the map() processes for each value in the values array.
 * (See the MP description for full details.)
 */
void mapreduce_map_all(mapreduce_t *mr, const char **values)
{
	num =0;	
	pid_t pid;
	int i,j;

	while(values[num]!=NULL)
		num++;
	if (num == 0)
		return;

	D(printf("Number of datasets is: %d \n", num);)

	pfds = (int *) malloc(sizeof(int)*2*num);
	buffer = (char **) malloc(sizeof(char*)*num);
	flagt = (int *) malloc(sizeof(int)*num);

	for(i=0;i<num;i++)
		flagt[i] = 0;

	for(i=0;i<num;i++)
		{ buffer[i] = (char *) malloc(sizeof(char)*(BUFFER_SIZE+1)); buffer[i][0] = '\0';}

	/* initialize the pipes*/
	for(i=0;i<num;i++)
		pipe(pfds+ 2*i);
	for(i=0;i<num;i++)
	{
		childpid[i] = fork();
		if(childpid[i]==0)
		{
			close(pfds[2*i]);
			map(pfds[2*i+1],values[i]);
			exit(0);
			break;
		}
		else
			close(pfds[2*i+1]);
	}


	pthread_create(&th,NULL, workthread, (void *)mr);

	
}


/**
 * Blocks until all the reduce() operations have been completed.
 * (See the MP description for full details.)
 */
void mapreduce_reduce_all(mapreduce_t *mr)
{
	int maxfd;
	fd_set readset;
	struct timeval tv;
	int i;
	int status;
	int res;

	maxfd = 0;
	for(i=0;i<num;i++)
		if (maxfd< pfds[2*i])
			maxfd = pfds[2*i];
	maxfd++;

	pthread_join(th, NULL);
	
	for(i=0;i<num;i++)
		waitpid(childpid[i],&status, WEXITED);

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	FD_ZERO(&readset);
	for(i=0;i<num;i++)
		FD_SET(pfds[2*i], &readset);


	res = select(maxfd, &readset, NULL, NULL, &tv);

	D(printf("select function returned value: %d \n", res);)

	if (res == -1)
		perror("select() error! \n");
	else
		if (res>0)
		{
			/* some data is avaliable*/
			D(printf("read function is called into \n ");)
			for(i=0;i<num;i++)
				if(FD_ISSET(pfds[2*i],&readset))
					{ read_from_fd(pfds[2*i],buffer[i],mr); flagt[i] = 1;}

			D(printf("read function is called out \n");)

		}
		



	return;
}


/**
 * Gets the current value for a key.
 * (See the MP description for full details.)
 */
const char *mapreduce_get_value(mapreduce_t *mr, const char *result_key)
{
	char * value;
	value = dictionary_get(mr->dic, result_key);

	return value;
}


/**
 * Destroys the mapreduce data structure.
 */
void mapreduce_destroy(mapreduce_t *mr)
{
	int i;
	dictionary_destroy_free(mr->dic);
	free(mr->dic);

		/* free the resource*/

	free(pfds);
	for(i=0;i<num;i++)
		free(buffer[i]);
	free(buffer);
	free(flagt);
	
}

