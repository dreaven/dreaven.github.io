/** @file server.c */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <queue.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "queue.h"
#include "libdictionary.h"

#define BUFFERSIZE 2000480 
#define THREADSIZE 200

#define	 D(x)  x

pthread_t th[THREADSIZE];
int sk[THREADSIZE];
int thid;
int sock_desc;

const int backlog = 10;

const char *HTTP_404_CONTENT = "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1>The requested resource could not be found but may be available again in the future.<div style=\"color: #eeeeee; font-size: 8pt;\">Actually, it probably won't ever be available unless this is showing up because of a bug in your program. :(</div></html>";
const char *HTTP_501_CONTENT = "<html><head><title>501 Not Implemented</title></head><body><h1>501 Not Implemented</h1>The server either does not recognise the request method, or it lacks the ability to fulfill the request.</body></html>";

const char *HTTP_200_STRING = "OK";
const char *HTTP_404_STRING = "Not Found";
const char *HTTP_501_STRING = "Not Implemented";

char * chtml="Content-Type: text/html\r\n";
char * ccss="Content-Type: text/css\r\n";
char * cjpg="Content-Type: image/jpeg\r\n";
char * cpng="Content-Type: image/png\r\n";
char * cplain="Content-Type: text/plain\r\n";
char * cgif="Content-Type: image/gif\r\n";
/**
 * Processes the request line of the HTTP header.
 * 
 * @param request The request line of the HTTP header.  This should be
 *                the first line of an HTTP request header and must
 *                NOT include the HTTP line terminator ("\r\n").
 *
 * @return The filename of the requested document or NULL if the
 *         request is not supported by the server.  If a filename
 *         is returned, the string must be free'd by a call to free().
 */
char* process_http_header_request(const char *request)
{
	// Ensure our request type is correct...
	if (strncmp(request, "GET ", 4) != 0)
		return NULL;

	// Ensure the function was called properly...
	assert( strstr(request, "\r") == NULL );
	assert( strstr(request, "\n") == NULL );

	// Find the length, minus "GET "(4) and " HTTP/1.1"(9)...
	int len = strlen(request) - 4 - 9;

	// Copy the filename portion to our new string...
	char *filename = malloc(len + 1);
	strncpy(filename, request + 4, len);
	filename[len] = '\0';

	// Prevent a directory attack...
	//  (You don't want someone to go to http://server:1234/../server.c to view your source code.)
	if (strstr(filename, ".."))
	{
		free(filename);
		return NULL;
	}

	return filename;
}

int checkFileType(const char * filename)
{
	int i,j;
	int N = strlen(filename);
	for(i=N-1;i>=0;i--)
		if(filename[i]=='.')
			break;
	if(i<0|| i == N-1)
		return -1;
	if (strcmp(filename+i+1, "html") == 0)
		return 0;
	else
		if (strcmp(filename+i+1, "css") == 0)
			return 1;
		else
			if  (strcmp(filename+i+1, "jpg") == 0)
				return 2;
			else
				if  (strcmp(filename+i+1, "png") == 0)
					return 3;
				else
					if (strcmp(filename+i+1, "gif") == 0)
						return 4;
	return -1;

}


void * workThread(void * ptr)
{
	int *ttt = (int *) ptr;
	int sock = *ttt;

	int rbufflen, readBytes;
	int i,j,k;
	char sbuff[BUFFERSIZE];
    char rbuff[BUFFERSIZE];
    char *filename;
    char *d, *start;
    dictionary_t dic;
    int fileflag = 1;
    int filelen;
    
    FILE *myfile;
    char filebuff[BUFFERSIZE], filedata[BUFFERSIZE], temp[BUFFERSIZE];
    int flagKeepAlive;
    int bflag;




    
    flagKeepAlive = 1;
    dictionary_init(&dic);

    while(flagKeepAlive)
    {

	    memset(filedata,0,sizeof(filedata));

	    fileflag = 200;
	    dictionary_remove(&dic, "Connection");
	    

	    memset(sbuff, 0, sizeof(sbuff)); 
		memset(rbuff, 0, sizeof(rbuff)); 

		while(1)
		{
			rbufflen = strlen(rbuff);
			readBytes = recv(sock, rbuff+rbufflen, BUFFERSIZE-rbufflen-1, 0);
			rbuff[rbufflen+readBytes] = 0;
			rbufflen = strlen(rbuff);
			if(rbufflen<4)
				continue;
			if (rbuff[rbufflen-1]=='\n' && rbuff[rbufflen-2]=='\r' && rbuff[rbufflen-3]=='\n' && rbuff[rbufflen-4]=='\r')
				break;
		}

		D(printf("the received request is:\n %s \n", rbuff);)
		D(printf("THE END OF REQUEST\n\n\n");)

		/* parse the header, and store the filename*/
		i = -1;
		start = rbuff;

		while( (d=strstr(start,"\r\n"))!=NULL  )
		{
			i++;
			d[0] = 0;
			d[1] = 0;
			if(i==0)
			{
				filename = process_http_header_request(start);
				D(printf("requested filename is: %s \n",filename));
			}
			else
			{
				dictionary_parse(&dic, start);
			}
			start = d+2;
			if (start[0]=='\r' && start[1]=='\n')
				break;
		}

		/* check whether keep alive is needed*/

		d = dictionary_get(&dic, "Connection");
		if (d==NULL)
			flagKeepAlive = 0;
		else
			if (strcasecmp(d,"Keep-Alive") ==0)
				flagKeepAlive = 1;
			else
				flagKeepAlive = 0;


		/*get filename*/
		if(filename==NULL)
			fileflag = 501;
		else
		{
			if(strcmp(filename,"/") == 0)
			{
				D(printf("index.html files is requested. \n");)
				free(filename);
				filename = (char *)malloc(BUFFERSIZE);
				memset(filename,0,sizeof(filename));
				strcpy(filename,"web/index.html");			
			}
			else
			{
				d = (char *)malloc(BUFFERSIZE);
				strcpy(d,"web");
				strcat(d,filename);
				free(filename);
				filename = d;
			}
		}


		if(fileflag == 501)
		{
			/* send back 501 response*/
			sprintf(sbuff,"HTTP/1.1 501 %s\r\n",HTTP_501_STRING );
			strcat(sbuff,chtml);
			sprintf(temp,"Content-Length: %d\r\n",strlen(HTTP_501_CONTENT) );
			strcat(sbuff,temp);
			if (flagKeepAlive==1)
				strcat(sbuff,"Connection: Keep-Alive\r\n");
			else
				strcat(sbuff,"Connection: close\r\n");

			strcat(sbuff,"\r\n");
			strcat(sbuff, HTTP_501_CONTENT);
			D(printf("The respose sent is: \n\n %s \n\n\n",sbuff);)
			send(sock,sbuff,strlen(sbuff),0);


		}
		else
		{
			/*D(printf("filename is: %s\n",filename);) */
			i = checkFileType(filename);
			if (i>=2)
				{bflag = 1; myfile = fopen(filename, "rb"); }
			else
				{ bflag = 0; myfile = fopen(filename, "r"); }

			if(myfile==NULL)
			{
				fileflag = 404;
				printf("%s is not found\n",filename);
			}
			else
			{
				if(bflag==1)
				{

					fseek(myfile, 0, SEEK_END);
					filelen = ftell(myfile);
					rewind(myfile);

					fread(filedata, 1, filelen, myfile);
					printf("binary file length is: %d\n",filelen);


				}
				else
				{
					while(!feof(myfile))
					{
						fgets(filebuff,BUFFERSIZE-2,myfile);
						strcat(filedata,filebuff);
					}
				}
				fclose(myfile);
			}

		


		}

		if(fileflag == 404)
		{


			free(filename);
			/* send back 501 response*/
			sprintf(sbuff,"HTTP/1.1 404 %s\r\n",HTTP_404_STRING );
			strcat(sbuff,chtml);
			sprintf(temp,"Content-Length: %d\r\n",strlen(HTTP_404_CONTENT) );
			strcat(sbuff,temp);
			if (flagKeepAlive==1)
				strcat(sbuff,"Connection: Keep-Alive\r\n");
			else
				strcat(sbuff,"Connection: close\r\n");

			strcat(sbuff,"\r\n");
			strcat(sbuff, HTTP_404_CONTENT);
			D(printf("The respose sent is: \n\n %s \n\n\n",sbuff);)
			send(sock,sbuff,strlen(sbuff),0);

		}

		if(fileflag == 200)
		{
			/* send back 501 response*/
			sprintf(sbuff,"HTTP/1.1 200 %s\r\n",HTTP_200_STRING );

			i = checkFileType(filename);
			switch(i)
			{
				case -1: strcat(sbuff,cplain); break;
				case 0: strcat(sbuff,chtml); break;
				case 1: strcat(sbuff,ccss); break;
				case 2: strcat(sbuff,cjpg); break;
				case 3: strcat(sbuff,cpng); break;
				case 4: strcat(sbuff,cgif); break;
				default : break;

			}
				

			if (bflag == 1)
				sprintf(temp,"Content-Length: %d\r\n", filelen);
			else
				sprintf(temp,"Content-Length: %d\r\n",strlen(filedata) );

			strcat(sbuff,temp);
			if (flagKeepAlive==1)
				strcat(sbuff,"Connection: Keep-Alive\r\n");
			else
				strcat(sbuff,"Connection: close\r\n");

			strcat(sbuff,"\r\n");
			if (bflag == 0)
				strcat(sbuff, filedata);
			else
			{
				i = strlen(sbuff);
				for(j=0;j<filelen;j++)
					sbuff[i+j] = filedata[j];
			}
			D(printf("The respose sent is: \n\n %s \n\n\n",sbuff);)
			if (bflag==0)
				send(sock,sbuff,strlen(sbuff),0);
			else
				send(sock,sbuff,i+filelen,0);

			free(filename);

		}
	}





	dictionary_destroy(&dic);

	close(sock);

	return;


}

void release(int sig)
{
	int i=0;
	for(i=0;i<=thid;i++)
		close(sk[i]);

	close(sock_desc);

	exit(0);

}

int main(int argc, char **argv)
{


	int i,j,k;
	int portnum;
	thid = 0;
	socklen_t len;

	if (argc !=2)
	{
		printf("Invalid input arguments! \n");
		exit(1);
	}

	portnum = atoi(argv[1]);

	signal(SIGINT, release);

	
    struct sockaddr_in server_addr, client_addr; 

    

    

    sock_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_desc == -1)
    {
    	printf("socket request failed! \n");
    	exit(1);
    }


    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));


    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(portnum); 

    i = bind(sock_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)); 
    if (i== -1 )
    {
    	printf("bind error! \n");
    	exit(1);
    }

    i = listen(sock_desc, backlog);
    if (i == -1)
    {
    	printf("listen error! \n");
    	exit(1);
    } 

    len = sizeof(client_addr);

   
    thid = 0;

    while(1)
    {
    	sk[thid] = accept(sock_desc, &client_addr, &len);
    	if (sk[thid]==-1)
    	{
    		printf("Accept function error! Now continue \n");
    		continue;
    	}

    	assert(thid>=0);
    	assert(thid<THREADSIZE);

    	pthread_create(th+thid, NULL, workThread,  sk+thid);


    	thid++;
    }


    close(sock_desc);



	return 0;
}
