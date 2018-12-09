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

/* This is the setup for a basic server.
 * We have now implemented multiple client functionality
 * and also are now implementing the use of threading.   */


//we can make a LL of pthread_t to store all the threads we make
typedef struct threadsLL{
	pthread_t name;
	struct threadsLL* next;
} threadsLL;



//we can now start to try and establish PTHREADS

void *clientHandler(void* client_socket){
	int socket_num = *(int*) client_socket;

	char response[] = "This is the SERVER speaking -- wassup ;)";
	write(socket_num, response, sizeof(response));
	close(socket_num);
	return NULL;
}


int main(int argc, char* argv[])
{

	/* This is the setup for a basic server  */

	//setup the LL for pthreadsLL
	threadsLL* front = NULL;
	threadsLL* traverser = NULL;

	int socket_descriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	/* int socket(int domain, int type, int protocol)  */
	//have now receieved a socket descriptor
	
	struct sockaddr_in server_address;

	memset(&server_address, 0, sizeof(server_address));
	
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_address.sin_port = htons(1234);

	bind(socket_descriptor, (struct sockaddr*) &server_address, sizeof(server_address));

	listen(socket_descriptor, 20);

	struct sockaddr_in client_address;
	socklen_t client_address_size = sizeof(client_address);

	//we want to be able to use a while loop to continuously keep connecting to new clients
	//we can manipulate the accept function and keep infinitely waiting inside the while loop
	
	int client_socket;
	int counter = 0;
	while(1){

		client_socket = accept(socket_descriptor, (struct sockaddr*) &client_address, &client_address_size);
		//now we need to check if we have actually recieved a proper client
		if(client_socket > 0){
			printf("SERVER -- We have successfully connected to the Client\n");

			threadsLL* newNode = (threadsLL*) malloc(sizeof(threadsLL));
			newNode->next = NULL;
			if(front == NULL){
				front = newNode;
				traverser = newNode;
			}else {
				traverser->next = newNode;
				traverser = newNode;
			}

			pthread_create(&newNode->name, NULL, clientHandler, &client_socket);	
			/*
			char str[] = "Hello World!";
		        write(client_socket, str, sizeof(str));
			close(client_socket);
			*/
			counter++;
		}
		//lets test for 2 clients to connect
		if(counter == 2){
			break;//we leaving trying to connect to clients
		}
	}//closes the while loop

	close(socket_descriptor);

	return 0;
}
