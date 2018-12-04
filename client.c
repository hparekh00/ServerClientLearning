#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char* argv[])
{
	int socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
	
	struct sockaddr_in server_address;

	memset(&server_address, 0, sizeof(server_address));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_address.sin_port = htons(1234);
	
	connect(socket_descriptor, (struct sockaddr*) &server_address, sizeof(server_address));
	
	char buffer[40];
	read(socket_descriptor, buffer, sizeof(buffer)-1);

	printf("Message from the server:\t%s\n", buffer);

	close(socket_descriptor);

	return 0;
}
