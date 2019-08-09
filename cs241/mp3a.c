/** @file shell.c */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"

/**
 * Starting point for shell.
 */
#define SSIZE  300

#define D(x) 

void execut(log_t *l, char * c)
{
	int flag;
	log_t * first;
	if (strcmp(c,"exit") == 0)
	{
		log_destroy(l);
		exit(0);
	}
	else
	{
		if (c[0]=='c' && c[1]=='d')   /* cd command*/
		{
				log_append(l, c);   /* add the command to history*/
				flag = chdir(c+3);
				D(printf("cd is called: path name is: %s\n",c+3);)
				if (flag!=0)
					printf("%s: No such file or directory\n",c+3);

		}
		else
		{
				if(strcmp(c,"!#")==0)
				{
					first = l->next;
					while(first!=NULL)
					{
						printf("%s\n",first->data);
						first = first->next;
					}
					free(c);
				}
				else  /* system call*/
				{
					flag = system(c);

					D(printf("system call is called, the command is %s",c);)
					log_append(l, c); 

				}
		}
	}
}


int main()
{
    
	log_t l;
	int flag;
	

	char * c, *d;
	char cwd[SSIZE ];

	size_t sz = SSIZE - 2;
	log_init(&l);
	
	

	while(1)
	{
		pid_t pid = getpid();
		getcwd(cwd, SSIZE-2);
		c = (char *) malloc(SSIZE+2);
		printf("(pid=%d)%s$ ", pid, cwd);
		getline(&c, &sz, stdin);

		D(printf("printf command is: %s\n",c);)
		flag = strlen(c);
		c[flag-1] = 0;
		D(printf("length is: %d \n",flag);)

		if (strlen(c) ==0)
			{ free(c); continue;}
		else
		{
			if ( c[0]=='!'  )
			{

				flag = strcmp(c,"!#");
				
				if( flag == 0 )
					execut(&l,c);
				else
				{
					d= log_search(&l, c+1);
					if(d==NULL)
						printf("No Match\n");
					else
					{
						printf("%s matches %s\n", c+1, d);
						strcpy(c,d);

						D(printf("new command is %s",c);)
						execut(&l, c);
					}

				}
			}
			else
				execut(&l,c);
			
				
		}


	}
    
}

