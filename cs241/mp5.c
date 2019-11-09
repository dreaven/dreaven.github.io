/** @file parmake.c */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "queue.h"
#include "rule.h"
#include  <string.h>
#include  <assert.h>

#ifndef DX
#define D(x)  

#endif
/**
 * Entry point to parmake.
 */

 int threadnum;
 char * makefilepath[2];
 int flagSpecifymake;
 int flagSpecifythread;
 char ** targetList;
 char *makeFile;


 queue_t queue;
 pthread_t * th;
 int * finish;
 int * running;
 pthread_mutex_t *fm, *rm;
 pthread_mutex_t  cond_mutex;
 pthread_cond_t flagFinish;
 int targetnumber;
 int finishTarget;


void freeQ(void * t, void * b)
{
	free(t);
	return;
}


void freeRule(void * t, void * b)
{
	rule_t * a = t;
	free(a->target);
	queue_iterate(a->deps, freeQ, NULL);
	queue_iterate(a->commands, freeQ, NULL);
	queue_destroy(a->deps);
	queue_destroy(a->commands);


	return;
}


void parsed_new_target (char *target)
{
	int len;
	D(printf("parsed_new_target is called: target name is %s \n", target);)
	rule_t * a = (rule_t *) malloc(sizeof(rule_t));

	len = strlen(target)+1;
	a->target = (char *) malloc(sizeof(char) * len);
	strcpy(a->target, target);
	a->deps = (queue_t *) malloc(sizeof(queue_t));
	a->commands = (queue_t *) malloc(sizeof(queue_t));
	queue_init(a->deps);
	queue_init(a->commands);

	queue_enqueue(&queue, a);
	return;
}
	



void parsed_new_dependency (char *target, char *dependency)
{
	rule_t * a;
	int sz = queue_size(&queue);
	int i;

	char *depnew;
	for(i=0;i<sz;i++)
	{
		a = queue_at(&queue, i);
		if ( strcmp(a->target, target)==0 )
			break;
	}

	assert(i<sz);

	depnew = (char *) malloc(sizeof(char) * ( strlen(dependency) + 1) );
	strcpy(depnew, dependency);
	queue_enqueue(a->deps, depnew);
	return;

}
	

void parsed_new_command (char *target, char *command)
{
	rule_t * a;
	int sz = queue_size(&queue);
	int i;
	char * commandnew;
	for(i=0;i<sz;i++)
	{
		a = queue_at(&queue, i);
		if (strcmp(a->target,target)==0)
			break;
	}
	assert(i<sz);

	commandnew = (char *) malloc(sizeof(char) * ( strlen(command) + 1) );
	strcpy(commandnew, command);
	queue_enqueue(a->commands, commandnew);
	return;

}


void printQ(queue_t *q)
{
	D(printf("Queue size is %d\n", queue_size(q));)
	rule_t * a;
	int sz = queue_size(q);
	int i,j;
	int k;
	for(i=0;i<sz;i++)
	{
		a = queue_at(q, i);
		D(printf("Target %d is  %s, length is %d \n", i, (char*) a->target, strlen((char *)a->target));)
		k = queue_size(a->deps);
		if (k== 0)
			{D(printf("There is no dependncy \n");)
			continue;}
		for(j = 0;j<k;j++)
		{
			D(printf("Dependendy  %d is  %s \n", j, queue_at(a->deps,j) ); ) 
		}
		
	}


	return;

}


/* key function to run the target */
void * runTarget( void * params )
{
	D(printf("20 so far so good in run Target function\n");) 
	int i,j,k;

	int len = targetnumber;
	int allDone = 1;
	int t1,t2,commandlen;
	rule_t *a, *a1;
	queue_t * q1;
	char *c1, *c2;

	int flagdep = 1;
	int workdone = 0;
	int currentFinishTarget = 0;
	int isFile, isallFile;
	struct stat fst, fsd;
	int newer;

	D( printf("1 so far so good in run Target function\n" ) ; )

	D(printf("Queue length is %d", len); )

	while(1)
	{	

		pthread_mutex_lock(&cond_mutex);
		currentFinishTarget = finishTarget;
		pthread_mutex_unlock(&cond_mutex);

		allDone = 1;
		workdone = 0;
		for(i=0;i<len;i++)
		{
			a = queue_at(&queue,i);

			pthread_mutex_lock(fm+i);
			pthread_mutex_lock(rm+i);
			if (running[i] == 1 || finish[i] == 1)
			{
				pthread_mutex_unlock(fm+i);
				pthread_mutex_unlock(rm+i);
				continue;
			}

			

			allDone = 0;
			/* check the dependency*/
			q1 = a->deps;
			t1 = queue_size(q1);

			if (access(a->target, F_OK) == 0)
				isFile = 1;
			else
				isFile = 0;

			isallFile = 1;

			for(k=0;k<t1;k++)
			{
				c1 = queue_at(q1,k);
				if (access(c1,F_OK) !=0 )
				{	
					isallFile = 0 ;
					break;
				}
			}

			if (isallFile == 1 && isFile == 1)
			{
				newer = 1;
				stat(a->target,&fst);

				for(k=0;k<t1;k++)
				{
					c1 = queue_at(q1,k);
					stat(c1,&fsd);
					if (fst.st_mtime < fsd.st_mtime)
						{ newer = 0; break;}
				}
				

				if (newer == 1)
				{



					running[i] = 0;
					finish[i] = 1;

					pthread_mutex_unlock(fm+i);
					pthread_mutex_unlock(rm+i);


				/* send out block signal*/
					workdone = 1;

					pthread_mutex_lock(&cond_mutex);
					finishTarget++;
					pthread_mutex_unlock(&cond_mutex);
					pthread_cond_broadcast(&flagFinish);
					break;

				}
				else
				{
					running[i] = 1;
					pthread_mutex_unlock(fm+i);
					pthread_mutex_unlock(rm+i);

					commandlen = queue_size(a->commands);
					for(j=0;j<commandlen;j++)
					{
						c1 = queue_at(a->commands,j);
						k = system(c1);
						if (k!=0)
							exit(-1);
					}

					pthread_mutex_lock(fm+i);
					pthread_mutex_lock(rm+i);
					running[i] = 0;
					finish[i] = 1;

					pthread_mutex_unlock(fm+i);
					pthread_mutex_unlock(rm+i);


					/* send out block signal*/
					workdone = 1;

					pthread_mutex_lock(&cond_mutex);
					finishTarget++;
					pthread_mutex_unlock(&cond_mutex);
					pthread_cond_broadcast(&flagFinish);
					break;
				}
			}

			





			D(printf("target %d : dependence length %d \n", i , t1);) 
			flagdep = 1;

			for(k=0;k<t1;k++)
			{
				c1 = queue_at(q1,k);
				/* search position in queue*/
				if(access(c1,F_OK) == 0)
					continue;

				for(j=0;j<len;j++)
				{
					if (j == i)
						continue;
					a1 = queue_at(&queue,j);
					c2 = a1->target;
					if (strcmp (c2, c1) == 0)
						break;
				}

				assert(j<len);
				if (finish[j]==0)
					{ flagdep = 0; break;}

			}

			if(flagdep == 0)   /* not meet dependency*/
			{
				pthread_mutex_unlock(fm+i);
				pthread_mutex_unlock(rm+i);
				continue;
			}
			else
			{
				running[i] = 1;
				pthread_mutex_unlock(fm+i);
				pthread_mutex_unlock(rm+i);

				commandlen = queue_size(a->commands);
				for(j=0;j<commandlen;j++)
				{
					c1 = queue_at(a->commands,j);
					k = system(c1);
					if (k!=0)
						exit(-1);
				}

				pthread_mutex_lock(fm+i);
				pthread_mutex_lock(rm+i);
				running[i] = 0;
				finish[i] = 1;

				pthread_mutex_unlock(fm+i);
				pthread_mutex_unlock(rm+i);


				/* send out block signal*/
				workdone = 1;

				pthread_mutex_lock(&cond_mutex);
				finishTarget++;
				pthread_mutex_unlock(&cond_mutex);
				pthread_cond_broadcast(&flagFinish);

				


				break;


			}



		}

		if (allDone == 1)
			return NULL;
		else
			if (workdone == 0)
			{
			/* wait for signals*/
				pthread_mutex_lock(&cond_mutex);
				while (finishTarget<= currentFinishTarget)
					pthread_cond_wait(&flagFinish,&cond_mutex);
				pthread_mutex_unlock(&cond_mutex);

			}
	}



	return NULL;

}

int main(int argc, char **argv)
{	
	/* declare variables here*/
	
	int startTargetIndex = 1;
	int i,j,k;


	/* end of declaration*/

	targetnumber = 0;
	threadnum = 1;
	makefilepath[0] = "./makefile";
	makefilepath[1] = "./Makefile";
	flagSpecifymake = 0;
	flagSpecifythread = 0;


	/* parse arguments */
	if (argc>=2)
	{
		i = 1;
		if (strcmp(argv[i],"-f") == 0)
		{
			flagSpecifymake = 1;
			makefilepath[0] = argv[i+1];
		}
		else
		{
			if (  strcmp(argv[i],"-j") == 0 )
			{
				flagSpecifythread = 1;
				threadnum = atoi(argv[i+1]);
			}
		}

		i = 3;
		if (argc>=i+1)
		{
		if (strcmp(argv[i],"-f") == 0)
		{
			flagSpecifymake = 1;
			makefilepath[0] = argv[i+1];
		}
		else
		{
			if (  strcmp(argv[i],"-j") == 0 )
			{
				flagSpecifythread = 1;
				threadnum = atoi(argv[i+1]);
			}
		}
		}
	}


	/* check the existence of the file*/
	if (flagSpecifymake == 0)
	{
		if (access( makefilepath[0], F_OK) == 0 )
			makeFile = makefilepath[0];
		else
			if (access (makefilepath[1], F_OK) == 0)
				makeFile = makefilepath[1];
			else
				exit(-1);
	}
	else
	{
		if (access (makefilepath[0],F_OK) == 0)
			makeFile = makefilepath[0];
		else
			exit(-1);
	}

	assert(threadnum>=1);

	startTargetIndex = 2*(flagSpecifythread + flagSpecifymake ) + 1;

	targetnumber = argc- startTargetIndex;

	targetList = (char **) malloc(sizeof(char*) * (targetnumber + 1)); 
	for(i = startTargetIndex;i<argc;i++)
	{
		targetList[i-startTargetIndex] = argv[i];
	}
	targetList[targetnumber] = NULL;

	D(printf("There are %d targets \n", targetnumber);)
	D(printf("Thread number is %d \n", threadnum);)
	D(printf("Makefile path is %s \n", makeFile);)


	queue_init(&queue);




	parser_parse_makefile(
		makeFile, 
		targetList, 
		parsed_new_target, 
		parsed_new_dependency, 
		parsed_new_command
		);


	D(printf("parseing: targetList: \n");)
	for(i=0;i<targetnumber;i++)
	{
		D(printf("%d th targert: %s \n", i, targetList[i]);)
	}
	/* print queue information*/

	
	/*printQ(&queue); */

	finishTarget = 0;
	targetnumber = queue_size(&queue);

	D(printf("Finally targetnumber is %d\n", targetnumber);)

 	finish = (int *) malloc(sizeof(int)*targetnumber);
 	running = (int *) malloc(sizeof(int)*targetnumber);
 	for(i=0;i<targetnumber;i++)
 	{
 		finish[i] = 0; 
 		running[i] = 0;
 	}

 	th = (pthread_t *) malloc(sizeof(pthread_t) * threadnum);

 	
 	fm = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)*targetnumber);
	rm = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)*targetnumber);
	for(i=0;i<targetnumber;i++)
	{	
		pthread_mutex_init(fm+i,NULL); 
		pthread_mutex_init(rm+i,NULL);
	}

 	pthread_mutex_init(&cond_mutex, NULL);

	pthread_cond_init(&flagFinish,NULL);

	


	for(i=0;i<threadnum;i++)
		pthread_create(th+i, NULL, runTarget,NULL);


	for(i=0;i<threadnum;i++)
		pthread_join(th[i], NULL);
	



	for(i=0;i<targetnumber;i++)
	{
		pthread_mutex_destroy(fm+i);
		pthread_mutex_destroy(rm+i);
	}

	pthread_mutex_destroy(&cond_mutex);
	pthread_cond_destroy(&flagFinish);

	free(fm);
	free(rm);
	free(finish);
	free(running);
	free(th);
	free(targetList);



	queue_iterate(&queue, freeRule, NULL);
	queue_destroy(&queue);



	return 0; 
}
