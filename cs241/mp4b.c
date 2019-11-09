/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"


#define INF 10000000
/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements. 
*/
typedef struct _job_t
{
  int jobid;
  int priority;
  int lastruntime;
  int runtime;
  int remaintime;
  int arrivaltime;
  int firstruntime;

} job_t;


typedef struct _corestatus
{
  int jobid;
  int busy;
  job_t * job;
 

} corestatus;




/* global variables */

int twait, tresponse, tturnround;
scheme_t gscheme;
int jobnum;
int coresize;


priqueue_t queue;
corestatus * cstatus;




/**
  Initalizes the scheduler.
 
  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/


int comparepriority( const void *ptr1, const void *ptr2)
{
  job_t *a = ptr1, *b = ptr2;
  if ( a->priority > b->priority)
    return 1;

 if ( a->priority < b->priority)
    return -1;

  /* they are equal */
  if (a->arrivaltime > b->arrivaltime )
    return 1;
  else
    return -1; 
}

int comparearrivaltime( const void *ptr1, const void *ptr2)
{
  job_t *a = ptr1, *b = ptr2;
  if ( a->arrivaltime > b->arrivaltime)
    return 1;

  if (a->arrivaltime == b->arrivaltime)
    return 0;
  else
    return -1; 
}

int compareruntime( const void *ptr1, const void *ptr2)
{
  job_t *a = ptr1, *b = ptr2;
  if ( a->runtime > b->runtime)
    return 1;

  if (a->runtime == b->runtime)
    return 0;
  else
    return -1; 
}


int compareremaintime( const void *ptr1, const void *ptr2)
{
  job_t *a = ptr1, *b = ptr2;
  if ( a->remaintime > b->remaintime)
    return 1;


  if ( a->remaintime < b->remaintime)
    return -1;




  if (a->arrivaltime > b->arrivaltime)
    return 1;
  else
    return -1; 
}


int compareRR( const void *ptr1, const void *ptr2)
{
  return -1; 
}


/*FCFS = 0, SJF, PSJF, PRI, PPRI, RR    five schedule algorithms */



void scheduler_start_up(int cores, scheme_t scheme)
{
  cstatus = (corestatus *) malloc(cores*sizeof(corestatus));
  coresize = cores;
  int i = 0;
  for(i=0;i<coresize;i++)
    cstatus[i].busy = 0;


  gscheme = scheme;
  switch(scheme)
  {
    case FCFS: priqueue_init(&queue, comparearrivaltime); break;
    case SJF: priqueue_init(&queue, compareruntime); break;
    case PSJF: priqueue_init(&queue, compareremaintime); break;
    case PRI: priqueue_init(&queue, comparepriority); break;
    case PPRI: priqueue_init(&queue, comparepriority); break;
    case RR: priqueue_init(&queue, compareRR); break;
    default: break;

  }

  tturnround = 0;
  tresponse = 0;
  twait =0;
  jobnum = 0;
  
  return;
}


/**
  Called when a new job arrives.
 
  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumptions:
    - You may assume that every job wil have a unique arrival time.

  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made. 
 
 */
int scheduler_new_job(int job_number, int time, int running_time, int priority)
{
  int i=0;
  
  job_t * replace, *a;
 
  int coreid;

  job_t * t = (job_t *) malloc(sizeof(job_t));
  t->jobid = job_number;
  t->arrivaltime = time;
  t->runtime = running_time;
  t->priority = priority;
  t->remaintime = running_time;
  t->firstruntime = -1;

  
  jobnum++;    /* total number of jobs is increased by one*/

  if(gscheme == FCFS || gscheme== SJF || gscheme== PRI || gscheme== RR)
  {
    
    for(i=0;i<coresize;i++)
    {
      if(cstatus[i].busy==0)
      {
        cstatus[i].busy = 1;
        cstatus[i].job = t;
        t->lastruntime = time;
        t->firstruntime = time;
        return i;
      }
    }

    /* not scheduled*/
    t->lastruntime = -1;
    priqueue_offer(&queue, t);
    return -1;


  }



  if(gscheme == PSJF)
  {

    /* first, if there is available source, just use it*/
    for(i=0;i<coresize;i++)
    {
      if(cstatus[i].busy==0)
      {
        cstatus[i].busy = 1;
        cstatus[i].job = t;
        t->lastruntime = time;
        t->firstruntime = time;
        return i;
      }
    }

    /* check remanining time of each job, remember the one with largest remain time, and latest arrival time*/
    
    a = cstatus[0].job;
    a->remaintime -= (time - a->lastruntime);
    a->lastruntime = time;
    replace = a;
    coreid = 0;


    for(i=1;i<coresize;i++)
    {
      a = cstatus[i].job;
      a->remaintime -= time - a->lastruntime;
      a->lastruntime = time;
      if (a->remaintime > replace->remaintime )
        {replace = a; coreid = i;}
      else
        if (a->remaintime== replace->remaintime && a->arrivaltime> replace->arrivaltime)
           {replace = a; coreid = i;}
    }

    if(replace->remaintime <= t->remaintime)
    {
      t->lastruntime = -1;
      t->firstruntime = -1;
      priqueue_offer(&queue, t);
      return -1;
    }
    else
    {
      cstatus[coreid].job = t;
      t->firstruntime = time;
      t->lastruntime = time;

      if (replace->firstruntime == time)
        replace->firstruntime = -1;

      priqueue_offer(&queue, replace);
      return coreid;


    }


  }




  if(gscheme == PPRI)
  {

    /* first, if there is available source, just use it*/
    for(i=0;i<coresize;i++)
    {
      if(cstatus[i].busy==0)
      {
        cstatus[i].busy = 1;
        cstatus[i].job = t;
        t->lastruntime = time;
        t->firstruntime = time;
        return i;
      }
    }

    /* check priority each job, remember the one with lowest priority and latest arrival time*/
    
    a = cstatus[0].job;
    replace = a;
    coreid = 0;


    for(i=1;i<coresize;i++)
    {
      a = cstatus[i].job;
      
      if (a->priority > replace->priority )
        {replace = a; coreid = i;}
      else
        if (a->priority== replace->priority && a->arrivaltime> replace->arrivaltime)
           {replace = a; coreid = i;}
    }

    if(replace->priority <= t->priority)
    {
      t->lastruntime = -1;
      t->firstruntime = -1;
      priqueue_offer(&queue, t);
      return -1;
    }
    else
    {
      cstatus[coreid].job = t;
      t->firstruntime = time;
      t->lastruntime = time;
      if (time== replace->firstruntime)
        replace->firstruntime = -1;

      priqueue_offer(&queue, replace);
      return coreid;


    }

  }




	return -1;
}


/**
  Called when a job has completed execution.
 
  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.
 
  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time)
{
  job_t * t;

  if( gscheme == FCFS || gscheme== SJF || gscheme== PRI || gscheme== RR)
  {
    
    t= cstatus[core_id].job;
    tturnround += time - t->arrivaltime;
    twait += time - t->arrivaltime - t->runtime;
    tresponse += t->firstruntime - t->arrivaltime;
    free(t);

    if ( priqueue_size(&queue) == 0)
    {
      cstatus[core_id].busy = 0;
      return -1;
    }
    else
    {
      t = priqueue_poll (&queue);
      if (t->firstruntime == -1)
        t->firstruntime = time;
      cstatus[core_id].job = t;
      return t->jobid;

    }
    

  }

  


  if(gscheme == PSJF || gscheme == PPRI)
  {

    t= cstatus[core_id].job;
    tturnround += time - t->arrivaltime;
    twait += time - t->arrivaltime - t->runtime;
    tresponse += t->firstruntime - t->arrivaltime;
    free(t);

    if ( priqueue_size(&queue) == 0)
    {
      cstatus[core_id].busy = 0;
      return -1;
    }
    else
    {
      t = priqueue_poll (&queue);
      if (t->firstruntime == -1)
        t->firstruntime = time;

      t->lastruntime = time;
      cstatus[core_id].job = t;
      return t->jobid;

    }
    


  }


 

}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.
 
  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator. 
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time)
{
  job_t * t = cstatus[core_id].job;
  priqueue_offer(&queue, t);

  t = priqueue_poll (&queue);
  cstatus[core_id].job = t;
  if(t->firstruntime==-1)
    t->firstruntime = time;

	return t->jobid;
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{

	return (float) twait/ (float) jobnum;
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
	return (float) tturnround/ (float) jobnum;
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
	return (float) tresponse/ (float) jobnum;
}


/**
  Free any memory associated with your scheduler.
 
  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
  free(cstatus);
  priqueue_destroy(&queue);

}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)  
  
  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
 */
void scheduler_show_queue()
{

}
