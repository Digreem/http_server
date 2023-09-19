#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 8080
#define MAX_LENGTH  100
#define SOCKET_OPT_ENABLE 1
#define ECHO_MODE 1
#define MAX_HEADER_SIZE 32
#define RESPONSE_BUF_SIZE MAX_HEADER_SIZE + MAX_LENGTH

char msg_buff[MAX_LENGTH];
char response_buff[RESPONSE_BUF_SIZE];

void communicate(int sockfd)
{
    int n;
    // infinite loop for chat
    for (;;) {
        bzero(msg_buff, MAX_LENGTH);
        bzero(response_buff, RESPONSE_BUF_SIZE);

        // read the message from client and copy it in buffer
        read(sockfd, msg_buff, sizeof(msg_buff));
        // print buffer which contains the client contents
        printf("From client: %s", msg_buff);

        if(!ECHO_MODE)
        {
        	n = 0;
        	// copy server message in the buffer
        	printf("Enter the string to send : ");
        	fflush(stdin);
        	while ((response_buff[n++] = getchar()) != '\n')
            ;
        }
        else
        {
        	strncpy(response_buff, "\n\r *** SERVER ECHO ***\n\r ", MAX_HEADER_SIZE);
        	strncat(response_buff, msg_buff, MAX_LENGTH);
        }

        // and send that buffer to client
        write(sockfd, response_buff, strlen(response_buff));

        // if msg contains "Exit" then server exit and chat ended.
        if (strncmp("exit", msg_buff, 4) == 0) {
            printf("Server Exit...\n");
            break;
        }
    }
}

int main(int argc, char *argv[])
{
	printf("Hello from server!!!\n");
	// Listening & connection sockets
	int listen_sock, connect_sock;
	// Socket address structure
	struct sockaddr_in servaddr, clientaddr;

	listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_sock == -1)
	{
		printf("# Error: socket creation failed #\n");
		exit(0);
	}
	else
		printf("# Socket successfully created #\n");

	// assign IP, PORT
	memset (&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	// set reusing socket option
	const int opt  = SOCKET_OPT_ENABLE;
	if(setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		printf("# Error on setting SO_REUSEADDR option to port #%d\n", PORT);
		exit(0);
	}

	// Binding newly created socket to given IP and verification
	if ((bind(listen_sock, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
		printf("# Error: socket bind failed...\n #");
		exit(0);
	}
	else
		printf("# Socket successfully binded #\n");

	// Now server is ready to listen
	if ((listen(listen_sock, 5)) != 0) {
		printf("# Error: Listen failed #\n");
		exit(0);
	}
	else
		printf("# Server listening for incoming connections...\n");

	// Accept the data packet from client and verification
	socklen_t client_len = sizeof(clientaddr);
	connect_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &client_len);
	if (connect_sock< 0)
	{
		printf("# Error: server accept failed #\n");
		exit(0);
	}
	else
		printf("# Error: server acccepted the client #\n");

	communicate(connect_sock);

	close(listen_sock);
	printf("# Listening socket closed #\n");
	close(connect_sock);
	printf("# Connection socket closed #\n");
	return 0;
}
