#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>

#define INTERVAL 15

/* This is the setup for a basic server.
 * We have now implemented multiple client functionality.
 * We have implemented the use of threading.  
 * We are implementing the use of timers.  */


//we must make a LL of client accounts to sort the addive clients
typedef struct clientAcct
{
	char* name;
	double balance;
	int active; //lets use 0 for not active and 1 for active
	struct clientAcct* next;
} clientAcct;

//set up the LL of client_acct
clientAcct* client_front = NULL;
clientAcct* client_traverser = NULL;


//we can make a LL of pthread_t to store all the threads we make
typedef struct threadsLL
{
	int sockfd;
	pthread_t name;
	struct threadsLL* next;
} threadsLL;

//set up the LL for threadsLL
threadsLL* front = NULL;
threadsLL* traverser = NULL;


//make the socket descriptor a global variable
int socket_descriptor = 0;



int createAcct(char*);
clientAcct* serve(char*);


//we can now start to try and establish PTHREADS

void *clientHandler(void* client_socket)
{
	int socket_num = *(int*) client_socket;

	char client_message[256];
	memset(client_message, '\0', sizeof(client_message));

	int read_return = 0;
	int acct_lock = 0;//0 if we are NOT locked into an acct, or 1 if we are locked into an acct
	clientAcct* acctSession;
	acctSession = NULL;
	double moneyHolder = 0;
	char* token;
	char* wasteBin;
	char* retnstr;

	while(1)
	{
		//lets reset the string storage client_message before starting
		memset(client_message, '\0', sizeof(client_message));

		read_return = read(socket_num, client_message, 255);//this will read from socket and store message in client_message char array
		if(read_return  == -1)
		{
			//we are going to need to send back read error, please try again
			write(socket_num, "We had trouble reading that. Please try again.", 52);
			
		}

		//now we will parse the text to see what we have been requested to do
		if(strncmp(client_message, "create ", 7) == 0)
		{
			//printf("We got the create command\n");

			//need to parse for the arg after the command
			token = strtok(client_message, " ");
			token = strtok(NULL, "");

			//we will need to create the acct if we are not locked into any acct
			if(acct_lock == 0 || acctSession == NULL)
			{
				if(createAcct(token) == 0)
				{
					//printf("We made the account.\n");
					write(socket_num, "We made the account.", 20);
				}else{
					//printf("An account with that name already exists. Pls try again.\n");
					write(socket_num, "An account with that name already exists. Please try again.", 59);
				}
			}else{
					//printf("We cannot run this command, you are locked into another account.\n");
					write(socket_num, "We cannot run this command, you are locked into another account.", 64);
			}
		}else if(strncmp(client_message, "serve ", 6) == 0){
			printf("We got the serve command\n");

			//need to parse for the arg after the command
			token = strtok(client_message, " ");
			token = strtok(NULL, " ");

			//first we must check that we are not in an active session
			if(acct_lock == 1 || acctSession != NULL)
			{
				//printf("We are in an active session. Must first end this session.\n");
				write(socket_num, "We are in an active session. Must first end this session.", 57);

			}else{//we can search for the account and activate the session, else return error
				acctSession = serve(token);
				//check if ptr is NULL or we recieved the proper account
				if(acctSession == NULL)
				{
					//printf("We cannot connect you to your desired account. Either the account doesn't exist or it is in use by another client.\n");
					write(socket_num, "We cannot connect you to your desired account. Either the account doesn't exist or it is in use by another client.", 114);
				}else{
					//printf("We found the account.\n");
					write(socket_num, "We found the account.", 21);
					acct_lock = 1;
					acctSession->active = 1;
				}
			}
		}else if(strncmp(client_message, "deposit ", 8) == 0){
			//printf("We got the deposit command\n");

			//we must first check for an active session, this is only valid within an active session
			if(acct_lock == 0 || acctSession == NULL)
			{
				//printf("You must be in an Active Session first. Please use Serve command or Create a new account.\n");
				write(socket_num, "You must be in an Active Session first. Please use Serve command or Create a new account.", 89);

			}else{
				//we can add the deposit amount to the account
				token = strtok(client_message, " ");
				token = strtok(NULL, " ");

				moneyHolder = strtod(token, &wasteBin);
				acctSession->balance = acctSession->balance + moneyHolder;
				//printf("You want to add: %lf\n", moneyHolder);
				//printf("We have added the desired amount.\n");
				write(socket_num, "We have added the desired amount.", 33);
				moneyHolder = 0;
			}
		}else if(strncmp(client_message, "withdraw ", 9) == 0){
			//printf("We got the withdraw command\n");

			if(acct_lock == 0 || acctSession == NULL)
			{
				//printf("You must be in an Active Session first. Please use Serve command or Create a new account.\n");
				write(socket_num, "You must be in an Active Session first. Please use Serve command or Create a new account.", 89);

			}else{
				//we need to store the amount specified in the account
				token = strtok(client_message, " ");
				token = strtok(NULL, " ");

				moneyHolder = strtod(token, &wasteBin);

				//now we need to check if we have the required funds
				if(moneyHolder > acctSession->balance)
				{
					//printf("You do not have the required funds for this transaction.\n");
					write(socket_num, "You do not have the required funds for this transaction.", 56);

				}else{
					acctSession->balance = acctSession->balance - moneyHolder;
					//printf("We have deducted the desired funds from your account.\n");
					write(socket_num, "We have deducted the desired funds from your account.", 53);
				}
				moneyHolder = 0;//reset the money holder
			}

		}else if(strncmp(client_message, "query", 5) == 0){
			//printf("We got the query command\n");
			if(acct_lock == 0 || acctSession == NULL)
			{
				//printf("You must be in an Active Session first. Please use Serve command or Create a new account.\n");
				write(socket_num, "You must be in an Active Session first. Please use Serve command or Create a new account.", 89);

			}else{
				//we must print out the balance on the account
				//printf("The balance on your account is: %lf.\n", acctSession->balance);
				wasteBin = (char*) malloc(sizeof(char) * 40);
				snprintf(wasteBin, 50, "%lf", acctSession->balance);
				retnstr = (char*) malloc(sizeof(char) * 50);
				strcpy(retnstr, "The balance on your account is: ");
				strcat(retnstr, wasteBin);
				strcat(retnstr, ".");
				write(socket_num, retnstr,50);
				free(wasteBin);
				free(retnstr);
			}
		}else if(strncmp(client_message, "end", 3) == 0){
			//printf("We got the end command\n");
			//in this case we must leave the active session
			if(acct_lock == 0 || acctSession == NULL)
			{
				//is not in an active session
				//printf("You are not in Active Session. You must be in an Active Session to end it.\n");
				write(socket_num, "You are not in Active Session. You must be in an Active Session to end it.", 74);

			}else{
				//we need to leave the active session
				acct_lock = 0;
				//we must also change the active bit in the struct to off
				acctSession->active = 0;//we have now made the acct inactive
				acctSession = NULL;
				//printf("The Active Session has now been ended.\n");
				write(socket_num, "The Active Session has now been ended.", 38);

			}
		}else if(strncmp(client_message, "quit", 4) == 0){
			printf("We got the quit command\n");
			//in the quit function, we need to end the current Active Account
			//and then tell the client to quit first
			//and then we end the connection with the client socket
			if(acct_lock == 1 || acctSession != NULL){
				//we need to end the current Active Account
				acctSession->active = 0;
				acct_lock = 0;
				acctSession = NULL;
				//we also need to search the threadsLL, join the thread, and remove from the LL
				threadsLL* t_ptr;
				threadsLL* t_prev;
				t_ptr = front;
				t_prev = NULL;
				//lets traverse the threadsLL in search of this sockfd
				while(t_ptr != NULL)
				{
					if(t_ptr->sockfd == socket_num)
					{
						//we join the thread, remove from LL, free node
						pthread_join(t_ptr->name, NULL);
						if(t_prev == NULL)
						{
							front = front->next;
							free(t_ptr);
							break;
						}else{
							t_prev->next = t_ptr->next;
							free(t_ptr);
							break;
						}
					}else{
						t_prev = t_ptr;
						t_ptr = t_ptr->next;
					}
				}
				
			}
			//we need to tell client to quit and the server connection
			break;
		}else{ //this was not a valid command, return invalid entry and which commands are possible
			//printf("This is not a vaild command\n");
			write(socket_num, "This is not a valid command.", 28);
		}

	}

	close(socket_num);

	return NULL;
}


int createAcct(char* newAcctName)//will return 0 if properly created acct, else return 1 if already exists
{

	//check LL of clientAcct's if name matches to an already existent account
	clientAcct* ptr;
	ptr = client_front;

	//printf("We in the Create Acct function!\n");

	while(ptr != NULL)
	{
		if(strcmp(ptr->name, newAcctName) == 0)
		{
			printf("The account already exists!\n");
			return 1;
		}else{
			ptr = ptr->next;
		}
		
	}
	//printf("Finished checking the LL\n");
	//create new node, initialize it, and add it to the front of the list
	clientAcct* newnode = (clientAcct*) malloc(sizeof(clientAcct));
	//printf("Made a new node\n");
	newnode->name = (char*) malloc(sizeof(char) * 255);
	strcpy(newnode->name, newAcctName);
	printf("Input the name\n");
	newnode->balance = 0;
	newnode->active = 0;
	if(client_front == NULL){
		newnode->next = NULL;
		client_front = newnode;
	}else{
		newnode->next = client_front;
		client_front = newnode;
	}

	return 0;
}


clientAcct* serve(char* name)
{
	clientAcct* ptr;
	ptr = client_front;

	while(ptr != NULL){
		if(strcmp(ptr->name, name) == 0){
			//we have found the acct
			if(ptr->active == 0){
				return ptr;
			}else{
				return NULL;
			}
		}else{
			ptr = ptr->next;
		}
	}
	return ptr;
}


void timer_went_off()
{

	signal(SIGALRM, (void(*)(int)) timer_went_off);

	clientAcct* ptr;

	ptr = client_front;

	//printf("%s\n", "Timer went off");

	while (ptr != NULL){

        	if(ptr->active == 1){ //account is in session

                printf("%s%s\t%s\t%s%f\n\n", "Account Name: ", ptr->name, "IN SERVICE", "Balance: ", ptr->balance);

		}else{ //account is not in session

                	printf("%s%s\t%s\t%s%f\n\n", "Account Name: ", ptr->name,"NOT IN SERVICE", "Balance: ", ptr->balance);

            	}

            	ptr = ptr->next;

    }

	struct itimerval it_val;
	it_val.it_value.tv_sec = 15;
	it_val.it_value.tv_usec = 0;
	it_val.it_interval = it_val.it_value;
	setitimer(ITIMER_REAL, &it_val, NULL);


    return;
}

void ctrlC_shutdown()
{
	//printf("\nWe just caught the signal CTRL+C\n");	

	//must stop the timer
	
	//lock all accounts
	clientAcct* c_ptr = client_front;
	while(c_ptr != NULL)
	{
		if(c_ptr->active == 1)
		{
			c_ptr->active = 0;
		}
		c_ptr = c_ptr->next;
	}


	//deallocate all memory

	//close all sockets
	close(socket_descriptor);

	//join all threads ; we can traverse the whole LL of threads to join them all together
	//printf("We need to join all of the threads!\n");
	threadsLL* finalPTR;
	finalPTR = front;
	while(finalPTR != NULL)
	{
		//pthread_join(finalPTR->name, NULL);
		pthread_detach(finalPTR->name);
		write(finalPTR->sockfd, "quit", 4);
		close(finalPTR->sockfd);
		finalPTR = finalPTR->next;
	}
	//printf("We joined all threads\n");
	//printf("Let's free all the nodes\n");
	//we must free all node in threadsLL
	threadsLL* ptr = front;
	while(front != NULL)
	{
		ptr = front;
		front = front->next;
		free(ptr);
	}

	while(client_front != NULL)
	{
		c_ptr = client_front;
		client_front = client_front->next;
		free(c_ptr);
	}

	exit(0);
}

int main(int argc, char* argv[])
{

	/* This is the setup for a basic server  */
	
	//this is to catch the ctrl+C signal
	signal(SIGINT, ctrlC_shutdown);

	//this is to catch the itimer signal
	signal(SIGALRM, (void (*)(int)) timer_went_off);
	//sigaction(SIGALRM, NULL, (void (*)(int)) timer_went_off);
	//need to establish the timer to start
	struct itimerval it_val;
	it_val.it_value.tv_sec = 15;
	it_val.it_value.tv_usec = 0;
	it_val.it_interval = it_val.it_value;
	setitimer(ITIMER_REAL, &it_val, NULL);

	if(argc != 2)
	{
		printf("Not enough input args\n");
		return 0;
	}

	int portNumber = atoi(argv[1]);
	printf("This is the port number: %d\n", portNumber);

	//setup the LL for pthreadsLL
	//threadsLL* front = NULL;
	//threadsLL* traverser = NULL;

	socket_descriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	/* int socket(int domain, int type, int protocol)  */
	//have now receieved a socket descriptor
	
	struct sockaddr_in server_address;

	memset(&server_address, 0, sizeof(server_address));
	
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(portNumber);

	bind(socket_descriptor, (struct sockaddr*) &server_address, sizeof(server_address));

	listen(socket_descriptor, 20);

	struct sockaddr_in client_address;
	socklen_t client_address_size = sizeof(client_address);

	//we want to be able to use a while loop to continuously keep connecting to new clients
	//we can manipulate the accept function and keep infinitely waiting inside the while loop
	
	int client_socket;
	while(1)
	{

		client_socket = accept(socket_descriptor, (struct sockaddr*) &client_address, &client_address_size);
		//now we need to check if we have actually recieved a proper client
		if(client_socket > 0)
		{
			printf("We have successfully connected to the Client\n");

			threadsLL* newNode = (threadsLL*) malloc(sizeof(threadsLL));
			newNode->next = NULL;
			newNode->sockfd = client_socket;
			if(front == NULL){
				//printf("Front was pointing to NULL, now points to the first node\n");
				front = newNode;
			}else {
				//printf("We added a new node to the LL\n");
				newNode->next = front;
				front = newNode;
			}

			pthread_create(&newNode->name, NULL, clientHandler, &client_socket);	
		}
	}//closes the while loop

	return 0;
}
