#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "servertest.h"


pthread_mutex_t lock; 
int sockfd = -1;

pthread_t readt = -1;
pthread_t writet = -1;

void ctrlC_shutdown();

void * commandin(void * arg){ 
    char debug[256];
    char string[256];
    memset(string, '\0', sizeof(string));

    while(1){
    sleep(2);
        pthread_mutex_lock(&lock);
	  while(1)
	  {
            printf("Input command:\n");    
            fgets(string,255,stdin);

	    //check that the command is ok 
	    if(strncmp(string, "create ", 6) == 0)
	    {
		break;
	    }else if(strncmp(string, "serve ", 6) == 0){
		break;
	    }else if(strncmp(string, "withdraw ", 9) == 0){
		break;
	    }else if(strncmp(string, "query", 5) == 0){
		break;
	    }else if(strncmp(string, "deposit ", 8) == 0){
		break;
	    }else if(strncmp(string, "end", 3) == 0){
		break;
	    }else if(strncmp(string, "quit", 4) == 0){
		break;
	    }else{
		printf("This is not a valid command.\n");
	    }
	  }
            strcpy(debug, string); 
            strtok(debug, " ");
            
	    if(write(sockfd, string, strlen(string))>0){

            }else{
                printf("Couldn't write to the socket \n");

            }
    
        pthread_mutex_unlock(&lock); 

            if(strncmp(string, "quit", 4)==0){
                printf("exiting\n");
                ctrlC_shutdown();
                return 0;
            }
            
        memset(string, '\0', sizeof(string));
    }
    
    return NULL;
}


void * serverout (void *arg){
    char string[256];
    memset(string, '\0', sizeof(string));
    
    while (1){
        
        //pthread_mutex_lock(&lock);
            long n = read(sockfd,string,255); 
            if (n < 0) {printf("Couldn't read from socket\n"); }
            
            if(strncmp(string, "quit", 4)==0){
                printf("Successfully exited!\n");

                ctrlC_shutdown();
                return 0;
            }
            
            printf("%s\n",string);
            memset(string, '\0', sizeof(string));

        
        //pthread_mutex_unlock(&lock);
        
    }
    
    return NULL;
}


void ctrlC_shutdown()
{
    if(readt != -1){
        pthread_detach(readt);
    }
    if(writet != -1){
        pthread_detach(writet);
    }

    exit(0);
}



int main(int argc, char ** argv)
{
    
    struct sockaddr_in serverinfo; 
    struct hostent *serverip; 
    
    signal(SIGINT, ctrlC_shutdown);
    
    if(argc != 3){ 
    printf("Incorrect Arg count \n");
    exit(0);
    }
    
    
    if(pthread_mutex_init(&lock, NULL)!=0){printf("Failed creating mutex \n");}

    printf("Welcome! Attempting to connect to host \n");
     
    serverip = gethostbyname(argv[1]);
    if(serverip==NULL){ printf("server does not exist! \n"); }
   
    sockfd =  socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd<0){printf("error creating socket \n"); }
    
    memset(&serverinfo, '\0', sizeof(serverinfo));
    
    serverinfo.sin_family = AF_INET; 
    serverinfo.sin_port = htons(atoi(argv[2]));  
    bcopy((char*) serverip->h_addr, (char*) &serverinfo.sin_addr.s_addr, serverip->h_length);

    socklen_t size = sizeof(serverinfo);

    //wait for 3 seconds
    while(connect(sockfd, (struct sockaddr *)&serverinfo,size)<0)
    {
        printf("Error Trying To Connect. Trying Again In 3 Seconds\n");
        sleep(3);
    }

    printf("Connection Successful \n\n");
    printf("Please choose one of the following options: \n");
    printf("\tcreate <accountname> \n");
    printf("\tserve <accountname> \n");
    printf("\tdeposit <value> \n");
    printf("\twithdraw <value> \n");
    printf("\tquery \n");
    printf("\tend \n");
    printf("\tquit \n");
    
  
    pthread_create(&writet, NULL,&commandin,NULL);
    pthread_create(&readt, NULL,&serverout ,NULL);
    
    pthread_join(writet, NULL);
    pthread_join(readt, NULL);

    pthread_mutex_destroy(&lock);
    close(sockfd);
    return 0;
}
